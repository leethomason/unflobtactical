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
	ITEM_CLIP_CELL,
	ITEM_CLIP_FLAME,
	ITEM_CLIP_ROCKET,
	ITEM_CLIP_GRENADE,

	WEAPON_AUTO		= 0x01,
	WEAPON_MELEE	= 0x02,
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
		int power;			// power consumed by cell weapons
	};
	float	speed;			// 1.0 is normal speed (and weight)
	Weapon weapon[2];		// primary and secondary

	bool HasWeapon( int select ) const		{ GLASSERT( select == 0 || select == 1 ); return weapon[select].damage > 0; }
	bool SupportsType( int select, int type ) const		{ 
		GLASSERT( type >= 0 && type < 3 && select >= 0 && select <= 1 );  
		if ( type == AUTO_SHOT ) {
			return (weapon[select].flags & WEAPON_AUTO) ? true : false;
		}
		return true;
	}
	bool Melee() const { return weapon[0].flags & WEAPON_MELEE ? true : false; }

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
						 float* chanceToHit, float* totalDamage, float* damagePerTU ) const;

};


class ClipItemDef : public ItemDef
{
public:

	virtual const ClipItemDef* IsClip() const { return this; }
	virtual int Rounds() const { return rounds; }

	int	type;
	int rounds;			// rounds in this clip, power of cell (100)
};


class ArmorItemDef : public ItemDef
{
public:
	virtual const ArmorItemDef* IsArmor() const { return this; }
};

// POD
struct ItemPart
{
	const ItemDef* itemDef;
	int rounds;					// rounds, charges, whatever.

	void Init( const ItemDef* itemDef, int rounds=1 );

	const WeaponItemDef* IsWeapon() const	{ return itemDef->IsWeapon(); }
	const ClipItemDef* IsClip() const		{ return itemDef->IsClip(); }
	const ArmorItemDef* IsArmor() const		{ return itemDef->IsArmor(); }

	bool None() const			{ return itemDef == 0; }
	void Clear()				{ itemDef = 0; rounds = 0; }
	void SetEmpty()				{ rounds = 0; }
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
	enum {
		NUM_PARTS = 3
	};

	Item()								{ memset( part, 0, sizeof(ItemPart)*NUM_PARTS ); }
	Item( const Item& rhs )				{ memcpy( part, rhs.part, sizeof(ItemPart)*NUM_PARTS ); }
	Item( const ItemDef* itemDef, int rounds=1 );
	Item( Game*, const char* name, int rounds=1 );

	void operator=( const Item& rhs )	{ memcpy( part, rhs.part, sizeof(ItemPart)*NUM_PARTS ); }

	// Add rounds to this Item. Returns 'true' if the items combine, and 'consumed' is set
	// true if the added item (a clip) is fully used up.
	bool Combine( Item* withThis, bool* consumed );
	// Works like combine, except the 'withThis' item isn't modified.
	bool Insert( const Item& withThis );
	// Remove one of the parts (i=1 or 2)
	void RemovePart( int i, Item* item );
	// wipe this item
	void Clear();

	const ItemDef* GetItemDef( int i=0 ) const		{ GLASSERT( i>=0 && i<3 ); return part[i].itemDef; }
	const WeaponItemDef* IsWeapon( int i=0 ) const	{ GLASSERT( i>=0 && i<3 ); return part[i].itemDef->IsWeapon(); }
	const ClipItemDef* IsClip( int i=0 ) const		{ GLASSERT( i>=0 && i<3 ); return part[i].itemDef->IsClip(); }
	const ArmorItemDef* IsArmor( int i=0 ) const	{ GLASSERT( i>=0 && i<3 ); return part[i].itemDef->IsArmor(); }

	int Rounds( int i=0 ) const						{ GLASSERT( i>=0 && i<3 ); return part[i].rounds; }

	// --- handle weapons ----//
	// Requires IsWeapon()
	// If this is a weapon (isWeapon) then returns rounds for
	// the primary(1) or secondary(2) weapon.
	int RoundsRequired( int i ) const;
	// Requires IsWeapon()
	// How many rounds are needed to fire this weapon once. 
	// This is 1/3 the cost of auto-mode.
	// Generally 1 for kinetic weapons, and power level for cell weapons.
	int RoundsAvailable( int i ) const;
	// Reqires IsWeapon()
	// consume ronuds for one fire of the weapon
	void UseRound( int i );

	const char* Name() const						{ return part[0].itemDef->name; }
	const char* Desc() const						{ return part[0].itemDef->desc; }
	int Deco( int i=0 ) const						{ GLASSERT( i>=0 && i<3 ); return part[i].itemDef->deco; }

	bool HasPart( int i ) const		{ return part[i].itemDef != 0; }
	bool IsNothing() const			{ return part[0].None(); }
	bool IsSomething() const		{ return !part[0].None(); }

	void Save( UFOStream* s ) const;
	void Load( UFOStream* s, Engine* engine, Game* game );

private:
	// An item can be made up of up to 3 parts. 
	// 0-weapon, 1-clip, 2-clip
	// If there isn't a [0] part, then the item is empty.
	ItemPart part[NUM_PARTS]; 
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
