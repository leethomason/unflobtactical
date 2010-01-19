#ifndef UFOATTACK_ITEM_INCLUDED
#define UFOATTACK_ITEM_INCLUDED

#include <string.h>
#include "../grinliz/glvector.h"
#include "../engine/ufoutil.h"
#include "../engine/enginelimits.h"
#include "gamelimits.h"

class ModelResource;
class UFOStream;
class Engine;
class Game;
class ParticleSystem;

enum {
	// Clip types - plenty of complexity here already.
	ITEM_CLIP_SHELL = 1,
	ITEM_CLIP_AUTO,
	ITEM_CLIP_PLASMA,
	ITEM_CLIP_TACHYON,
	ITEM_CLIP_FLAME,
//	ITEM_CLIP_ROCKET,
	ITEM_CLIP_GRENADE,

	WEAPON_AUTO		 = 0x01,
	WEAPON_MELEE	 = 0x02,
	WEAPON_EXPLOSIVE = 0x04,	// only needed for cell weapons - adds a "feature" to the cell clip
};

class WeaponItemDef;
class ClipItemDef;
class ArmorItemDef;

struct DamageDesc
{
	float kinetic;
	float energy;
	float incind;

	float Total() const	{ return kinetic + energy + incind; }

	void Clear()	{ kinetic = energy = incind = 0.0f; }
	void Set( float k, float e, float i )	{ kinetic = k; energy = e; incind = i; }
	void Scale( float x ) {
		kinetic *= x;
		energy *= x;
		incind *= x;
	}
};


class ItemDef
{
public:
	ItemDef() {};
	virtual ~ItemDef() {};

	//int				type;
	const char*		name;
	const char*		desc;
	int				deco;
	const ModelResource*	resource;	// can be null, in which case render with crate.

	void InitBase( /*int type, */ const char* name, const char* desc, int deco, const ModelResource* resource, int index )
	{
		//GLASSERT( type >= ITEM_WEAPON && type <= ITEM_GENERAL );
		//this->type = type; 
		this->name = name; 
		this->desc = desc; 
		this->deco = deco; 
		this->resource = resource; 
		this->index = index;
	}

	virtual const WeaponItemDef* IsWeapon() const { return 0; }
	virtual const ClipItemDef* IsClip() const  { return 0; }
	virtual int Rounds() const { return 1; }
	virtual const ArmorItemDef* IsArmor() const { return 0; }

	// optimization trickiness:
	int index;
};


class WeaponItemDef : public ItemDef
{
public:
	virtual const WeaponItemDef* IsWeapon() const { return this; }

	struct Weapon {
		int clipType;		// type of clip required (ITEM_CLIP_ROCKET for example). 0 for melee.
		int flags;			// WEAPON_AUTO, etc.
		float damage;		// damage done by weapon, 1.0 is normal
		float accuracy;		// 1.0 is average
	};

	const char*	fireDesc[3];	// name of the 3 fire modes
	float		speed;			// 1.0 is normal speed (and weight)
	Weapon		weapon[2];		// primary and secondary

	bool HasWeapon( int select ) const		{ GLASSERT( select == 0 || select == 1 ); return weapon[select].damage > 0; }

	bool SupportsType( int select, int type ) const;
	void FireModeToType( int mode, int* select, int* type ) const;

	bool Melee() const			{ return weapon[0].flags & WEAPON_MELEE ? true : false; }
	bool IsAlienBlaster() const	{ return weapon[0].clipType == ITEM_CLIP_PLASMA || weapon[0].clipType == ITEM_CLIP_TACHYON; }

	void RenderWeapon(	int select,
						ParticleSystem*,
						const grinliz::Vector3F& p0, 
						const grinliz::Vector3F& p1,
						bool impact,
						U32 currentTime,
						U32* doneTime ) const;

	bool CompatibleClip( const ItemDef* itemDef, int* which ) const;
	
