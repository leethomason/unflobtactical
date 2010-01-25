#ifndef UFO_ATTACK_AI_INCLUDED
#define UFO_ATTACK_AI_INCLUDED


#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"

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
		int					mode;	// 0, 1, 2
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

	// called by Scene
	void StartTurn( const Unit* units, const Targets& targets );

	// Return true if done.
	virtual bool Think( const Unit* move,
						const Unit* units,
						const Targets& targets,
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
						Map* map,
						AIAction* action );

};


#endif // UFO_ATTACK_AI_INCLUDED