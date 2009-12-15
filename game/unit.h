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

	bool InUse()				{ return status != STATUS_NOT_INIT; }
	bool IsAlive()				{ return status == STATUS_ALIVE; }
	void DoDamage( const float* damage );
	void UseTU( float tu )		{ stats.UseTU( tu ); }

	int Status() const			{ return status; }
	int Team() const			{ return team; }
	int AlienType()	const		{ return (body>>ALIEN_TYPE_SHIFT) & ALIEN_TYPE_MASK; }
	int Gender() const			{ return (body>>GENDER_SHIFT) & GENDER_MASK; }
	const char* FirstName() const;
	const char* LastName() const;

	void SetMapPos( int x, int z );
	void SetMapPos( const grinliz::Vector2I& pos ) { SetMapPos( pos.x, pos.y ); }
	void SetPos( const grinliz::Vector3F& pos, float rotation );
	void SetYRotation( float rotation );
	void CalcPos( grinliz::Vector3F* ) const;

	// Compute the map pos: x,y (always int) and rotation (always multiple of 45)
	void CalcMapPos( grinliz::Vector2I*, float* rotation ) const;

	Item* GetWeapon();
	const Item* GetWeapon() const;
	Inventory* GetInventory();
	void UpdateInventory();

	Model* GetModel()						{ return model; }
	const Model* GetModel() const			{ return model; }
	Model* GetWeaponModel()					{ return weapon; }
	const Model* GetWeaponModel() const		{ return weapon; }

	// Time for the primary(0) or secondary(1) weapon to snap, auto, or aimed shot.
	// Returns 0.0 if the weapon doesn't exist or support that fire mode.
	float FireTime( int select, int type ) const;
	// Accuracy of the weapon (0 or 1) at 1 unit of range.
	// Returns 0.0 if the weapon doesn't exist or support that fire mode.
	float FireAccuracy( int select, int type ) const;

	float AngleBetween( const grinliz::Vector2I& dst, bool quantize ) const;

	const Stats& GetStats() const	{ return stats; }

	void Save( UFOStream* s ) const;
	void Load( UFOStream* s, Engine* engine, Game* game );

private:
	enum {	
		// Aliens
		ALIEN_TYPE_MASK = 0x03,
		ALIEN_TYPE_SHIFT = 0,

		// Terrans
		GENDER_MASK = 0x01,
		GENDER_SHIFT = 0,

		// Marines (also have gender)
		HAIR_MASK = 0x03,
		HAIR_SHIFT = 1,
		SKIN_MASK = 0x03,
		SKIN_SHIFT = 3,
		NAME0_MASK = 0xff,
		NAME0_SHIFT = 16,
		NAME1_MASK = 0xff,
		NAME1_SHIFT = 24,
	};

	void GenerateSoldier( U32 seed );
	void GenerateCiv( U32 seed );
	void GenerateAlien( int type, U32 seed );
	void CreateModel( bool alive );

	void UpdateModel();		// make the model current with the unit status - armor, etc.
	void UpdateWeapon();	// set the gun position

	void Kill();

	int status;
	int team;	// terran, alien, civ
	U32 body;	// describes everything! a random #

	Game*		game;
	Engine*		engine;
	Model*		model;
	Model*		weapon;
	Inventory	inventory;

	Stats stats;
};


#endif // UFOATTACK_UNIT_INCLUDED
