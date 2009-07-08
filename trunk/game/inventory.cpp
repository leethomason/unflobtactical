#include "inventory.h"
#include "game.h"

int Inventory::slotSize[NUM_SLOTS] = {	3,			// hand
										1, 2, 2, 1,	// backpack
										2, 1, 1, 1, // belt
										1, 1 };		// leg


Inventory::Inventory()
{
	memset( slots, 0, sizeof( Item )*NUM_SLOTS );
}


bool Inventory::AddItem( int slot, const Item& item )
{
	const ItemDef* def = item.itemDef;

	// Is it too big?
	if ( def->size > 3 ) {
		return false;
	}

	// if the slot was specified:
	if ( slot != ANY_SLOT ) {
		if ( slots[slot].None() && def->size <= slotSize[slot] ) {
			slots[slot] = item;
			return true;
		}
		return false;
	}

	// Is it the weapon slot only?
	if ( def->size == 3 ) {
		if ( slots[0].None() ) {
			slots[0] = item;
			return true;
		}
		return false;
	}
	// Is it a weapon - prefer the 1st slot.
	if ( def->IsWeapon() && slots[0].None() ) {
		slots[0] = item;
		return true;
	}

	// Find the smallest possible slot.
	for( int i=def->size; i<=3; ++i ) {
		for( int j=1; j<NUM_SLOTS; ++j ) {
			if ( slotSize[j] == i && slots[j].None() ) {
				slots[j] = item;
				return true;
			}
		}
	}
	return false;
}


Item* Inventory::ArmedWeapon()
{
	if ( !slots[0].None() && slots[0].itemDef->IsWeapon() )
		return &slots[0];
	return 0;
}


void Inventory::Save( UFOStream* s )
{
	s->WriteU8( 1 );	// version
	for( int i=0; i<NUM_SLOTS; ++i ) {
		slots[i].Save( s );
	}
}


void Inventory::Load( UFOStream* s, Engine* engine, Game* game )
{
	int version = s->ReadU8();	// version
	GLASSERT( version == 1 );
	for( int i=0; i<NUM_SLOTS; ++i ) {
		slots[i].Load( s, engine, game );
	}
}

