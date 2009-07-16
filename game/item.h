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

struct ItemDef
{
	struct Weapon {
		int shell;			// type of shell SH_KINETIC, etc.
		int clip;			// type of clip required
		int autoRounds;		// 0: no auto
		int damageBase;		// base damage
		int range;
		float accuracy;		// 1.0 is average
		int power;			// power consumed by cell weapons
	};
	struct Clip {
		int shell;			// modifies the shell of the weapon
		int rounds;			// rounds in this clip, power of cell (100)
	};
	int				type;
	const char*		name;
	const char*		desc;
	int				deco;
	ModelResource*	resource;	// can be null, in which case render with crate.
	int				size;		// 1-3 unit can carry, 4: big

	union {
		Weapon			weapon[2];	// primary fire and secondary fire
		Clip			clip;
	};

	void QueryWeaponRender( grinliz::Vector4F* beamColor, float* beamDecay, float* beamWidth, grinliz::Vector4F* impactColor ) const;
	bool IsWeapon() const { return type == ITEM_WEAPON; }
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
