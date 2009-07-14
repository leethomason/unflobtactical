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
	ITEM_CLIP,
	ITEM_GENERAL
};

		
struct ItemDef
{
	int				type;
	const char*		name;
	const char*		desc;
	int				deco;
	ModelResource*	resource;

	//int				hp;			// how tough this is
	int				size;		// 1-3 unit can carry, 4: big

	int				material;	// what it does damage to
	int				rounds;		// how many rounds it carries (weapons)
	int				damageBase;	// how much damage it does.

	//void Init( int _type, const char* _name, ModelResource* _resource, int _size );
	//void InitWeapon( const char* _name, ModelResource* _resource, int _size, int flags, int damage, int rounds );

	void QueryWeaponRender( grinliz::Vector4F* beamColor, float* beamDecay, float* beamWidth, grinliz::Vector4F* impactColor ) const;
	bool IsWeapon() const { return type == ITEM_WEAPON; }
};


struct Item
{
	const ItemDef* itemDef;
	int hp;
	int rounds;

	void Init( const ItemDef* itemDef );
	void Save( UFOStream* s ) const;
	void Load( UFOStream* s, Engine* engine, Game* game );
	bool None() const										 { return itemDef == 0; }
};


#endif // UFOATTACK_ITEM_INCLUDED
