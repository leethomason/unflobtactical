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

#include "ai.h"
#include "unit.h"
#include "targets.h"
#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/map.h"
#include "../grinliz/glperformance.h"
#include "../grinliz/glutil.h"
#include "tacmap.h"

#include <float.h>
#include <limits.h>

using namespace grinliz;


AI::AI( int team, Visibility* vis, Engine* engine, const Unit* units, BattleScene* battleScene )
{
	m_team = team;
	m_visibility = vis;
	m_engine = engine;
	m_units = units;
	m_battleScene = battleScene;

	for( int i=0; i<MAX_UNITS; ++i ) {
		m_enemy[i] = 0.0f;
	}

	if ( m_team == ALIEN_TEAM) {
		for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i )
			m_enemy[i] = 1.0f;
		for( int i=CIV_UNITS_START; i<CIV_UNITS_END; ++i ) 
			m_enemy[i] = 0.3f;
	}
	else if ( m_team == TERRAN_TEAM ) {
		for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i )
			m_enemy[i] = 1.0f;
	}
	else if ( m_team == CIV_TEAM ) {
		for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i )
			m_enemy[i] = 1.0f;
	}
	else {
		GLASSERT( 0 );
	}

	for( int i=0; i<MAX_UNITS; ++i ) {
		m_lkp[i].pos.Set( 0, 0 );
		m_lkp[i].turns = MAX_TURNS_LKP;
		m_travel[i].Set( m_random.Rand(MAP_SIZE), m_random.Rand(MAP_SIZE) );
	}
}


void AI::StartTurn( const Unit* units )
{
	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( units[i].IsAlive() ) {
			if ( m_visibility->TeamCanSee( m_team, units[i].MapPos() ) ) {
				m_lkp[i].pos = units[i].MapPos();
				m_lkp[i].turns = 0;
			}
			else {
				m_lkp[i].turns++;
			}
		}
		m_thinkCount[i] = 0;
	}
	m_numSpitters = 0;
	if ( m_team == ALIEN_TEAM ) {
		for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) {
			if ( units[i].IsAlive() && units[i].AlienType() == Unit::ALIEN_SPITTER ) {
				++m_numSpitters;
			}
		}
	}
}


void AI::Inform( const Unit* theUnit, int quality )
{
	int i = theUnit - m_units;
	if ( m_lkp[i].turns >= quality ) {
		m_lkp[i].turns = quality;
		m_lkp[i].pos = theUnit->MapPos();
	}
}


