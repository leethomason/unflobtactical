/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

	// Add an item. returns slot if successful, -1 if not
	int AddItem( const Item& item );
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

	int ArmorAmount() const;			// amount of armor points

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