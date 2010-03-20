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

#ifndef UFOATTACK_UNIT_INCLUDED
#define UFOATTACK_UNIT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glbitarray.h"
#include "../engine/enginelimits.h"

#include "stats.h"
#include "inventory.h"
#include "gamelimits.h"

class Model;
class ModelResource;
class Engine;
class Game;
class Map;


class Unit
{
public:
	enum {
		STATUS_NOT_INIT,
		STATUS_ALIVE,
		STATUS_DEAD
	};

	enum {
		ALIEN_0,	// grey
		ALIEN_1,	// mind zapper
		ALIEN_2,	// trooper
		ALIEN_3,	// elite
	};

	enum { 
		AI_NORMAL,
		AI_GUARD
	};

	enum {
		MALE,
		FEMALE
	};

	Unit() : status( STATUS_NOT_INIT ), model( 0 ) {}
	~Unit();

	// Note that the 'stats' should be set before init.
	// Load calls init automatically
	void Init(	Engine* engine, Game* game, 
				int team,
				int status,
				int alienType,
				int seed );
	
	void Free();

	bool InUse() const			{ return status != STATUS_NOT_INIT; }
	bool IsAlive() const		{ return status == STATUS_ALIVE; }
	// Do damage to this unit. Will create a Storage on the map, if the map is provided.
	void DoDamage( const DamageDesc& damage, Map* map );
	void UseTU( float tu )		{ stats.UseTU( tu ); }

	void NewTurn();
	void SetUserDone()			{ userDone = true; }
	bool IsUserDone()			{ return userDone; }

	int Status() const			{ return status; }
	int Team() const			{ return team; }
	int AlienType()	const		{ return type; }
	int Gender() const			{ return GetValue( GENDER ); }
	int AI() const				{ return ai; }

	const char* FirstName() const;
	const char* LastName() const;
	const char* Rank() const;

	void SetMapPos( int x, int z );
	void SetMapPos( const grinliz::Vector2I& pos ) { SetMapPos( pos.x, pos.y ); }
	void SetPos( const grinliz::Vector3F& pos, float rotation );
	void SetYRotation( float rotation );
	void CalcPos( grinliz::Vector3F* ) const;

	// Compute the bounding box of this unit's sight. (VERY loose.)
	void CalcVisBounds( grinliz::Rectangle2I* b ) const;

	// Compute the map pos: x,y (always int) and rotation (always multiple of 45)
	void CalcMapPos( grinliz::Vector2I*, float* rotation ) const;
	// Convenience to CalcMapPos
	grinliz::Vector2I Pos() const {
		grinliz::Vector2I p;
		CalcMapPos( &p, 0 );
		return p;
	}

	Item* GetWeapon();
	const Item* GetWeapon() const;
	Inventory* GetInventory();
	const Inventory* GetInventory() const;
	void UpdateInventory();

	Model* GetModel()						{ return model; }
	const Model* GetModel() const			{ return model; }
	Model* GetWeaponModel()					{ return weapon; }
	const Model* GetWeaponModel() const		{ return weapon; }

	// Returns true if the mode can be used: mode is supported, enough time units,
	// enough rounds, etc.
	bool CanFire( int select, int type ) const;
	// Time for the primary(0) or secondary(1) weapon to snap, auto, or aimed shot.
	float FireTimeUnits( int select, int type ) const;
	// Accuracy of the weapon (0 or 1) at 1 unit of range.
	// Returns 0.0 if the weapon doesn't exist or support that fire mode.
	//float FireAccuracy( int select, int type ) const;
	// returns true if this fire mode is supported
	bool FireStatistics( int select, int type, float distance, float* chanceToHit, float* chanceAnyHit, float* tu, float* damagePerTU ) const;
	// returns true of the mode->type conversion succeeds (has the weapon, etc.)

	bool FireModeToType( int mode, int* select, int* type ) const;

	float AngleBetween( const grinliz::Vector2I& dst, bool quantize ) const;

	const Stats& GetStats() const	{ return stats; }
	static void GenStats( int team, int type, int seed, Stats* stats );

	void Save( TiXmlElement* doc ) const;
	void Load( const TiXmlElement* doc, Engine* engine, Game* game );

private:
	// WARNING: change this then change GetValue()
	enum {	
		GENDER,			// 1, 0-1
		HAIR,			// 2, 0-3
		SKIN,			// 2, 0-3
		LAST_NAME,		// 4, 0-15
		FIRST_NAME,		// 4, 0-15
	};
	U32 GetValue( int which ) const;	// ALIEN_TYPE, etc.

//	void GenerateSoldier( U32 seed );
//	void GenerateCiv( U32 seed );
//	void GenerateAlien( U32 seed, int type );
	void CreateModel();

	void UpdateModel();		// make the model current with the unit status - armor, etc.
	void UpdateWeapon();	// set the gun position

	void Kill( Map* map );

	int status;
	int ai;			// normal or guard
	int team;		// terran, alien, civ
	int type;		// type of alien
	U32 body;		// describes everything! a random #
	bool userDone;	// ui metaphore - flag if the user is done with this unit

	Game*		game;
	Engine*		engine;
	Model*		model;
	Model*		weapon;
	bool		visibilityCurrent;	// if set, the visibility is current. Can be set by CalcAllVisibility()

	Inventory	inventory;
	Stats		stats;
};


#endif // UFOATTACK_UNIT_INCLUDED
