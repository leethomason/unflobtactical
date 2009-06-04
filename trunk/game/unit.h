#ifndef UFOATTACK_SPRITE_INCLUDED
#define UFOATTACK_SPRITE_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glvector.h"
#include "../engine/enginelimits.h"

class UFOStream;
class Model;
class ModelResource;
class Engine;
class Game;


struct ItemDef
{
	enum {
		TYPE_WEAPON,
		TYPE_ARMOR
	};

	int				type;
	const char*		name;
	ModelResource*	resource;

	void Init( int _type, const char* _name, ModelResource* _resource ) {
		GLASSERT( _resource );
		type = _type;
		name = _name;
		resource = _resource;
	}
};


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
		ALIEN_0,
		ALIEN_1,
		ALIEN_2,
		ALIEN_3
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

	bool InUse()			{ return status != STATUS_NOT_INIT; }
	bool IsAlive()			{ return status == STATUS_ALIVE; }

	int Status()			{ return status; }
	int Team()				{ return team; }
	int AlienType()			{ return (body>>ALIEN_TYPE_SHIFT) & ALIEN_TYPE_MASK; }
	int Gender()			{ return (body>>GENDER_SHIFT) & GENDER_MASK; }
	const char* FirstName();
	const char* LastName();

	void SetMapPos( int x, int z );
	void SetPos( const grinliz::Vector3F& pos, float rotation );
	void SetYRotation( float rotation );
	void CalcPos( grinliz::Vector3F* ) const;
	void CalcMapPos( grinliz::Vector2I* ) const;

	void SetWeapon( const ItemDef* itemDef );	
	Model* GetModel() { return model; }
	const Model* GetModel() const { return model; }

	float AngleBetween( const Unit* target, bool quantize ) const;

	void Save( UFOStream* s );
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
	void CreateModel();

	void UpdateModel();		// make the model current with the unit status - armor, etc.
	void UpdateWeapon();	// set the gun position

	int status;
	int team;	// terran, alien, civ
	U32 body;	// describes everything! a random #

	Game*		game;
	Engine*		engine;
	Model*		model;
	Model*		weapon;
	ItemDef*	weaponItem;	// temporary - needs inventory system
};


#endif // UFOATTACK_SPRITE_INCLUDED