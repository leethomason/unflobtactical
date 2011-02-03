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

#ifndef UFO_ATTACK_AI_INCLUDED
#define UFO_ATTACK_AI_INCLUDED


#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrandom.h"

#include "gamelimits.h"
#include "battlescene.h"	// FIXME: for MotionPath. Factor out?
#include "battlevisibility.h"

class Unit;
class SpaceTree;
class Map;

class AI
{
public:
	enum {
		ACTION_NONE = 0,
		ACTION_MOVE = 1,
		ACTION_SHOOT = 3,
		ACTION_ROTATE = 4,
		ACTION_INVENTORY = 5,
	};

	struct MoveAIAction {
		MotionPath			path;
	};

	struct ShootAIAction {
		WeaponMode			mode;
		grinliz::Vector3F	target;
		float				targetWidth;
		float				targetHeight;
	};

	struct RotateAIAction {
		int					x, y;
	};

	struct InventoryAIAction {
		// The unit will drop all weapons and clips, and then pick up new
		// items. Effectively puts part of the AI in the Unit code, but
		// it's messy both ways.
	};

	struct AIAction {
		int actionID;
		union {
			MoveAIAction		move;
			ShootAIAction		shoot;
			RotateAIAction		rotate;
			InventoryAIAction	inventory;
		};
	};

	AI( int team,					// AI in instantiated for a TEAM, not a unit
		Visibility* visibilit,	
		Engine* engine, 			// Line of site checking
		const Unit* units );		// all the units we can scan

	virtual ~AI()	{}

	void StartTurn( const Unit* units );
	void Inform( const Unit* theUnit, int quality );

	enum {
		AI_NORMAL = 0x00,
		AI_WANDER = 0x01,
		AI_GUARD  = 0x02,
		AI_TRAVEL = 0x04,
	};

	// Return true if done.
	virtual bool Think( const Unit* move,
						int flags,
						TacMap* map,
						AIAction* action ) = 0;
protected:
	enum {
		THINK_NOT_OPTION,			// can't do this (if (no weapon) can't shoot)
		THINK_NO_ACTION,			// no action taken
		THINK_SOLVED_NO_ACTION,		// state solved for - movetoammo on ammo, for example
		THINK_ACTION				// action filled in
	};

	// if THINK_NOT_OPTION end move.
	int ThinkBase( const Unit* move );

	// THINK_NOT_OPTION no weapon / ammo
	// THINK_NO_ACTION  no target
	// THINK_ACTION		shot taken
	int ThinkShoot(			const Unit* move,
							TacMap* map,
							AIAction* action );

	// THINK_SOLVED_NO_ACTION standing on ammo
	// THINK_ACTION           move
	// THINK_NO_ACTION		  nothing found, not enough time
	int ThinkMoveToAmmo(	const Unit* theUnit,
							TacMap* map,
							AIAction* action );

	// THINK_NO_ACTION		no storage
	int ThinkInventory(		const Unit* theUnit,
							TacMap* map,
							AIAction* action);

	// THINK_ACTION			move
	// THINK_NO_ACTION		not enough time, no destination,
	int ThinkSearch(		const Unit* theUnit,
							int flags,
							TacMap* map,
							AIAction* action );

	int ThinkWander(		const Unit* theUnit,
							TacMap* map,
							AIAction* action );

	int ThinkTravel(		const Unit* theUnit,
							TacMap* map,
							AIAction* action );

	int ThinkRotate(		const Unit* theUnit,
							TacMap* map,
							AIAction* action );

	// Utility:
	bool LineOfSight( const Unit* shooter, const Unit* target ); // calls the engine LoS to get an accurate value
	void TrimPathToCost( MP_VECTOR< grinliz::Vector2<S16> >* path, float maxCost );
	int  VisibleUnitsInArea(	const Unit* theUnit,
								const Unit* units,
								const grinliz::Rectangle2I& bounds );

	struct LKP {
		grinliz::Vector2I	pos;
		int					turns;
	};
	const Unit* m_units;

	int m_team;

	Visibility* m_visibility;
	grinliz::Random m_random;
	Engine* m_engine;	// for ray queries.
	MP_VECTOR< grinliz::Vector2<S16> > m_path[4];

	enum {
		MAX_TURNS_LKP = 100
	};
	LKP					m_lkp[MAX_UNITS];			// Last Known Position of enemies.
	grinliz::Vector2I	m_travel[MAX_UNITS];		// Destination of Travel-ing AI
	int					m_thinkCount[MAX_UNITS];	// number of times Think has been called this turn. If too high, abort.
	float				m_enemy[MAX_UNITS];			// 1.0: enemy. 0.0: friend. in between, kind of malevalence
};


class WarriorAI : public AI
{
public:
	WarriorAI( int team, Visibility* vis, Engine* engine, const Unit* units ) : AI( team, vis, engine, units )		{}
	virtual ~WarriorAI()					{}

	virtual bool Think( const Unit* move,
						int flags,
						TacMap* map,
						AIAction* action );

};


class CivAI : public AI
{
public:
	CivAI( int team, Visibility* vis, Engine* engine, const Unit* units ) : AI( team, vis, engine, units )		{}
	virtual ~CivAI()																		{}
	virtual bool Think( const Unit* move,
						int flags,
						TacMap* map,
						AIAction* action );
};


class NullAI : public AI
{
public:
	NullAI( int team, Visibility* vis, Engine* engine, const Unit* units ) : AI( team, vis, engine, units )		{}
	virtual ~NullAI()																		{}
	virtual bool Think( const Unit* move,
						int flags,
						TacMap* map,
						AIAction* action );
};


#endif // UFO_ATTACK_AI_INCLUDED