	// Basic damage for this weapon.
	void DamageBase( int select, DamageDesc* damageArray ) const;
	// Amount of time it takes to use this weapon. (Does not depend on the Unit.)
	float TimeUnits( int select, int type ) const;
	// Accuracy base - modified by the unit.
	float AccuracyBase( int select, int type ) const;
	// Statistics for this weapon. 
	void FireStatistics( int select, int type, float shooterAccuracy, float distance, 
						 float* chanceToHit,	// chance a round hits
						 float* chanceAnyHit,	// chance any round (of 3) hits
						 float* totalDamage, 
						 float* damagePerTU ) const;

};


class ClipItemDef : public ItemDef
{
public:

	virtual const ClipItemDef* IsClip() const { return this; }
	virtual int Rounds() const { return defaultRounds; }

	bool IsAlien() const { return type == ITEM_CLIP_PLASMA || type == ITEM_CLIP_TACHYON; }

	int	type;
	int defaultRounds;
};


class ArmorItemDef : public ItemDef
{
public:
	virtual const ArmorItemDef* IsArmor() const { return this; }
};


/* POD.
   An Item is made up of up to 3 parts.
   The simple case:
   ARMOR is one part.

   More complex,
   AR-2 is the main part.
   AUTOCLIP is part[1]
   GRENADE is part[2]

   part[0] is what the Item "is" - gun, medket, etc.
*/
class Item
{
public:
//	enum {
//		NUM_PARTS = 3
//	};

	Item()								{ rounds = 0; itemDef = 0; }
	Item( const Item& rhs )				{ rounds = rhs.rounds; itemDef = rhs.itemDef; }
	Item( const ItemDef* itemDef, int rounds=1 );
	Item( Game*, const char* name, int rounds=1 );

	void operator=( const Item& rhs )	{ rounds = rhs.rounds; itemDef = rhs.itemDef; }

	// Add rounds to this Item. Returns 'true' if the items combine, and 'consumed' is set
	// true if the added item (a clip) is fully used up.
	//bool Combine( Item* withThis, bool* consumed );
	// Works like combine, except the 'withThis' item isn't modified.
	//bool Insert( const Item& withThis );
	// Remove one of the parts (i=1 or 2)
	//void RemovePart( int i, Item* item );
	// wipe this item
	void Clear()									{ rounds = 0; itemDef = 0; }

	const ItemDef* GetItemDef() const				{ return itemDef; }
	const WeaponItemDef* IsWeapon() const			{ return itemDef->IsWeapon(); }
	const ClipItemDef* IsClip() const				{ return itemDef->IsClip(); }
	const ArmorItemDef* IsArmor() const				{ return itemDef->IsArmor(); }

	int Rounds() const								{ return rounds; }

	// --- handle weapons ----//
	// consume one rounds
	void UseRounds( int n=1 );

	const char* Name() const						{ return itemDef->name; }
	const char* Desc() const						{ return itemDef->desc; }
	int Deco() const								{ return itemDef->deco; }

	bool IsNothing() const							{ return itemDef == 0; }
	bool IsSomething() const						{ return itemDef != 0; }

	void Save( UFOStream* s ) const;
	void Load( UFOStream* s, Engine* engine, Game* game );

private:
	int rounds;
	const ItemDef* itemDef;
};


class Storage
{
public:
	Storage()									{ memset( rounds, 0, sizeof(int)*EL_MAX_ITEM_DEFS ); }
	~Storage();

	void Init( const int* roundArr )			{ memcpy( rounds, roundArr, sizeof(int)*EL_MAX_ITEM_DEFS ); }
	const int* Rounds() const					{ return rounds; }

	bool Empty() const;
	void AddItem( const Item& item );
	void RemoveItem( const ItemDef*, Item* item );
	
	void SetCount( const ItemDef*, int count );
	int GetCount( const ItemDef* ) const;

private:
	int GetIndex( const ItemDef* itemDef ) const {
		int index = itemDef->index;
		GLASSERT( index >=0 && index < EL_MAX_ITEM_DEFS );
		return index;
	}

	int rounds[EL_MAX_ITEM_DEFS];
};


#endif // UFOATTACK_ITEM_INCLUDED