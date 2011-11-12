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
	A W		- carried
	M M M	- backpack
	M M M
*/

class Inventory
{
public:

	enum {
		WEAPON_SLOT = 0,
		ARMOR_SLOT,
		GENERAL_SLOT
	};

	enum {
		NUM_SLOTS  = 6 + 2
	};

	Inventory();

	// Add an item. returns slot if successful, -1 if not.
	// Dropped item allows an upgrade; can be null.
	int AddItem( const Item& item, Item* dropped );
	int AddItem( int slot, const Item& item );

	bool RemoveItem( int slot );
	bool Empty() {
		for( int i=0; i<NUM_SLOTS; ++i ) {
			if ( slots[i].IsSomething() )
				return false;
		}
		return true;
	}
	void ClearItem( const char* name );

	bool IsGeneralSlotFree();
	bool IsSlotFree( const ItemDef* itemDef );

	int CalcClipRoundsTotal( const ClipItemDef* ) const;
	void UseClipRound( const ClipItemDef* );
	void RestoreClips();
	
	Item* ArmedWeapon();				// null if no weapon ready
	const Item* ArmedWeapon() const;	// null if no weapon ready

	int GetArmorLevel() const;			// return armor level: 0-3

	void Save( FILE* fp, int depth ) const;
	void Load( const TiXmlElement* doc, const ItemDefArr& arr );

	Item GetItem( int slot ) const				{ GLASSERT( slot >=0 && slot < NUM_SLOTS ); return slots[slot]; }				// return a copy...too easy to change while in use
	Item* AccessItem( int slot )				{ GLASSERT( slot >=0 && slot < NUM_SLOTS ); return &slots[slot]; }

	bool Contains( const ItemDef* itemDef ) const;
	bool Contains( const char* name ) const;

	int GetDeco( int slot ) const;
	const ItemDef* GetItemDef( int slot ) const	{ GLASSERT( slot >=0 && slot < NUM_SLOTS ); return slots[slot].GetItemDef(); }

	// Writes to the 3 terms of damage desc as multipliers. 1: full damage from that type, 0: no damage.
	// Walks the inventory to account for Armor (if worn) and shielding.
	void GetDamageReduction( DamageDesc* );

private:
	int UpgradeItem( const Item& item, int slot, Item* dropped );
	Item slots[NUM_SLOTS];
};


#endif // UFOATTACK_INVENTORY_INCLUDED