bool AI::SafeLineOfSight(	const Unit* source, 
							const Unit* target, 
							int mode,
							bool multicast,
							Engine* engine,
							BattleScene* battle )
{
	const Model* sourceModel		= battle->GetModel( source );
	const Model* sourceWeaponModel	= battle->GetWeaponModel( source );
	const Model* targetModel		= battle->GetModel( target );

	if ( !sourceModel || !targetModel ) {
		return false;
	}

	int sourceTeam = source->Team();
	int targetTeam = target->Team();

	Vector3F sourcePos, targetPos;

	float fireRotation = source->AngleBetween( target->MapPos(), false );
	sourceModel->CalcTrigger( &sourcePos, &fireRotation );
	targetModel->CalcTarget( &targetPos );

	float length = ( targetPos - sourcePos ).Length();
	
	Vector3F normal = ( targetPos - sourcePos );
	normal.Normalize();

	static const Vector3F up = { 0, 1, 0 };
	Vector3F tangent;
	CrossProduct( normal, up, &tangent );
	
	const WeaponItemDef* wid = source->GetWeaponDef();
	Accuracy accuracy = source->CalcAccuracy( mode );

	// Don't blow ourselves up.
	if ( wid->IsExplosive( mode ) && length <= EXPLOSIVE_RANGE ) {
		return false;
	}

	const int COUNT = multicast ? 5 : 1;

	// Send out rays over the possible shooting space, see what happens. Basically want to know:
	// 1. Does the center ray hit.
	// 2. Do other possible solutions do bad things.

	for( int i=0; i<COUNT; ++i ) {
		float delta = 0;
		if ( COUNT > 1 ) {
			delta = Interpolate(  0.f,             -accuracy.RadiusAtOne()*length,
								(float)(COUNT-1), accuracy.RadiusAtOne()*length,
								float(i) );
		}
		Vector3F t = sourcePos + normal*length + tangent*delta;

		Ray ray;
		ray.origin = sourcePos;
		ray.direction = t - sourcePos;
		Vector3F intersection;

		const Model* ignore[3] = { sourceModel, sourceWeaponModel, 0 };
		Model* m = engine->IntersectModel( ray, TEST_TRI, 0, 0, ignore, &intersection );
		float distanceToImpact = m ? (intersection - sourcePos).Length() : (float)MAP_SIZE;

		// Did we hit our own team?
		const Unit* u = battle->GetUnit( m, false );
		if ( !u ) {
			u = battle->GetUnit( m, true );
		}
		if ( u && u->Team() == sourceTeam ) {
			GLOUTPUT(( "Ray fail %d: hit own team\n", battle->GetUnitID( source ) ));
			return false;
		}
		// Is an explosive weapon too close?
		if ( wid->IsExplosive(mode) && m && distanceToImpact <= EXPLOSIVE_RANGE ) {
			GLOUTPUT(( "Ray fail %d: blow up in face.\n", battle->GetUnitID( source ) ));
			return false;
		}

		// Do we actually hit the target? Only check for the center ray cast.
		if ( i == COUNT/2 ) {
			if ( u && u->Team() == targetTeam ) {
				// all good.
				GLOUTPUT(( "Ray main pass %d.\n", battle->GetUnitID( source ) ));
			}
			else {
				GLOUTPUT(( "Ray fail %d: no line of site to target.\n", battle->GetUnitID( source ) ));
				return false;
			}
		}
	}
	return true;
}


void AI::TrimPathToCost( MP_VECTOR< grinliz::Vector2<S16> >* path, float maxCost )
{
	float cost = 0.0f;

	for ( unsigned i=1; i<path->size(); ++i ) {
		const Vector2<S16>& p0 = (*path)[ i-1 ];
		const Vector2<S16>& p1 = (*path)[ i ];
		if ( abs( p0.x-p1.x ) && abs( p0.y-p1.y ) ) {
			cost += 1.41f;
		}
		else {
			cost += 1.0f;
		}
		if ( cost > maxCost ) {
			path->resize( i );
			return;
		}
	}
}


int AI::VisibleUnitsInArea(	const Unit* theUnit,
							const Unit* units,
							const grinliz::Rectangle2I& bounds )
{
	int count = 0;
	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( m_enemy[i] > 0 && units[i].IsAlive() ) {
			if ( m_visibility->TeamCanSee( theUnit->Team(), units[i].MapPos() ) ) {
				Vector2I p;
				units[i].CalcMapPos( &p, 0 );
				if ( bounds.Contains( p ) )
					++count;
			}
		}
	}
	return count;
}


