#ifndef BATTLE_VISIBILITY_INCLUDED
#define BATTLE_VISIBILITY_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glbitarray.h"
#include "../grinliz/glvector.h"

#include "gamelimits.h"

class BattleScene;
class Unit;
class Map;

// Groups all the visibility code together. In the battlescene itself, visibility quickly
// becomes difficult to track. 'Visibility' groups it all together and does the minimum
// amount of computation.
class Visibility {
public:
	Visibility();
	~Visibility()					{}

	void Init( BattleScene* bs, const Unit* u, Map* m )	{ battleScene = bs; this->units = u; map = m; }

	void InvalidateAll();
	// The 'bounds' reflect the area that is invalid, not the visibility of the units.
	void InvalidateAll( const grinliz::Rectangle2I& bounds );
	void InvalidateUnit( int i );

	bool TeamCanSee( int team, int x, int y );	//< Can anyone on the 'team' see the location (x,y)
	bool TeamCanSee( int team, const grinliz::Vector2I& pos )	{ return TeamCanSee( team, pos.x, pos.y ); }
	int NumTeamCanSee( int viewer, int viewee );

	bool UnitCanSee( int unit, int x, int y );
	bool UnitCanSee( const Unit* src, const Unit* target ); 
	
	void CalcVisMap( grinliz::BitArray<MAX_UNITS, MAX_UNITS, 1>* canSeeMap );

	// returs the current state of the FoW bit - and clears it!
	bool FogCheckAndClear()	{ bool result = fogInvalid; fogInvalid = false; return result; }

private:
	void CalcUnitVisibility( int unitID );
	void CalcVisibilityRay( int unitID, const grinliz::Vector2I& pos, const grinliz::Vector2I& origin );
	void CalcTeam( int team, int* start, int* end );

	BattleScene*	battleScene;
	const Unit*		units;
	Map*			map;
	bool			fogInvalid;

	bool			current[MAX_UNITS];	//< Is the visibility current? Triggers CalcUnitVisibility if not.

	grinliz::BitArray< MAP_SIZE, MAP_SIZE, MAX_UNITS >	visibilityMap;
	grinliz::BitArray< MAP_SIZE, MAP_SIZE, 1 >			visibilityProcessed;		// temporary - used in vis calc.
};

#endif // BATTLE_VISIBILITY_INCLUDED
