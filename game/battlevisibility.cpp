/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "battlevisibility.h"
#include "battlescene.h"
#include "../engine/engine.h"

#include "../grinliz/glrectangle.h"



using namespace grinliz;

Visibility::Visibility() : battleScene( 0 ), units( 0 ), map( 0 )
{
	for( int i=0; i<MAX_UNITS; ++i )
		current[i] = false;
	fogInvalid = true;
}


void Visibility::InvalidateUnit( int i ) 
{
	GLRELASSERT( i>=0 && i<MAX_UNITS );
	current[i] = false;
	// Do not check IsAlive(). Specifically called when units are alive or just killed.
	if ( i >= TERRAN_UNITS_START && i < TERRAN_UNITS_END ) {
		fogInvalid = true;
	}
}


void Visibility::InvalidateAll( const Rectangle2I& bounds )
{
	if ( bounds.Empty() )
		return;

	Rectangle2I vis;

	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( units[i].IsAlive() ) {
			units[i].CalcVisBounds( &vis );
			if ( bounds.Intersect( vis ) ) {
				current[i] = false;
				if ( units[i].Team() == TERRAN_TEAM ) {
					fogInvalid = true;
				}
			}
		}
	}
}


void Visibility::InvalidateAll()
{
	for( int i=0; i<MAX_UNITS; ++i ) {
		current[i] = false;
	}
	fogInvalid = true;
}


void Visibility::CalcTeam( int team, int* r0, int* r1 )
{
	if ( team == TERRAN_TEAM ) {
		*r0 = TERRAN_UNITS_START;
		*r1 = TERRAN_UNITS_END;
	}
	else if ( team == ALIEN_TEAM ) {
		*r0 = ALIEN_UNITS_START;
		*r1 = ALIEN_UNITS_END;
	}
	else if ( team == CIV_TEAM ) {
		*r0 = CIV_UNITS_START;
		*r1 = CIV_UNITS_END;
	}
	else {
		GLRELASSERT( 0 );
	}
}



bool Visibility::TeamCanSee( int team, int x, int y )
{
	//GRINLIZ_PERFTRACK
	int r0=0, r1=0;
	CalcTeam( team, &r0, &r1 );

	for( int i=r0; i<r1; ++i ) {
		// Check isAlive to avoid the function call.
		if ( units[i].IsAlive() && UnitCanSee( i, x, y ) )
			return true;
	}
	return false;
}


int Visibility::NumTeamCanSee( int viewer, int viewee )
{
	int b0=0, b1=0;
	CalcTeam( viewee, &b0, &b1 );
	
	int count = 0;
	
	for( int i=b0; i<b1; ++i ) {
		if ( units[i].IsAlive() ) {
			Vector2I p = units[i].MapPos();

			if ( TeamCanSee( viewer, p.x, p.y ) ) {
				++count;
			}
		}
	}
	return count;
}


bool Visibility::UnitCanSee( int i, int x, int y )
{
	GLASSERT( i >= 0 && i <MAX_UNITS );

	if ( Engine::mapMakerMode ) {
		return true;
	}
	else if ( units[i].IsAlive() ) {
		if ( !current[i] ) {
			CalcUnitVisibility( i );
			current[i] = true;
		}
		if ( visibilityMap.IsSet( x, y, i ) ) {
			return true;
		}
	}
	return false;
}



/*	Huge ol' performance bottleneck.
	The CalcVis() is pretty good (or at least doesn't chew up too much time)
	but the CalcAll() is terrible.
	Debug mode.
	Start: 141 MClocks
	Moving to "smart recursion": 18 MClocks - that's good! That's good enough to hide the cost is caching.
		...but also has lots of artifacts in visibility.
	Switched to a "cached ray" approach. 58 MClocks. Much better, correct results, and can be optimized more.

	In Core clocks:
	"cached ray" 45 clocks
	33 clocks after tuning. (About 1/4 of initial cost.)
	Tighter walk: 29 MClocks

	Back on the Atom:
	88 MClocks. But...experimenting with switching to 360degree view.
	...now 79 MClocks. That makes little sense. Did facing take a bunch of cycles??
*/