int AI::ThinkShoot(	const Unit* theUnit,
					TacMap* map,
					AIAction* action )
{
	static const float MINIMUM_FIRE_CHANCE			= 0.02f;	// A shot is only valid if it has this chance of hitting.
	static const int   EXPLOSION_ZONE				= 2;		// radius to check of clusters of enemies to blow up
	static const float	MINIMUM_EXPLOSIVE_RANGE		= 4.0f;

	if ( !theUnit->HasGunAndAmmo( true ) ) {
		GLASSERT( 0 );	// should have been weeded out upstream.
		return THINK_NOT_OPTION;
	}

	int best = -1;
	float bestScore = 0.0f;
	int bestMode = 0;
	float bestChance = 0.0f;

	const WeaponItemDef* wid = theUnit->GetWeaponDef();
	GLASSERT( wid );

	for( int i=0; i<MAX_UNITS; ++i ) {
		if (    m_enemy[i] > 0
			 && m_units[i].IsAlive() 
			 && m_battleScene->GetModel( &m_units[i] )
			 && m_visibility->UnitCanSee( theUnit, &m_units[i] )
			 && SafeLineOfSight( theUnit, &m_units[i], 0, false, m_engine, m_battleScene ) )
		{
			// special case: aliens won't shoot terrans if they have spitters.
			if (    m_numSpitters
				 && m_team == ALIEN_TEAM
				 && !(theUnit->AlienType() == Unit::ALIEN_SPITTER)
				 && m_units[i].Team() == CIV_TEAM ) 
			{
				continue;
			}

			int len2 = (m_units[i].MapPos() - theUnit->MapPos()).LengthSquared();
			float len = sqrtf( (float)len2 );

			BulletTarget bulletTarget( len );
			m_battleScene->GetModel( &m_units[i] )->CalcTargetSize( &bulletTarget.width, &bulletTarget.height );


			for ( int mode=0; mode<WeaponItemDef::BASE_MODES; ++mode ) {

				if ( theUnit->CanFire( mode ) ) {
					float chance, anyChance, tu, dptu;
					
					if ( theUnit->FireStatistics( mode, bulletTarget, &chance, &anyChance, &tu, &dptu ) ) {
						float score = dptu * m_enemy[i];	// Interesting: good AI, but results in odd choices.

						if ( wid->IsExplosive( mode ) ) {
							if ( len < MINIMUM_EXPLOSIVE_RANGE ) {
								score = 0.0f;
							}
							else {
								Rectangle2I bounds;
								bounds.pos = m_units[i].MapPos();
								bounds.size.Set( 1, 1 );
								bounds.Outset( EXPLOSION_ZONE );
								
								int count = VisibleUnitsInArea( theUnit, m_units, bounds );
								score *= ( count <= 1 ) ? 0.5f : (float)count;
							}
						}

						if ( chance >= MINIMUM_FIRE_CHANCE && score > bestScore ) {
							bestScore = score;
							best = i;
							bestMode = mode;
							bestChance = chance;
						}
					}
				}
			}
		}
	}
	if ( best >= 0 ) {
		action->actionID = ACTION_SHOOT;
		action->shoot.mode = bestMode;
		m_battleScene->GetModel( &m_units[best] )->CalcTarget( &action->shoot.target );
		m_battleScene->GetModel( &m_units[best] )->CalcTargetSize( &action->shoot.targetWidth, &action->shoot.targetHeight );
		return THINK_ACTION;
	}
	return THINK_NO_ACTION;
}


int AI::ThinkPsiAttack( const Unit* theUnit, AIAction* action )
{
	if (    theUnit->HasPsiAttack() 
		 && theUnit->TU() >= TU_PSI  ) 
	{
		// Survey: 
		float enemyScore[3] = { 0, 0, 0 };
		for( int i=0; i<MAX_UNITS; ++i ) {
			if ( m_units[i].IsAlive() ) {
				int team = m_units[i].Team();

				// This turn or last treated as the same, so we don't re-blast too much. 
				int turns = m_lkp[i].turns - 1;
				if ( turns < 0 ) turns = 0;	
				enemyScore[team] += (float)turns * m_enemy[i]; 
			}
		}
		int best = -1;
		float bestScore = 0;

		for( int i=0; i<MAX_UNITS; ++i ) {
			if (    m_units[i].IsAlive() 
				 && m_enemy[i] > 0
				 && m_visibility->UnitCanSee( theUnit, &m_units[i] ) )
			{
				float score = enemyScore[m_units[i].Team()];
				score *= 1.0f + 0.1f*m_random.Uniform();

				if ( score > bestScore ) {
					best = i;
					bestScore = score;
				}
			}
		}
		//GLOUTPUT(( "psi score %.1f %.1f %.1f id=%d\n", enemyScore[0], enemyScore[1], enemyScore[2], best ));
		if ( best >= 0 ) {
			action->actionID = ACTION_PSI_ATTACK;
			action->psi.targetID = best;
			return THINK_ACTION;
		}
	}
	return THINK_NO_ACTION;
}


