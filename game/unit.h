#ifndef UFOATTACK_SPRITE_INCLUDED
#define UFOATTACK_SPRITE_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrandom.h"
#include "../engine/enginelimits.h"

class Stream;
class Model;
class ModelResounce;
class Engine;
class Game;

class Unit
{
public:
	enum {
		STATUS_UNUSED,
		STATUS_ALIVE,
		STATUS_DEAD
	};

	enum {
		TERRAN_MARINE,
		CIVILIAN,
		ALIEN
	};

	enum {
		ALIEN_0,
		ALIEN_1,
		ALIEN_2,
		ALIEN_3
	};

	enum {
		MALE,
		FEMALE
	};

	Unit() : status( STATUS_UNUSED ), model( 0 ), game( 0 ), engine( 0 ) {}
	~Unit()	{ FreeModel(); }

	int Status()			{ return status; }
	int Team()				{ return team; }
	int AlienType()			{ return (body>>ALIEN_TYPE_SHIFT) & ALIEN_TYPE_MASK; }
	int Gender()			{ return (body>>GENDER_SHIFT) & GENDER_MASK; }
	const char* FirstName()	{ return firstName; }
	const char* LastName()	{ return lastName; }

	// only works if model is in existence!!
	void SetPos( int x, int z );

	void GenerateTerran( U32 seed );
	void GenerateCiv( U32 seed );
	void GenerateAlien( int type, U32 seed );

	void CreateModel( Game* game, Engine* engine );
	void FreeModel();
	Model* GetModel() { return model; }

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
	};

	void UpdateModel();	// make the model current with the unit status - armor, etc.

	int status;
	int team;	// terran, alien, civ
	int body;	// describes physical appearence

	const char* firstName;
	const char* lastName;

	Game* game;
	Engine* engine;
	Model* model;
};

/*
class Sprite
{
public:
	Sprite();
	~Sprite();



	void Init(	Engine* engine, const Unit
				ModelResource* resource,
				int team );

	void Save( Stream* );
	void Load( Stream* );

private:
	Model* model;
};
*/

#endif // UFOATTACK_SPRITE_INCLUDED