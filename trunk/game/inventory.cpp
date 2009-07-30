#include "inventory.h"
#include "game.h"

//int Inventory::slotSize[NUM_SLOTS] = {	3,			// hand
//										2, 2, 2, 1, 1, 1,	// backpack
//										2, 1, 2,	// belt
//										1, 1 };		// leg


Inventory::Inventory()
{
	memset( slots, 0, sizeof( Item )*NUM_SLOTS );
}


bool Inventory::IsGeneralSlotFree()
{
	for( int i=GENERAL_SLOT; i<NUM_SLOTS; ++i ) {
		if ( slots[i].IsNothing() )
			return true;
	}
	return false;
}


bool Inventory::IsSlotFree( const ItemDef* itemDef )
{
	if ( itemDef->IsWeapon() && slots[WEAPON_SLOT].IsNothing() )
		return true;
	if ( itemDef->IsArmor() && slots[ARMOR_SLOT].IsNothing() )
		return true;
	return IsGeneralSlotFree();
}


bool Inventory::AddItem( int slot, const Item& item )
{
	const ItemDef* def = item.GetItemDef();

//	// Is it too big?
//	if ( def->size > 3 ) {
//		return false;
//	}

	// if the slot was specified:
	if ( slot != ANY_SLOT ) {
		if ( slots[slot].IsNothing() /*&& def->size <= slotSize[slot]*/ ) {
			if ( slot == ARMOR_SLOT && !item.IsArmor() )
				return false;
			if ( slot == WEAPON_SLOT && !item.IsWeapon() ) 
				return false;

			slots[slot] = item;
			return true;
		}
		return false;
	}

	if ( item.IsArmor() && slots[ARMOR_SLOT].IsNothing() ) {
		slots[ARMOR_SLOT] = item;
		return true;
	}

	if ( item.IsWeapon() && slots[WEAPON_SLOT].IsNothing() ) {
		slots[WEAPON_SLOT] = item;
		return true;
	}

	for( int j=GENERAL_SLOT; j<NUM_SLOTS; ++j ) {
		if ( slots[j].IsNothing() ) {
			slots[j] = item;
			return true;
		}
	}

/*
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
	}*/
	return false;
}


Item* Inventory::ArmedWeapon()
{
	if ( slots[0].IsSomething() && slots[0].IsWeapon() )
		return &slots[0];
	return 0;
}



int Inventory::GetDeco( int s0 ) const
{
	int deco = DECO_NONE;
	GLASSERT( s0 >= 0 && s0 < NUM_SLOTS );
	if ( slots[s0].IsSomething() ) {
		deco = slots[s0].Deco();
	}
	return deco;
}


bool Inventory::Swap( int s0, int s1 )
{
	GLASSERT( s0 >= 0 && s0 < NUM_SLOTS );
	GLASSERT( s1 >= 0 && s1 < NUM_SLOTS );

	bool swap = false;

	if ( s0 >= GENERAL_SLOT && s1 >= GENERAL_SLOT ) 
	{
		// Can swap anything in the general section:
		swap = true;
	}
	else if (    (s0 == WEAPON_SLOT && (slots[s1].IsNothing() || slots[s1].IsWeapon() ) )
		      || (s1 == WEAPON_SLOT && (slots[s0].IsNothing() || slots[s0].IsWeapon() ) ) )
	{
		swap = true;
	}
	else if (    (s0 == ARMOR_SLOT && (slots[s1].IsNothing() || slots[s1].IsArmor() ) )
			  || (s1 == ARMOR_SLOT && (slots[s0].IsNothing() || slots[s0].IsArmor() ) ) )
	{
		swap = true;
	}

	if ( swap )
		grinliz::Swap( &slots[s0], &slots[s1] );
	return swap;
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