int AI::ThinkMoveToAmmo(	const Unit* theUnit,
							TacMap* map,
							AIAction* action )
{
	// Is theUnit already standing on the Storage? If so, use!
	Vector2I theUnitPos = theUnit->MapPos();
	const Storage* storage = map->GetStorage( theUnitPos.x, theUnitPos.y );
	
	if ( storage && storage->IsResupply( theUnit->GetWeaponDef() )) {
		return THINK_SOLVED_NO_ACTION;
	}

	// Need to find Storage and go there.
	// This approach may find a destinatino that we can't go to, but
	// there are strange bugs coming from the storage code, so I'm
	// re-writing to be simple.
	Vector2I found = map->FindStorage( theUnit->GetWeaponDef(), theUnitPos );
	Vector2<S16> start = { theUnitPos.x, theUnitPos.y };

	if ( found.x >= 0 && found.y >= 0 ) {
		Vector2<S16> end = { found.x, found.y };
		float cost;
		if ( map->SolvePath( theUnit, start, end, &cost, &m_path[0] ) == micropather::MicroPather::SOLVED ) {
			MP_VECTOR< grinliz::Vector2<S16> >& path = m_path[0];
			TrimPathToCost( &path, theUnit->TU() );

			if ( path.size() > 1 ) {
				action->actionID = ACTION_MOVE;
				action->move.path.Init( path );
				return THINK_ACTION;
			}
		}
	}
	return THINK_NO_ACTION;
}


int AI::ThinkInventory(	const Unit* theUnit, TacMap* map, AIAction* action )
{
	Vector2I pos = theUnit->MapPos();

	// Drop all the weapons, and pick up new ones.
	const Storage* storage = map->GetStorage( pos.x, pos.y );

//	if (	  theUnit->HasGunAndAmmo( false )									// all good, just need to re-distribute. Should work, but buggy. 
	
	if ( storage && storage->IsResupply( theUnit->GetWeaponDef() ) )		// can pick up new stuff
	{
		action->actionID = ACTION_INVENTORY;
		return THINK_ACTION;
	}
	return THINK_NO_ACTION;
}


int AI::ThinkSearch(const Unit* theUnit,
					int flags,
					TacMap* map,
					AIAction* action )
{
	int best = -1;
	float bestGolfScore = FLT_MAX;

	// Reserve for auto or snap??
	float tu = theUnit->TU();

	if ( theUnit->CanFire( 1 ) ) {
		tu -= theUnit->FireTimeUnits( 1 );
	}
	else if ( theUnit->CanFire( 0 ) ) {
		tu -= theUnit->FireTimeUnits( 0 );
	}

	// TU is adjusted for weapon time. If we can't effectively move any more, stop now.
	if ( tu < 1.8f ) {
		return THINK_NO_ACTION;
	}

	// Are we more or less at the LKP? If so, mark it unknown. Keeps everyone from rushing a long cold spot.
	Rectangle2I zone;
	zone.pos = theUnit->MapPos();
	zone.size.Set( 1, 1 );
	zone.Outset( 1 );

	for( int i=0; i<MAX_UNITS; ++i ) {
		if (    m_enemy[i] > 0 
			 &&	m_units[i].IsAlive() 
			 && m_battleScene->GetModel( &m_units[i] )
			 && m_lkp[i].turns < MAX_TURNS_LKP ) 
		{
			
			// Check for a position going invalid.
			if (    m_lkp[i].turns >= 2 
				 && zone.Contains( m_lkp[i].pos )) 
			{
				m_lkp[i].turns = MAX_TURNS_LKP;
				continue;
			}

			int len2 = (theUnit->MapPos()-m_lkp[i].pos).LengthSquared();

			// Limit just how far units will go charging off. 1/2 the map?
			// Somewhat limits the "zerg rush" AI
			if ( len2 > MAP_SIZE*MAP_SIZE/4 )
				continue;

			float len = sqrtf( (float)len2 );

			// The older the data, the worse the score.
			const float NORMAL_TU = (float)(MIN_TU + MAX_TU) * 0.5f;
			float score = len + (float)(m_lkp[i].turns)*NORMAL_TU;

			// Guards only move on what they can currently see
			// so they don't go chasing things.
			if ( ( flags & AI_GUARD ) && !m_visibility->UnitCanSee( theUnit, &m_units[i] ) ) {
				score = FLT_MAX;
			}
					
			if ( score < bestGolfScore ) {
				bestGolfScore = score;
				best = i;
			}
		}
	}
	if ( best >= 0 ) {
		Vector2<S16> start = { theUnit->MapPos().x, theUnit->MapPos().y };
		Vector2<S16> end   = { m_lkp[best].pos.x, m_lkp[best].pos.y };

		float lowCost = FLT_MAX;
		int bestPath = -1;
		const Vector2<S16> delta[4] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
		for( int i=0; i<4; ++i ) {
			// The path is blocked *by our target*. Fooling around with how the map pather
			// works is tweaky. So just check 4 spots.
			float cost;
			int result = map->SolvePath( theUnit, start, end+delta[i], &cost, &m_path[i] );
			if ( result == micropather::MicroPather::SOLVED ) {
				if ( cost < lowCost ) {
					lowCost = cost;
					bestPath = i;
				}
			}
		}
		if ( bestPath >= 0 ) {
			// stupid double array syntax...
			MP_VECTOR< grinliz::Vector2<S16> >& path = m_path[bestPath];
			TrimPathToCost( &path, tu );

			if ( path.size() > 1 ) {
				action->actionID = ACTION_MOVE;
				action->move.path.Init( path );
				return THINK_ACTION;
			}
		}
	}
	return THINK_NO_ACTION;
}