void Visibility::CalcUnitVisibility( int unitID )
{
	//unit = units;	// debugging: 1st unit only
	GLRELASSERT( unitID >= 0 && unitID < MAX_UNITS );
	const Unit* unit = &units[unitID];

	Vector2I pos = unit->MapPos();

	// Clear out the old settings.
	// Walk the area in range around the unit and cast rays.
	visibilityMap.ClearPlane( unitID );
	visibilityProcessed.ClearAll();

	Rectangle2I mapBounds = map->Bounds();

	// Can always see yourself.
	visibilityMap.Set( pos.x, pos.y, unitID );
	visibilityProcessed.Set( pos.x, pos.y, 0 );

	const int MAX_SIGHT_SQUARED = MAX_EYESIGHT_RANGE*MAX_EYESIGHT_RANGE;

	for( int r=MAX_EYESIGHT_RANGE; r>0; --r ) {
		Vector2I p = { pos.x-r, pos.y-r };
		static const Vector2I delta[4] = { { 1,0 }, {0,1}, {-1,0}, {0,-1} };

		for( int k=0; k<4; ++k ) {
			for( int i=0; i<r*2; ++i ) {
				if (    mapBounds.Contains( p )
					 && !visibilityProcessed.IsSet( p.x, p.y )
					 && (p-pos).LengthSquared() <= MAX_SIGHT_SQUARED ) 
				{
					CalcVisibilityRay( unitID, p, pos );
				}
				p += delta[k];
			}
		}
	}
}


void Visibility::CalcVisibilityRay( int unitID, const Vector2I& pos, const Vector2I& origin )
{
	/* Previous pass used a true ray casting approach, but this doesn't get good results. Numerical errors,
	   view stopped by leaves, rays going through cracks. Switching to a line walking approach to 
	   acheive stability and simplicity. (And probably performance.)
	*/


#if 0
#ifdef DEBUG
	{
		// Max sight
		const int MAX_SIGHT_SQUARED = MAX_EYESIGHT_RANGE*MAX_EYESIGHT_RANGE;
		Vector2I vec = pos - origin;
		int len2 = vec.LengthSquared();
		GLASSERT( len2 <= MAX_SIGHT_SQUARED );
		if ( len2 > MAX_SIGHT_SQUARED )
			return;
	}
#endif
#endif

	const Surface* lightMap = map->GetLightMap();
	GLRELASSERT( lightMap->Format() == Surface::RGB16 );

	const float OBSCURED = 0.50f;
	const float DARK  = ((units[unitID].Team() == ALIEN_TEAM) ? 1.5f :2.0f) / (float)MAX_EYESIGHT_RANGE;
	const float LIGHT = 1.0f / (float)MAX_EYESIGHT_RANGE;
	//const int EPS = 10;

	float light = 1.0f;
	bool canSee = true;

	// Always walk the entire line so that the places we can not see are set
	// as well as the places we can.
	LineWalk line( origin.x, origin.y, pos.x, pos.y ); 
	while ( line.CurrentStep() <= line.NumSteps() )
	{
		Vector2I p = line.P();
		Vector2I q = line.Q();
		Vector2I delta = q-p;

		if ( canSee ) {
			canSee = map->CanSee( p, q );

			if ( canSee ) {
				U16 c = lightMap->GetImg16( q.x, q.y );
				Color4U8 rgba = Surface::CalcRGB16( c );

				const float distance = ( delta.LengthSquared() > 1 ) ? 1.4f : 1.0f;

				if ( map->Obscured( q.x, q.y ) ) {
					light -= OBSCURED * distance;
				}
				else if ( map->Flared( q.x, q.y ) ) {
					// Do nothing. Treat as perfect white.
				}
				else
				{
					// Blue channel is typically high. So 
					// very dark  ~255
					// very light ~255*3 (white)
					int lum = rgba.r + rgba.g + rgba.b;
					float fraction = Interpolate( 255.0f, DARK, 765.0f, LIGHT, (float)lum ); 
					light -= fraction * distance;
				}
			}
		}
		visibilityProcessed.Set( q.x, q.y );
		if ( canSee ) {
			visibilityMap.Set( q.x, q.y, unitID, true );
		}

		// If all the light is used up, we will see no further.	
		if ( canSee && light < 0.0f )
			canSee = false;	

		line.Step(); 
	}
}


bool Visibility::UnitCanSee( const Unit* src, const Unit* target )
{
	const Vector2I t = target->MapPos();
	return UnitCanSee( src - units, t.x, t.y );
}


void Visibility::CalcVisMap( grinliz::BitArray<MAX_UNITS, MAX_UNITS, 1>* canSeeMap )
{
	canSeeMap->ClearAll();

	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( units[i].IsAlive() ) {
			for( int j=0; j<MAX_UNITS; ++j ) {
				if ( units[j].IsAlive() && UnitCanSee( &units[i], &units[j] ) ) {
					canSeeMap->Set( i, j );
				}
			}
		}
	}
}


