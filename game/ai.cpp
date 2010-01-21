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
	// - if on storage, check if we need stuff
	// - can see nothing, move towards something the team can see. Select between
	//   current canSee and LKPs
	// - if nothing to move to, stand around

	// FIXME:
	//		check for out of ammo
	//		do something about out of ammo
	//		check for blowing up friends
	//		doesn't use explosives

	action->actionID = ACTION_NONE;
	Vector2I theUnitPos;
	theUnit->CalcMapPos( &theUnitPos, 0 );


	if ( theUnit->GetWeapon() ) {
		int best = -1;
		float bestScore = 0.0f;
		int bestMode = 0;

		const Item* weapon = theUnit->GetWeapon();
		const WeaponItemDef* wid = weapon->GetItemDef()->IsWeapon();
		GLASSERT( wid );

		for( int i=m_enemyStart; i<m_enemyEnd; ++i ) {
			if ( units[i].IsAlive() && units[i].GetModel() && targets.CanSee( theUnit, &units[i] ) ) {
	
				Vector2I pos;
				units[i].CalcMapPos( &pos, 0 );
				int len2 = (pos-theUnitPos).LengthSquared();
				float len = sqrtf( (float)len2 );
				
				for ( int mode=FIRE_MODE_START; mode<FIRE_MODE_END; ++mode ) {
					int select, type;
					wid->FireModeToType( 1, &select, &type );

					// Check for enough ammo
					int nRounds = (type == AUTO_SHOT) ? 3 : 1;
					if ( weapon->Rounds() < nRounds )
						continue;

					// Handle explosives
					// FIXME: account for explosives
					if ( wid->IsExplosive( select ) )
						continue;

					float chance, anyChance, tu, dptu;

					if ( theUnit->FireStatistics( select, type, len, &chance, &anyChance, &tu, &dptu ) ) {
						float score = dptu / (float)units[i].GetStats().HPFraction();

						if ( score > bestScore ) {
							bestScore = score;
							best = i;
							bestMode = mode;
						}
					}
				}
			}
		}
		if ( best >= 0 ) {
			action->actionID = ACTION_SHOOT;
			action->shoot.mode = bestMode;
			units[best].GetModel()->CalcTarget( &action->shoot.target );
			return false;
		}
	}

	return true;
}