int AI::ThinkWander(	const Unit* theUnit,
						TacMap* map,
						AIAction* action )
{
	// -------- Wander --------- //
	// If the aliens don't see anything, they just stand around. That's okay, except it's weird
	// that they completely skip their turn. So if they are set to wander, then move a space randomly.
	Vector2I choices[8] = { {0,1}, {0,-1}, {1,0},  {-1,0}, 
							{1,1}, {1,-1}, {-1,1}, {-1,-1} };
	for( int i=0; i<8; ++i ) {
		Swap( &choices[m_random.Rand(8)], &choices[m_random.Rand(8)] );
	}
	for ( int i=0; i<8; ++i ) {
		float cost;
		Vector2I pos = theUnit->MapPos();
		Vector2<S16> start = { pos.x, pos.y };
		Vector2<S16> end = { pos.x+choices[i].x, pos.y+choices[i].y };

		int result = map->SolvePath( theUnit, start, end, &cost, &m_path[0] );
		if ( result == micropather::MicroPather::SOLVED && m_path[0].size() == 2 ) {
			TrimPathToCost( &m_path[0], theUnit->TU() );
			if ( m_path[0].size() == 2 ) {
				action->actionID = ACTION_MOVE;
				action->move.path.Init( m_path[0] );
				return THINK_ACTION;
			}
		}
	}
	return THINK_NO_ACTION;
}


int AI::ThinkTravel(	const Unit* theUnit,
						TacMap* map,
						AIAction* action )
{
	// -------- Wander --------- //
	// If the aliens don't see anything, they just stand around. That's okay, except it's weird
	// that they completely skip their turn. Travelling units travel far over wide areas of the map.

	int index = theUnit - m_units;
	int result = -1;
	float cost = 0;

	Rectangle2I mapBounds = map->Bounds();

	// Look for an acceptable travel destination. 4 is abitrary...3-5 all seem pretty modest.
	for( int i=0; i<4; ++i ) {
		if (    mapBounds.Contains( m_travel[index] ) 
			 && m_travel[index] != theUnit->MapPos() )
		{
			Vector2I pos = theUnit->MapPos();
			Vector2<S16> start = { pos.x, pos.y };
			Vector2<S16> end = { m_travel[index].x, m_travel[index].y };
			result = map->SolvePath( theUnit, start, end, &cost, &m_path[0] );
			if ( result == micropather::MicroPather::SOLVED ) {
				TrimPathToCost( &m_path[0], theUnit->TU() );
				if ( m_path[0].size() > 2 ) {
					action->actionID = ACTION_MOVE;
					action->move.path.Init( m_path[0] );
					return THINK_ACTION;
				}
			}
		}

		// Look for a new travel destination. Prefer destinations that aren't currently visible.
		for( int j=0; j<4; ++j ) {
			m_travel[index].x = m_random.Rand( mapBounds.size.x );
			m_travel[index].y = m_random.Rand( mapBounds.size.y );
			
			if ( !m_visibility->TeamCanSee( m_team, m_travel[index] ) )
				break;
		}
	}
	return THINK_NO_ACTION;
}


