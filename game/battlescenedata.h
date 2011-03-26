#ifndef UFOATTACK_BATTLESCENEDATA_INCLUDED
#define UFOATTACK_BATTLESCENEDATA_INCLUDED

class Unit;
class Storage;

class BattleSceneData : public SceneData
{
public:
	int	  seed;
	int   scenario;			// FARM_SCOUT, etc. Can be 0 to defer loading.
	bool  crash;			// is a crashed alien
	Unit* soldierUnits;		// pointer to MAX_TERRAN units
	bool  dayTime;
	float alienRank;

	Storage* storage;
};


#endif // UFOATTACK_BATTLESCENEDATA_INCLUDED