#include "ai.h"
#include "unit.h"
#include "targets.h"
#include "../engine/model.h"
	
using namespace grinliz;

AI::AI( int team )
{
	m_team = team;
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


bool WarriorAI::Think(	const Unit* theUnit,
						const Unit* units,
						const Targets& targets,
						AIAction* action )
{
	// Crazy simple 1st AI. If:
	// - theUnit can see something, shoot it. Choose the unit with the
	//	 greatest chance of going down
	// - can see nothing, move towards something the team can see. Select between
	//   current canSee and LKPs
	// - if nothing to move to, stand around
	//
	// FIXES:
	//		check for out of ammo
	//		do something about out of ammo
	//		check for blowing up friends

	action->actionID = ACTION_NONE;
	Vector2I theUnitPos;
	theUnit->CalcMapPos( &theUnitPos, 0 );

	if ( theUnit->GetWeapon() ) {
		for( int i=m_enemyStart; i<m_enemyEnd; ++i ) {
			int best = -1;
			int bestScore = INT_MAX;

			// First pass, just check out all the targets and choose the best.
			if ( units[i].IsAlive() && units[best].GetModel() && targets.CanSee( theUnit, &units[i] ) ) {
				Vector2I pos;
				units[i].CalcMapPos( &pos, 0 );
				
				// the score gets worse with distance2 and hp
				int len2 = (pos.x-theUnitPos.x)*(pos.x-theUnitPos.x) + (pos.y-theUnitPos.y)*(pos.y-theUnitPos.y);
				int score = len2 * units[i].GetStats().HP();

				if ( score < bestScore ) {
					bestScore = score;
					best = i;
				}
			}

			int fireMode = -1;
			if ( best >= 0 ) {
				const WeaponItemDef* wid = theUnit->GetWeapon()->GetItemDef()->IsWeapon();
				Vector2I pos;
				units[best].CalcMapPos( &pos, 0 );

				int len2 = (pos.x-theUnitPos.x)*(pos.x-theUnitPos.x) + (pos.y-theUnitPos.y)*(pos.y-theUnitPos.y);
				float len = sqrt( (float)len2 );
				
				float bestDPTU = 0.0f;

				for( int i=0; i<3; ++i ) {
					int select, type;
					float chance, anyChance, tu, dptu;

					wid->FireModeToType( 1, &select, &type );
					theUnit->FireStatistics( select, type, len, &chance, &anyChance, &tu, &dptu );
					bool consider = false;

					if ( tu > 0 && tu <= theUnit->GetStats().TU() ) {
						if ( wid->weapon[select].flags & WEAPON_EXPLOSIVE ) {
							if ( len >= 5.0f ) 
								consider = true;
						}
						else { 
							consider = true;
						}
					}
					if ( consider && dptu > bestDPTU ) {
						fireMode = i;
					}
				}
			}
			if ( fireMode >= 0 ) {
				action->actionID = ACTION_SHOOT;
				action->shoot.mode = fireMode;
				units[best].GetModel()->CalcTarget( &action->shoot.target );
			}
		}
	}



	return true;
}