int AI::ThinkRotate(	const Unit* theUnit,
						TacMap* map,
						AIAction* action )
{
	int best = -1;
	float bestGolfScore = FLT_MAX;

	for( int i=0; i<MAX_UNITS; ++i ) {
		if (    m_enemy[i] > 0
			 && m_units[i].IsAlive() 
			 && m_battleScene->GetModel( &m_units[i] )
			 && m_visibility->TeamCanSee( m_team, m_units[i].MapPos() )
			 && m_lkp[i].turns < MAX_TURNS_LKP ) 
		{
			int len2 = (theUnit->MapPos() - m_lkp[i].pos).LengthSquared();

			// If the enemy isn't within some reasonable shoot range, doesn't matter.
			// go with 1.4*max sight
			if ( len2 > MAX_EYESIGHT_RANGE*MAX_EYESIGHT_RANGE*2 )
				continue;

			float len = sqrtf( (float)len2 );

			// The older the data, the worse the score.
			float golfScore = len*(float)(m_lkp[i].turns) / m_enemy[i];
					
			if ( golfScore < bestGolfScore ) {
				bestGolfScore = golfScore;
				best = i;
			}
		}
	}
	if ( best >= 0 ) {
		action->actionID = ACTION_ROTATE;
		action->rotate.x = m_units[best].MapPos().x;
		action->rotate.y = m_units[best].MapPos().y;
		return THINK_ACTION;
	}
	return THINK_NO_ACTION;
}


int AI::ThinkBase( const Unit* theUnit )
{
	int index = theUnit - m_units;
	GLASSERT( index >= 0 && index < MAX_UNITS );
	m_thinkCount[index] += 1;

	if ( m_thinkCount[index] >= 5 )
		return THINK_NOT_OPTION;
	return THINK_NO_ACTION;
}


