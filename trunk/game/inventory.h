#ifndef UFOATTACK_INVENTORY_INCLUDED
#define UFOATTACK_INVENTORY_INCLUDED

#include "item.h"

class ItemDef;
class UFOStream;
class Engine;
class Game;


/*  3x4

	M M M	- backpack
	S S S
	M S M	- belt
	S S L   - leg / hand
*/

class Inventory
{
public:

	enum {
		ANY_SLOT = -1,
		WEAPON_SLOT = 0,
		ARMOR_SLOT = 1,
		GENERAL_SLOT = 2
	};

	enum {
		DX = 3,
		DY = 4,
		NUM_SLOTS  = 12,
	};

	Inventory();

	// Add an item. 0: weapon slot, -1: any slot available
	bool AddItem( int slot, const Item& item );
	
	Item* ArmedWeapon();	// null if no weapon ready

	void Save( UFOStream* s );
	void Load( UFOStream* s, Engine* engine, Game* game );

	const Item& GetItem( int slot ) const		{ GLASSERT( slot >=0 && slot < NUM_SLOTS ); return slots[slot]; }
	//int GetSlotSize( int slot ) const			{ return slotSize[slot]; }

private:

	Item slots[NUM_SLOTS];
	//static int slotSize[NUM_SLOTS];
};


#endif // UFOATTACK_INVENTORY_INCLUDED