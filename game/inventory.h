#ifndef UFOATTACK_INVENTORY_INCLUDED
#define UFOATTACK_INVENTORY_INCLUDED

#include "item.h"

class ItemDef;
class Engine;
class Game;
class TiXmlElement;


/*  
	M M M	- backpack
	M M M
	A 2 1	- armor, secondary, primary
*/

class Inventory
{
public:

	enum {
		NO_SLOT = -2,
		ANY_SLOT = -1,
		WEAPON_SLOT_PRIMARY = 0,
		WEAPON_SLOT_SECONDARY,
		ARMOR_SLOT,
		GENERAL_SLOT
	};

	enum {
		DX = 3,
		DY = 4,
		NUM_SLOTS  = 6 + 3
	};

	Inventory();

	// Add an item. 0: weapon slot, -1: any slot available
	bool AddItem( const Item& item, int* slot=0 );
	bool RemoveItem( int slot );
	bool Empty() {
		for( int i=0; i<NUM_SLOTS; ++i ) {
			if ( slots[i].IsSomething() )
				return false;
		}
		return true;
	}

	bool IsGeneralSlotFree();
	bool IsSlotFree( const ItemDef* itemDef );

	int CalcClipRoundsTotal( const ClipItemDef* ) const;
	void UseClipRound( const ClipItemDef* );
	
	Item* ArmedWeapon();				// null if no weapon ready
	const Item* ArmedWeapon() const;	// null if no weapon ready
	const Item* SecondaryWeapon() const;

	void Save( TiXmlElement* doc ) const;
	void Load( const TiXmlElement* doc, Engine* engine, Game* game );

	const Item& GetItem( int slot ) const		{ GLASSERT( slot >=0 && slot < NUM_SLOTS ); return slots[slot]; }
	Item* AccessItem( int slot )				{ GLASSERT( slot >=0 && slot < NUM_SLOTS ); return &slots[slot]; }

	int GetDeco( int slot ) const;
	const ItemDef* GetItemDef( int slot ) const	{ GLASSERT( slot >=0 && slot < NUM_SLOTS ); return slots[slot].GetItemDef(); }

	void SwapWeapons();

private:

	Item slots[NUM_SLOTS];
};


#endif // UFOATTACK_INVENTORY_INCLUDED