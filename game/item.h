#ifndef UFOATTACK_ITEM_INCLUDED
#define UFOATTACK_ITEM_INCLUDED

#include "../grinliz/glvector.h"

class ModelResource;
class UFOStream;
class Engine;
class Game;

struct ItemDef
{
	enum {
		TYPE_WEAPON,
		TYPE_ARMOR,

		AIMED = 0,
		SNAP  = 1,
		AUTO  = 2
	};

	int				type;
	const char*		name;
	ModelResource*	resource;

	int				material;
	int				hp;
	int				rounds;
	int				damageBase;
	int				size;		// 1-3 unit can carry, 4: big

	void Init( int _type, const char* _name, ModelResource* _resource, int _size );
	void InitWeapon( const char* _name, ModelResource* _resource, int _size, int flags, int damage, int rounds );

	void QueryWeaponRender( grinliz::Vector4F* beamColor, float* beamDecay, float* beamWidth, grinliz::Vector4F* impactColor ) const;
	bool IsWeapon() const { return type == TYPE_WEAPON; }
};


struct Item
{
	const ItemDef* itemDef;
	int hp;
	int rounds;

	void Init( const ItemDef* itemDef );
	void Save( UFOStream* s ) const;
	void Load( UFOStream* s, Engine* engine, Game* game );
	bool None() { return itemDef == 0; }
};


#endif // UFOATTACK_ITEM_INCLUDED
