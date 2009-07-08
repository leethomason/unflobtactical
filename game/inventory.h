#ifndef UFOATTACK_INVENTORY_INCLUDED
#define UFOATTACK_INVENTORY_INCLUDED

#include "item.h"

struct ItemDef;
class UFOStream;
class Engine;
class Game;



class Inventory
{
public:

	enum {
		ANY_SLOT = -1,
		WEAPON_SLOT = 0
	};

	Inventory();
	// Add an item. 0: weapon slot, -1: any slot available
	bool AddItem( int slot, const Item& item );
	
	Item* ArmedWeapon();	// null if no weapon ready

	void Save( UFOStream* s );
	void Load( UFOStream* s, Engine* engine, Game* game );

private:

	/*
		weapon
			3
		backpack:
			1221
		belt
			2112
		leg
			11
	*/
	enum {
		NUM_SLOTS = 11
	};
	Item slots[NUM_SLOTS];
	static int slotSize[NUM_SLOTS];
};


#endif // UFOATTACK_INVENTORY_INCLUDED