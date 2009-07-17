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
		int clip;			// type of clip required
		int autoRounds;		// 0: no auto
		int damageBase;		// base damage
		int range;
		float accuracy;		// 1.0 is average
		int power;			// power consumed by cell weapons
	};
	Weapon weapon[2];		// primary and secondary
	void QueryWeaponRender( int select, grinliz::Vector4F* beamColor, float* beamDecay, float* beamWidth, grinliz::Vector4F* impactColor ) const;
};

class ClipItemDef : public ItemDef
{
public:
	virtual const ClipItemDef* IsClip() const { return this; }

	int shell;			// modifies the shell of the weapon
	int rounds;			// rounds in this clip, power of cell (100)
};


struct Item
{
	const ItemDef* itemDef;
	int rounds[2];

	void Init( const ItemDef* itemDef );
	void Save( UFOStream* s ) const;
	void Load( UFOStream* s, Engine* engine, Game* game );
	bool None() const										 { return itemDef == 0; }
};


#endif // UFOATTACK_ITEM_INCLUDED
