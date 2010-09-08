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

#include <float.h>

using namespace grinliz;


AI::AI( int team, Engine* engine, const Unit* units )
{
	m_team = team;
	m_engine = engine;
	m_units = units;

	if ( m_team == ALIEN_TEAM) {
		m_enemyTeam  = TERRAN_TEAM;
		m_enemyStart = TERRAN_UNITS_START;
		m_enemyEnd   = TERRAN_UNITS_END;
	}
	else {
		m_enemyTeam  = ALIEN_TEAM;
		m_enemyStart = ALIEN_UNITS_START;
		m_enemyEnd   = ALIEN_UNITS_END;
	}

	for( int i=0; i<MAX_UNITS; ++i ) {
		m_lkp[i].pos.Set( 0, 0 );
		m_lkp[i].turns = MAX_TURNS_LKP;
	}
}


void AI::StartTurn( const Unit* units, const Targets& targets )
{
	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( units[i].IsAlive() ) {
			if ( targets.TeamCanSee( m_team, i ) ) {
				m_lkp[i].pos = units[i].Pos();
				m_lkp[i].turns = 0;
			}
			else {
				m_lkp[i].turns++;
			}
		}
	}
}


void AI::Inform( const Unit* theUnit, int quality )
{
	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( m_units[i].Team() != theUnit->Team() ) {
			if ( m_lkp[i].turns > quality ) {
				m_lkp[i].turns = quality;		// Correct quality? 2 turns back for shooting?
				m_lkp[i].pos = theUnit->Pos();
			}
		}
	}
}

bool AI::LineOfSight( const Unit* shooter, const Unit* target )
{
	GLASSERT( shooter->GetModel() );
	GLASSERT( target->GetModel() );
	GLASSERT( shooter != target );

	Vector3F p0, p1, intersection;
	shooter->GetModel()->CalcTrigger( &p0 );
	target->GetModel()->CalcTarget( &p1 );

	Ray ray;
	ray.origin = p0;
	ray.direction = p1 - p0;

	const Model* ignore[3] = { shooter->GetModel(), shooter->GetWeaponModel(), 0 };
	Model* m = m_engine->IntersectModel( ray, TEST_TRI, 0, 0, ignore, &intersection );
	
	if ( m == target->GetModel() || m == target->GetWeaponModel() ) {
		return true;
	}
	return false;
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
							const Targets& targets,
							int start, int end, const grinliz::Rectangle2I& bounds )
{
	int count = 0;
	for( int i=start; i<end; ++i ) {
		if ( targets.TeamCanSee( theUnit->Team(), &units[i] ) ) {
			Vector2I p;
			units[i].CalcMapPos( &p, 0 );
			if ( bounds.Contains( p ) )
				++count;
		}
	}
	return count;
}


