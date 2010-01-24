#include "ai.h"
#include "unit.h"
#include "targets.h"
#include "../engine/model.h"
#include "../engine/loosequadtree.h"
#include "../engine/map.h"
#include "../grinliz/glperformance.h"

#include <float.h>

using namespace grinliz;

#define AILOG GLOUTPUT

AI::AI( int team, SpaceTree* tree )
{
	m_team = team;
	m_spaceTree = tree;

	if ( m_team == Unit::ALIEN) {
		m_enemyTeam  = Unit::SOLDIER;
		m_enemyStart = TERRAN_UNITS_START;
		m_enemyEnd   = TERRAN_UNITS_END;
	}
	else {
		m_enemyTeam  = Unit::ALIEN;
		m_enemyStart = ALIEN_UNITS_START;
		m_enemyEnd   = ALIEN_UNITS_END;
	}

	for( int i=0; i<MAX_UNITS; ++i ) {
		m_lkp[i].pos.Set( -1, -1 );
		m_lkp[i].turns = 1;
	}
}


void AI::StartTurn( const Unit* units, const Targets& targets )
{
	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( targets.TeamCanSee( m_team, i ) ) {
			units[i].CalcMapPos( &m_lkp[i].pos, 0 );
			m_lkp[i].turns = 0;
		}
		else {
			m_lkp[i].turns++;
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
	const Model* ignore[5] = { shooter->GetModel(), shooter->GetWeaponModel(), target->GetModel(), target->GetWeaponModel(), 0 };

	Model* m = m_spaceTree->QueryRay( p0, (p1-p0), 0, 0, ignore, TEST_TRI, &intersection );
	if ( !m ) {
		return true;
	}
	if ( (intersection - p0).LengthSquared() > (p1-p0).LengthSquared() ) {
		return true;
	}
	return false;
}


void AI::TrimPathToCost( std::vector< grinliz::Vector2<S16> >* path, float maxCost )
{
	float cost = 0.0f;

	for ( unsigned i=1; i<path->size(); ++i ) {
		const Vector2<S16>& p0 = path->at( i-1 );
		const Vector2<S16>& p1 = path->at( i );
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



bool WarriorAI::Think(	const Unit* theUnit,
						const Unit* units,
						const Targets& targets,
						Map* map,
						AIAction* action )
{
	// QuickProfile qp( "WarriorAI::Think()" );
	// Crazy simple 1st AI. If:
	// - theUnit can see something, shoot it. Choose the unit with the
	//	 greatest chance of going down
	// - if on storage, check if we need stuff
	// - can see nothing, move towards something the team can see. Select between
	//   current canSee and LKPs
	// - if nothing to move to, stand around

	// FIXME:
	//		check for out of ammo
	//		do something about out of ammo
	//		check for blowing up friends

	const float MINIMUM_FIRE_CHANCE			= 0.02f;	// A shot is only valid if it has this chance of hitting.
	const int   EXPLOSION_ZONE				= 2;		// radius to check of clusters of enemies to blow up
	const float	MINIMUM_EXPLOSIVE_RANGE		= 4.0f;

	action->actionID = ACTION_NONE;
	Vector2I theUnitPos;
	theUnit->CalcMapPos( &theUnitPos, 0 );

	AILOG(( "Think unit=%d\n", theUnit - units ));

	// -------- Shoot -------- //
	if ( theUnit->GetWeapon() ) {
		AILOG(( "  Shoot\n" ));

		int best = -1;
		float bestScore = 0.0f;
		int bestMode = 0;
		float bestChance = 0.0f;

		const Item* weapon = theUnit->GetWeapon();
		const WeaponItemDef* wid = weapon->GetItemDef()->IsWeapon();
		GLASSERT( wid );

		for( int i=m_enemyStart; i<m_enemyEnd; ++i ) {
			if (    units[i].IsAlive() 
				 && units[i].GetModel() 
				 && targets.CanSee( theUnit, &units[i] )
				 && LineOfSight( theUnit, &units[i] ) ) 
			{
				Vector2I pos;
				units[i].CalcMapPos( &pos, 0 );
				int len2 = (pos-theUnitPos).LengthSquared();
				float len = sqrtf( (float)len2 );
				
				for ( int mode=FIRE_MODE_START; mode<FIRE_MODE_END; ++mode ) {
					int select, type;
					wid->FireModeToType( mode, &select, &type );
					if ( theUnit->CanFire( select, type ) ) {
						float chance, anyChance, tu, dptu;

						if ( theUnit->FireStatistics( select, type, len, &chance, &anyChance, &tu, &dptu ) ) {
							float score = dptu;	// Interesting: good AI, but results in odd choices.
												// FIXME: add back in, less of a factor / (float)units[i].GetStats().HPFraction();

							if ( wid->IsExplosive( select ) ) {
								if ( len < MINIMUM_EXPLOSIVE_RANGE ) {
									score = 0.0f;
								}
								else {
									Rectangle2I bounds;
									bounds.Set( pos.x-EXPLOSION_ZONE, pos.y-EXPLOSION_ZONE,
												pos.x+EXPLOSION_ZONE, pos.y+EXPLOSION_ZONE );
									int count = VisibleUnitsInArea( theUnit, units, targets, m_enemyStart, m_enemyEnd, bounds );

									if ( count <= 1 )
										score *= 0.5f;
									else
										score *= (float)count;
								}
							}

							AILOG(( "    target %2d score=%.3f s/t=%d %d chance=%.3f dptu=%.3f\n",
								    i, score, select, type, chance, dptu ));

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
			AILOG(( "  **Shooting at %d mode=%d bestScore=%.3f\n", best, bestMode, bestScore ));
			action->actionID = ACTION_SHOOT;
			action->shoot.mode = bestMode;
			units[best].GetModel()->CalcTarget( &action->shoot.target );
			return false;
		}
	}


	// --------- Weapon Management --------- //
	// The actual logic of when to swap is tricky. By reaching here, we didn't shoot, but
	// that could be for many reasons. However, if the primary fire mode is out of rounds,
	// then swap it if it will help.
	const Inventory* inventory = theUnit->GetInventory();	
	const Item* primaryWeapon = inventory->ArmedWeapon();
	const Item* secondaryWeapon = inventory->SecondaryWeapon();

	int primaryRounds   = 0;
	int secondaryRounds = 0;
	if ( primaryWeapon )   primaryRounds   = inventory->CalcClipRoundsTotal( primaryWeapon->ClipType( 0 ) );	
	if ( secondaryWeapon ) secondaryRounds = inventory->CalcClipRoundsTotal( secondaryWeapon->ClipType( 0 ) );	

	if ( !primaryRounds && secondaryRounds ) {
		AILOG(( "  **Swapping primary and secondary weapons.\n" ));
		action->actionID = ACTION_SWAP_WEAPON;
		return false;
	}

	// -------- Use & Find Storage --------- //
	/*
	if ( !primaryRounds ) {
		AILOG(( "  Out of Ammo\n" ));
		
		const Storage* storage = map->GetStorage( theUnitPos.x, theUnitPos.y );
		if ( storage ) {
	}
	*/

	// -------- Move -------- //
	// Move closer with the intent of shooting something. Unit visibility change will cause AI
	// to be re-invoked, so there isn't a concern about getting too close.
	{
		AILOG(( "  Move Stategic\n" ));
		int best = -1;
		float bestGolfScore = FLT_MAX;

		for( int i=m_enemyStart; i<m_enemyEnd; ++i ) {
			if (    units[i].IsAlive() 
				 && units[i].GetModel() ) 
			{
				// FIXME: consider using the pather for "close" if the fast distance
				// gives strange answers.
				int len2 = (theUnitPos-m_lkp[i].pos).LengthSquared();
				if ( len2 > 0 ) {
					float len = sqrtf( (float)len2 );

					// The older the data, the worse the score.
					float score = len + (float)(m_lkp[i].turns)*NORMAL_TU;
					
					if ( score < bestGolfScore ) {
						bestGolfScore = score;
						best = i;
					}
				}
			}
		}
		if ( best >= 0 ) {
			Vector2<S16> start = { theUnitPos.x, theUnitPos.y };
			Vector2<S16> end   = { m_lkp[best].pos.x, m_lkp[best].pos.y };

			// Reserve for auto or snap??
			float tu = theUnit->GetStats().TU();

			int select1, type1, select0, type0;
			theUnit->FireModeToType( AUTO_SHOT, &select1, &type1 );
			theUnit->FireModeToType( SNAP_SHOT, &select0, &type0 );

			if ( theUnit->CanFire( select1, type1 ) ) {
				tu -= theUnit->FireTimeUnits( select1, type1 );
			}
			else if ( theUnit->CanFire( select0, type0 ) ) {
				tu -= theUnit->FireTimeUnits( select0, type0 );
			}

			float lowCost = FLT_MAX;
			int bestPath = -1;
			const Vector2<S16> delta[4] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
			for( int i=0; i<4; ++i ) {
				// The path is blocked *by our target*. Fooling around with how the map pather
				// works is tweaky. So just check 4 spots.
				float cost;
				int result = map->SolvePath( start, end+delta[i], &cost, &m_path[i] );
				if ( result == micropather::MicroPather::SOLVED ) {
					if ( cost < lowCost ) {
						lowCost = cost;
						bestPath = i;
					}
				}
			}
			if ( bestPath >= 0 ) {
				// stupid double array syntax...
				std::vector< grinliz::Vector2<S16> >& path = m_path[bestPath];

				TrimPathToCost( &path, tu );

				if ( path.size() > 1 ) {
					AILOG(( "  **Moving to (%d,%d)\n", path[path.size()-1].x, path[path.size()-1].y ));
					action->actionID = ACTION_MOVE;
					action->move.path.Init( path );
					return false;
				}
			}
		}
	}

	return true;
}

