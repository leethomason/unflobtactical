#ifndef UFOATTACK_ITEM_INCLUDED
#define UFOATTACK_ITEM_INCLUDED

#include "../grinliz/glvector.h"

class ModelResource;
class UFOStream;
class Engine;
class Game;

enum {
	ITEM_WEAPON,
	ITEM_ARMOR,
	ITEM_GENERAL,

	ITEM_CLIP_SHELL,
	ITEM_CLIP_AUTO,
	ITEM_CLIP_CELL,
	ITEM_CLIP_CANON,
	ITEM_CLIP_ROCKET,
	ITEM_CLIP_GRENADE,
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
	ModelResource*	resource;	// can be null, in which case render with crate.
	//int				size;		// 1-3 unit can carry, 4: big

	void InitBase( int type, const char* name, const char* desc, int deco, ModelResource* resource /*,int size*/ )
	{
		this->type = type; this->name = name; this->desc = desc; this->deco = deco; this->resource = resource; //this->size = size;
	}

	virtual const WeaponItemDef* IsWeapon() const { return 0; }
	virtual const ClipItemDef* IsClip() const  { return 0; }
	bool IsArmor() const { return type == ITEM_ARMOR; }
};

class WeaponItemDef : public ItemDef
{
public:
	virtual const WeaponItemDef* IsWeapon() const { return this; }

	struct Weapon {
		int shell;			// type of shell SH_KINETIC, etc.
		int clipType;		// type of clip required (ITEM_CLIP_ROCKET for example)
		int autoRounds;		// 0: no auto
		int damageBase;		// base damage
		int range;
		float accuracy;		// 1.0 is average
		int power;			// power consumed by cell weapons
	};
	Weapon weapon[2];		// primary and secondary

	void QueryWeaponRender( int select, grinliz::Vector4F* beamColor, float* beamDecay, float* beamWidth, grinliz::Vector4F* impactColor ) const;
	bool CompatibleClip( const ItemDef* itemDef, int* which ) const;
};

class ClipItemDef : public ItemDef
{
public:
	virtual const ClipItemDef* IsClip() const { return this; }

	int shell;			// modifies the shell of the weapon
	int rounds;			// rounds in this clip, power of cell (100)
};


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


struct Item
{
	enum {
		NUM_PARTS = 3
	};

	Item()								{ memset( part, 0, sizeof(ItemPart)*NUM_PARTS ); }
	Item( const ItemPart& itemPart )	{ memset( part, 0, sizeof(ItemPart)*NUM_PARTS ); 
										  part[0] = itemPart; }
	Item( const Item& rhs )				{ memcpy( part, rhs.part, sizeof(ItemPart)*NUM_PARTS ); }
	void operator=( const Item& rhs )	{ memcpy( part, rhs.part, sizeof(ItemPart)*NUM_PARTS ); }

	void Init( const ItemDef* itemDef ) {
		part[0].Init( itemDef );
	}

	bool Combine( Item* withThis, bool* consumed );

	// An item can be made up of up to 3 parts. 
	// 0-weapon, 1-clip, 2-clip
	// If there isn't a [0] part, then the item is empty.
	ItemPart part[NUM_PARTS]; 

	bool IsNothing() const { return part[0].None(); }
	bool IsSomething() const { return !part[0].None(); }

	void Save( UFOStream* s ) const;
	void Load( UFOStream* s, Engine* engine, Game* game );
};


#endif // UFOATTACK_ITEM_INCLUDED