int AI::ThinkShoot(  const Unit* theUnit,
					  const Targets& targets,
					  Map* map,
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

	for( int i=m_enemyStart; i<m_enemyEnd; ++i ) {
		if (    m_units[i].IsAlive() 
			 && m_units[i].GetModel() 
			 && targets.CanSee( theUnit, &m_units[i] )
			 && LineOfSight( theUnit, &m_units[i]))
		{
			int len2 = (m_units[i].Pos()-theUnit->Pos()).LengthSquared();
			float len = sqrtf( (float)len2 );

			for ( int _mode=0; _mode<3; ++_mode ) {
				WeaponMode mode = (WeaponMode)_mode;

				if ( theUnit->CanFire( mode ) ) {
					float chance, anyChance, tu, dptu;

					if ( theUnit->FireStatistics( (WeaponMode)mode, len, &chance, &anyChance, &tu, &dptu ) ) {
						float score = dptu;	// Interesting: good AI, but results in odd choices.
											// FIXME: add back in, less of a factor / (float)units[i].GetStats().HPFraction();

						if ( wid->IsExplosive( (WeaponMode)mode ) ) {
							if ( len < MINIMUM_EXPLOSIVE_RANGE ) {
								score = 0.0f;
							}
							else {
								Rectangle2I bounds;
								bounds.min = bounds.max = theUnit->Pos();
								bounds.Outset( EXPLOSION_ZONE );
								
								int count = VisibleUnitsInArea( theUnit, m_units, targets, m_enemyStart, m_enemyEnd, bounds );
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
		action->shoot.mode = (WeaponMode)bestMode;
		m_units[best].GetModel()->CalcTarget( &action->shoot.target );
		return THINK_ACTION;
	}
	return THINK_NO_ACTION;
}


int AI::ThinkMoveToAmmo(	const Unit* theUnit,
							const Targets& targets,
							Map* map,
							AIAction* action )
{
	// Is theUnit already standing on the Storage? If so, use!
	Vector2I theUnitPos = theUnit->Pos();
	const Storage* storage = map->GetStorage( theUnitPos.x, theUnitPos.y );
	
	if ( storage && storage->IsResupply( theUnit->GetWeaponDef() )) {
		return THINK_SOLVED_NO_ACTION;
	}

	// Need to find Storage and go there.
	Vector2I storeLocs[8];
	int nFound = 0;
	map->FindStorage( theUnit->GetWeaponDef(), 4, storeLocs, &nFound );

	float lowCost = FLT_MAX;
	int bestPath = -1;
			
	Vector2<S16> start = { theUnitPos.x, theUnitPos.y };
			
	for( int i=0; i<nFound; ++i ) {
		Vector2<S16> end = { storeLocs[i].x, storeLocs[i].y };
		float cost;
		map->SolvePath( theUnit, start, end, &cost, &m_path[i] );

		if ( cost < lowCost ) {
			lowCost = cost;
			bestPath = i;
		}
	}
	if ( bestPath >= 0 ) {
		MP_VECTOR< grinliz::Vector2<S16> >& path = m_path[bestPath];
		TrimPathToCost( &path, theUnit->TU() );
		if ( path.size() > 1 ) {
			action->actionID = ACTION_MOVE;
			action->move.path.Init( path );
			return THINK_ACTION;
		}
	}
	return THINK_NO_ACTION;
}


int AI::ThinkInventory(	const Unit* theUnit, Map* map, AIAction* action )
{
	Vector2I pos = theUnit->Pos();

	// Drop all the weapons, and pick up new ones.
	const Storage* storage = map->GetStorage( pos.x, pos.y );

	if (	  theUnit->HasGunAndAmmo( false )									// all good, just need to re-distribute
		 || ( storage && storage->IsResupply( theUnit->GetWeaponDef() ) ) )		// can pick up new stuff
	{
		action->actionID = ACTION_INVENTORY;
		return THINK_ACTION;
	}
	return THINK_NO_ACTION;
}


int AI::ThinkSearch(const Unit* theUnit,
					const Targets& targets,
					int flags,
					Map* map,
					AIAction* action )
{
	int best = -1;
	float bestGolfScore = FLT_MAX;

	// Reserve for auto or snap??
	float tu = theUnit->TU();

	if ( theUnit->CanFire( kAutoFireMode ) ) {
		tu -= theUnit->FireTimeUnits( kAutoFireMode );
	}
	else if ( theUnit->CanFire( kSnapFireMode ) ) {
		tu -= theUnit->FireTimeUnits( kSnapFireMode );
	}

	// TU is adjusted for weapon time. If we can't effectively move any more, stop now.
	if ( tu < 1.8f ) {
		return THINK_NO_ACTION;
	}

	for( int i=m_enemyStart; i<m_enemyEnd; ++i ) {
		if (    m_units[i].IsAlive() 
			 && m_units[i].GetModel()
			 && m_lkp[i].turns < MAX_TURNS_LKP ) 
		{
			int len2 = (theUnit->Pos()-m_lkp[i].pos).LengthSquared();

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
			if ( ( flags & AI_GUARD ) && !targets.CanSee( theUnit, &m_units[i] ) ) {
				score = FLT_MAX;
			}
					
			if ( score < bestGolfScore ) {
				bestGolfScore = score;
				best = i;
			}
		}
	}
	if ( best >= 0 ) {
		Vector2<S16> start = { theUnit->Pos().x, theUnit->Pos().y };
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


int AI::ThinkWander(const Unit* theUnit,
					const Targets& targets,
					Map* map,
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
		Vector2I pos = theUnit->Pos();
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


int AI::ThinkRotate(const Unit* theUnit,
					const Targets& targets,
					Map* map,
					AIAction* action )
{
	int best = -1;
	float bestGolfScore = FLT_MAX;

	for( int i=m_enemyStart; i<m_enemyEnd; ++i ) {
		if (    m_units[i].IsAlive() 
			 && m_units[i].GetModel()
			 && m_lkp[i].turns < MAX_TURNS_LKP ) 
		{
			int len2 = (theUnit->Pos() - m_lkp[i].pos).LengthSquared();

			// If the enemy isn't within some reasonable shoot range, doesn't matter.
			// go with 1.4*max sight
			if ( len2 > MAX_EYESIGHT_RANGE*MAX_EYESIGHT_RANGE*2 )
				continue;

			float len = sqrtf( (float)len2 );

			// The older the data, the worse the score.
			const float NORMAL_TU = (float)(MIN_TU + MAX_TU) * 0.5f;
			float score = len + (float)(m_lkp[i].turns)*NORMAL_TU;
					
			if ( score < bestGolfScore ) {
				bestGolfScore = score;
				best = i;
			}
		}
	}
	if ( best >= 0 ) {
		float angle = atan2f( (float)(m_units[best].Pos().y - theUnit->Pos().y), (float)(m_units[best].Pos().x - theUnit->Pos().x) );
		action->actionID = ACTION_ROTATE;
		action->rotate.x = m_units[best].Pos().x;
		action->rotate.y = m_units[best].Pos().y;
		return THINK_ACTION;
	}
	return THINK_NO_ACTION;
}


bool WarriorAI::Think(	const Unit* theUnit,
						const Targets& targets,
						int flags,
						Map* map,
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

	const float CLOSE_ENOUGH				= 4.5f;		// don't close closer than this...

	action->actionID = ACTION_NONE;
	Vector2I theUnitPos;
	theUnit->CalcMapPos( &theUnitPos, 0 );
	float theUnitNearTarget = FLT_MAX;
	
	int result = 0;

	// -------- Shoot -------- //
	if ( theUnit->HasGunAndAmmo( true ) ) {
		if ( ThinkShoot( theUnit, targets, map, action ) == THINK_ACTION )
			return false;	// not done - can shoot again!

		// Generally speaking, only move if not doing shooting first.
		if ( theUnit->TU() > theUnit->GetStats().TotalTU() * 0.9f ) {
			if ( ThinkSearch( theUnit, targets, flags, map, action ) == THINK_ACTION )
				return false;	// still will wander & rotate
		}

		if ( theUnit->TU() == theUnit->GetStats().TotalTU() ) {
			if ( ThinkWander( theUnit, targets, map, action ) == THINK_ACTION )
				return false;	// will still rotate
		}
		ThinkRotate( theUnit, targets, map, action );
		return true;
	}
	else {
		AI_LOG(( "[ai.warrior] Unit %d Out of Ammo.\n", theUnit - m_units ));
		// Out of ammo. Get Ammo!
		if ( theUnit->HasGunAndAmmo( false ) ) {
			result = ThinkInventory( theUnit, map, action );
			GLASSERT( result == THINK_ACTION );
			return false;
		}
		result = ThinkMoveToAmmo( theUnit, targets, map, action );
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

