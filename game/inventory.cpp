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


int Inventory::CalcClipRoundsTotal( const ClipItemDef* cid ) const
{
	int total = 0;
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slots[i].IsSomething() && slots[i].IsClip() && slots[i].IsClip() == cid ) {
			total += slots[i].Rounds();
		}
	}
	return total;
}


void Inventory::UseClipRound( const ClipItemDef* cid )
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slots[i].IsSomething() && slots[i].IsClip() && slots[i].IsClip() == cid ) {
			slots[i].UseRounds( 1 );
			return;
		}
	}
	GLASSERT( 0 );
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
	Item item( itemDef );
	int slot=0;
	if ( AddItem( item, &slot ) ) {
		RemoveItem( slot );
		return true;
	}
	return false;
}


bool Inventory::AddItem( const Item& item, int* slot )
{
	/*
	// if the slot was specified:
	if ( slot != ANY_SLOT ) {
		if ( slots[slot].IsNothing() ) {
			if ( slot == ARMOR_SLOT && !item.IsArmor() )
				return false;
			if ( slot == WEAPON_SLOT && !item.IsWeapon() ) 
				return false;

			slots[slot] = item;
			return true;
		}
		return false;
	}
	*/

	if ( item.IsArmor() ) {
		if ( slots[ARMOR_SLOT].IsNothing() ) {
			slots[ARMOR_SLOT] = item;
			if ( slot ) *slot = ARMOR_SLOT;
			return true;
		}
		return false;
	}

	if ( item.IsWeapon() ) {
		for( int i=0; i<2; ++i ) {
			if ( slots[WEAPON_SLOT_PRIMARY+i].IsNothing() ) {
				slots[WEAPON_SLOT_PRIMARY+i] = item;
				if ( slot ) *slot = WEAPON_SLOT_PRIMARY+i;
				return true;
			}
		}
		return true;
	}

	for( int j=GENERAL_SLOT; j<NUM_SLOTS; ++j ) {
		if ( slots[j].IsNothing() ) {
			slots[j] = item;
			if ( slot ) *slot = j;
			return true;
		}
	}

	return false;
}


bool Inventory::RemoveItem( int slot ) 
{
	GLASSERT( slot >= 0 && slot < NUM_SLOTS );
	if ( slots[slot].IsSomething() ) {
		slots[slot].Clear();
		return true;
	}
	return false;
}


Item* Inventory::ArmedWeapon()
{
	if ( slots[0].IsSomething() && slots[0].IsWeapon() )
		return &slots[0];
	return 0;
}


const Item* Inventory::ArmedWeapon() const
{
	if ( slots[0].IsSomething() && slots[0].IsWeapon() )
		return &slots[0];
	return 0;
}


const Item* Inventory::SecondaryWeapon() const
{
	if ( slots[WEAPON_SLOT_SECONDARY].IsSomething() && slots[WEAPON_SLOT_SECONDARY].IsWeapon() )
		return &slots[WEAPON_SLOT_SECONDARY];
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


void Inventory::SwapWeapons()
{
	grinliz::Swap( &slots[WEAPON_SLOT_PRIMARY], &slots[WEAPON_SLOT_SECONDARY] );
}


/*bool Inventory::Swap( int s0, int s1 )
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
*/

void Inventory::Save( UFOStream* s ) const
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


