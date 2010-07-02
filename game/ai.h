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

#include <vector>

class Unit;
class Targets;
class SpaceTree;
class Map;


class AI
{
public:
	enum {
		ACTION_NONE = 0,
		ACTION_MOVE = 1,
		ACTION_SHOOT = 3,
		ACTION_SWAP_WEAPON = 4,
		ACTION_PICK_UP = 5,
	};

	struct MoveAIAction {
		MotionPath			path;
	};

	struct ShootAIAction {
		WeaponMode			mode;
		grinliz::Vector3F	target;
	};

	struct PickUpAIAction {
		enum { MAX_ITEMS = 4 };
		const ItemDef* itemDefArr[MAX_ITEMS];	// items to be picked up, in priority order.
	};

	struct AIAction {
		int actionID;
		union {
			MoveAIAction	move;
			ShootAIAction	shoot;
			PickUpAIAction	pickUp;
		};
	};

	AI( int team,			// AI in instantiated for a TEAM, not a unit
		SpaceTree* tree );	// Line of site checking

	virtual ~AI()	{}

	void StartTurn( const Unit* units, const Targets& targets );

	enum {
		AI_WANDER = 0x01,
		AI_GUARD  = 0x02
	};

	// Return true if done.
	virtual bool Think( const Unit* move,
						const Unit* units,
						const Targets& targets,
						int flags,
						Map* map,
						AIAction* action ) = 0;
protected:
	// Utility:
	bool LineOfSight( const Unit* shoot, const Unit* target );
	void TrimPathToCost( std::vector< grinliz::Vector2<S16> >* path, float maxCost );
	int  VisibleUnitsInArea(	const Unit* theUnit,
								const Unit* units,
								const Targets& targets,
								int start, int end, const grinliz::Rectangle2I& bounds );

	struct LKP {
		grinliz::Vector2I	pos;
		int					turns;
	};
	LKP m_lkp[MAX_UNITS];

	int m_team;
	int m_enemyTeam;
	int m_enemyStart;
	int m_enemyEnd;
	grinliz::Random m_random;
	SpaceTree* m_spaceTree;
	std::vector< grinliz::Vector2<S16> > m_path[4];
};


class WarriorAI : public AI
{
public:
	WarriorAI( int team, SpaceTree* tree ) : AI( team, tree )		{}
	virtual ~WarriorAI()					{}

	virtual bool Think( const Unit* move,
						const Unit* units,
						const Targets& targets,
						int flags,
						Map* map,
						AIAction* action );

};


#endif // UFO_ATTACK_AI_INCLUDED