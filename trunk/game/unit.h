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

class UFOStream;
class Model;
class ModelResource;
class Engine;
class Game;


class Unit
{
public:
	enum {
		STATUS_NOT_INIT,
		STATUS_ALIVE,
		STATUS_DEAD
	};

	enum {
		SOLDIER,
		CIVILIAN,
		ALIEN
	};

	enum {
		ALIEN_0,	// grey
		ALIEN_1,	// mind zapper
		ALIEN_2,	// trooper
		ALIEN_3		// elite
	};

	enum {
		MALE,
		FEMALE
	};

	Unit() : status( STATUS_NOT_INIT ), model( 0 ) {}
	~Unit();

	void Init(	Engine* engine, Game* game, 
				int team,	 
				int alienType,	// if alien...
				U32 seed );
	
	void Free();

	bool InUse() const			{ return status != STATUS_NOT_INIT; }
	bool IsAlive() const		{ return status == STATUS_ALIVE; }
	void DoDamage( const DamageDesc& damage );
	void UseTU( float tu )		{ stats.UseTU( tu ); }

	void NewTurn();
	void SetUserDone()			{ userDone = true; }
	bool IsUserDone()			{ return userDone; }

	int Status() const			{ return status; }
	int Team() const			{ return team; }
	int AlienType()	const		{ return GetValue( ALIEN_TYPE ); }
	int Gender() const			{ return GetValue( GENDER ); }

	const char* FirstName() const;
	const char* LastName() const;
	const char* Rank() const;

	void SetMapPos( int x, int z );
	void SetMapPos( const grinliz::Vector2I& pos ) { SetMapPos( pos.x, pos.y ); }
	void SetPos( const grinliz::Vector3F& pos, float rotation );
	void SetYRotation( float rotation );
	void CalcPos( grinliz::Vector3F* ) const;

	// Note the visibility is current (or not...) irrespective of the unit being alive.
	bool VisibilityCurrent() const				{ return visibilityCurrent; }
	void SetVisibilityCurrent( bool current)	{ visibilityCurrent = current; }
	// Compute the bounding box of this unit's sight. (VERY loose.)
	void CalcVisBounds( grinliz::Rectangle2I* b ) const;

	// Compute the map pos: x,y (always int) and rotation (always multiple of 45)
	void CalcMapPos( grinliz::Vector2I*, float* rotation ) const;

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
	float FireAccuracy( int select, int type ) const;
	// returns true if this fire mode is supported
	bool FireStatistics( int select, int type, float distance, float* chanceToHit, float* chanceAnyHit, float* tu, float* damagePerTU ) const;
	// returns true of the mode->type conversion succeeds (has the weapon, etc.)

	bool FireModeToType( int mode, int* select, int* type ) const;

	float AngleBetween( const grinliz::Vector2I& dst, bool quantize ) const;

	const Stats& GetStats() const	{ return stats; }

	void Save( UFOStream* s ) const;
	void Load( UFOStream* s, Engine* engine, Game* game );


private:
	enum {	
		ALIEN_TYPE,		// 2, 0-3. Needs to be first for GenerateAlien()
		GENDER,			// 1, 0-1
		HAIR,			// 2, 0-3
		SKIN,			// 2, 0-3
		LAST_NAME,		// 3, 0-7
		FIRST_NAME,		// 3, 0-7
	};
	void GenerateSoldier( U32 seed );
	U32 GetValue( int which ) const;	// ALIEN_TYPE, etc.
	void GenerateCiv( U32 seed );
	void GenerateAlien( int type, U32 seed );
	void CreateModel( bool alive );

	void UpdateModel();		// make the model current with the unit status - armor, etc.
	void UpdateWeapon();	// set the gun position

	void Kill();

	int status;
	int team;	// terran, alien, civ
	U32 body;	// describes everything! a random #
	bool userDone;	// ui metaphore - flag if the user is done with this unit

	Game*		game;
	Engine*		engine;
	Model*		model;
	Model*		weapon;
	Inventory	inventory;
	bool visibilityCurrent;	// if set, the visibility is current. Can be set by CalcAllVisibility()

	Stats stats;
};


#endif // UFOATTACK_UNIT_INCLUDED