bool WarriorAI::Think(	const Unit* theUnit,
						int flags,
						TacMap* map,
						AIAction* action )
{
	// QuickProfile qp( "WarriorAI::Think()" );
	
	// if unit has gun&ammo
	//		
	// Crazy simple 1st AI. If:
	// - theUnit can see something, shoot it. Choose the unit with the
	//	 greatest chance of going down
	// - if on storage, check if we need stuff
	// - can see nothing, move towards something the team can see. Select between
	//   current canSee and LKPs
	// - if nothing to move to, stand around

	action->actionID = ACTION_NONE;
	Vector2I theUnitPos;
	theUnit->CalcMapPos( &theUnitPos, 0 );

	if ( ThinkBase( theUnit ) == THINK_NOT_OPTION )
		return true;
	
	int result = 0;

	// PSI takes no ammo. Check first.
	if ( ThinkPsiAttack( theUnit, action ) == THINK_ACTION ) {
		return false;	
	}

	// Special case: Crawler always runs around.
	if ( theUnit->AlwaysCivAI() ) {
		CivAI civAI( theUnit->Team(), m_visibility, m_engine, m_units, m_battleScene );
		return civAI.Think( theUnit, flags, map, action );
	}

	// -------- Shoot -------- //
	if ( theUnit->HasGunAndAmmo( true ) ) {
		result = ThinkShoot( theUnit, map, action );
		//GLOUTPUT(( "HasGunAndAmmo. ThinkShoot=%d tu=%f\n", result, theUnit->TU() ));
		if ( result == THINK_ACTION )
			return false;	// not done - can shoot again!

		// Generally speaking, only move if not doing shooting first.
		if ( theUnit->TU() > theUnit->GetStats().TotalTU() * 0.9f ) {
			result = ThinkSearch( theUnit, flags, map, action );
			//GLOUTPUT(( "  ThinkSearch=%d tu=%f\n", result, theUnit->TU() ));
			if ( result  == THINK_ACTION )
				return false;	// still will wander & rotate
		}

		if ( theUnit->TU() == theUnit->GetStats().TotalTU() ) {
			if ( flags & AI_TRAVEL ) {
				result = ThinkTravel( theUnit, map, action );
				//GLOUTPUT(( "  ThinkTravel=%d tu=%f\n", result, theUnit->TU() ));
				if ( result == THINK_ACTION )
					return false;
			}
			else {
				result = ThinkWander( theUnit, map, action );
				//GLOUTPUT(( "  ThinkWander=%d tu=%f\n", result, theUnit->TU() ));
				if ( result == THINK_ACTION )
					return false;	// will still rotate
			}
		}
		ThinkRotate( theUnit, map, action );
		return true;
	}
	else {
		AI_LOG(( "[ai.warrior] Unit %d Out of Ammo.\n", theUnit - m_units ));
		// Out of ammo. Get Ammo!
		if ( theUnit->HasGunAndAmmo( false ) ) {
			result = ThinkInventory( theUnit, map, action );
			if ( result == THINK_ACTION )
				return false;
			else
				return true;		// nothing more to do...
		}
		result = ThinkMoveToAmmo( theUnit, map, action );
		if ( result == THINK_SOLVED_NO_ACTION ) {
			if ( ThinkInventory( theUnit, map, action ) == THINK_ACTION ) {
				return false; // have ammo now!
			}
			// somethig went wrong and we're stuck.
			return true;
		}
		else if ( result == THINK_ACTION ) {
			return false;	// moving
		}
		else {
			return true;	// stuck. End turn. No point rotating. This unit will get shot.
		}
	}

	return true;
}



bool NullAI::Think( const Unit* move,
					int flags,
					TacMap* map,
					AIAction* action )
{
	action->actionID = ACTION_NONE;
	return true;	// and we're done!
}


bool CivAI::Think(	const Unit* theUnit,
					int flags,
					TacMap* map,
					AIAction* action )
{
	Vector2F sumRun = { 0, 0 };
	action->actionID = ACTION_NONE;

	// Civs wander unless they are running away from something.
	for( int i=0; i<MAX_UNITS; ++i ) {
		if (    m_enemy[i] > 0
			 && m_units[i].IsAlive() )
		{
			if ( m_visibility->UnitCanSee( theUnit, &m_units[i] ) )
			{
				Vector2I runI = theUnit->MapPos() - m_units[i].MapPos();
				Vector2F run = { (float)runI.x, (float)runI.y };
				float len = run.Length();
				GLASSERT( len > 0 );
				if ( len > 0 ) {
					run.Multiply( 1.0f / len  );
					sumRun += run;
				}
			}
		}
	}

	if ( sumRun.LengthSquared() > 0 ) {
		const float dest[2] = { 8.0f, 4.0f };

		sumRun.Normalize();

		// Try to move further, then closer. Failing that, wander.
		for( int i=0; i<2; ++i ) {
			Vector2I end32 = { theUnit->MapPos().x + LRintf( sumRun.x*dest[i] ), theUnit->MapPos().y + LRintf( sumRun.y*dest[i] ) };
			if ( map->Bounds().Contains( end32 ) ) {
				grinliz::Vector2<S16> start = { theUnit->MapPos().x, theUnit->MapPos().y };
				grinliz::Vector2<S16> end = { end32.x, end32.y };

				float cost = 0;
				int result = map->SolvePath( theUnit, start, end, &cost, &m_path[0] );

				if ( result == micropather::MicroPather::SOLVED ) {
					TrimPathToCost( &m_path[0], theUnit->TU() );
					action->actionID = ACTION_MOVE;
					action->move.path.Init( m_path[0] );
					return true;
				}
			}
		}
	}

	// Didn't run...wander.
	ThinkWander( theUnit, map, action );
	return true;	// civs are a 1-shot AI
}
