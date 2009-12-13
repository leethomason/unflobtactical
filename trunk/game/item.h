#ifndef UFOATTACK_ITEM_INCLUDED
#define UFOATTACK_ITEM_INCLUDED

#include <string.h>
#include "../grinliz/glvector.h"
#include "../engine/ufoutil.h"
#include "../engine/enginelimits.h"

class ModelResource;
class UFOStream;
class Engine;
class Game;

enum {
	ITEM_WEAPON,
	ITEM_ARMOR,
	ITEM_GENERAL,

	// Clip types - plenty of complexity here already.
	ITEM_CLIP_SHELL = 1,
	ITEM_CLIP_AUTO,
	ITEM_CLIP_CELL,
	ITEM_CLIP_CANON,
	ITEM_CLIP_ROCKET,
	ITEM_CLIP_GRENADE,

	WEAPON_AUTO		= 0x01,
	WEAPON_MELEE	= 0x02,
	WEAPON_EXPLOSIVE = 0x04,	// only needed for cell weapons - adds a "feature" to the cell clip

	DAMAGE_KINETIC		= 0,
	DAMAGE_ENERGY		= 1,
	DAMAGE_INCINDIARY	= 2,
	NUM_DAMAGE			= 3
};

class WeaponItemDef;
class ClipItemDef;

class ItemDef
{
public:
	ItemDef() {};
	virtual ~ItemDef() {};

	int				type;
	const char*		name;
	const char*		desc;
	int				deco;
	const ModelResource*	resource;	// can be null, in which case render with crate.

	void InitBase( int type, const char* name, const char* desc, int deco, const ModelResource* resource, int index )
	{
		this->type = type; this->name = name; this->desc = desc; this->deco = deco; this->resource = resource; this->index = index;
	}

	virtual const WeaponItemDef* IsWeapon() const { return 0; }
	virtual const ClipItemDef* IsClip() const  { return 0; }
	virtual int Rounds() const { return 1; }
	bool IsArmor() const { return type == ITEM_ARMOR; }

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
		float damage;		// 
		float accuracy;		// 1.0 is average
		int power;			// power consumed by cell weapons
	};
	float	speed;			// 1.0 is normal speed (and weight)
	Weapon weapon[2];		// primary and secondary

	bool HasWeapon( int select ) const { GLASSERT( select == 0 || select == 1 ); return weapon[select].damage > 0; }
	bool Melee() const { return weapon[0].flags & WEAPON_MELEE ? true : false; }

	void QueryWeaponRender( int select, grinliz::Vector4F* beamColor, float* beamDecay, float* beamWidth, grinliz::Vector4F* impactColor ) const;
	bool CompatibleClip( const ItemDef* itemDef, int* which ) const;
	
	void DamageBase( int select, float* damageArray ) const;
	float TimeBase( int select, int type ) const;
};


class ClipItemDef : public ItemDef
{
public:

	virtual const ClipItemDef* IsClip() const { return this; }
	virtual int Rounds() const { return rounds; }

	int rounds;			// rounds in this clip, power of cell (100)
};

// POD
struct ItemPart
{
	const ItemDef* itemDef;
	int rounds;					// rounds, charges, whatever.

	void Init( const ItemDef* itemDef, int rounds=1 );

	const WeaponItemDef* IsWeapon() const	{ return itemDef->IsWeapon(); }
	const ClipItemDef* IsClip() const		{ return itemDef->IsClip(); }
	bool IsArmor() const					{ return itemDef->IsArmor(); }

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
	bool IsArmor( int i=0 ) const					{ GLASSERT( i>=0 && i<3 ); return part[i].itemDef->IsArmor(); }

	int Rounds( int i=0 ) const						{ GLASSERT( i>=0 && i<3 ); return part[i].rounds; }
	const char* Name() const						{ return part[0].itemDef->name; }
	const char* Desc() const						{ return part[0].itemDef->desc; }
	int Deco() const								{ return part[0].itemDef->deco; }

	bool IsNothing() const { return part[0].None(); }
	bool IsSomething() const { return !part[0].None(); }

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
