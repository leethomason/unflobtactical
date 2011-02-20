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

#include "inventory.h"
#include "game.h"
#include "../tinyxml/tinyxml.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;


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
//			if ( slots[i].Rounds() == 0 ) {
//				// clip is consumed.
//				slots[i].Clear();
//			}
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
	int slot = AddItem( item );
	if ( slot >= 0 ) {
		RemoveItem( slot );
		return true;
	}
	return false;
}


int Inventory::AddItem( const Item& item )
{
	GLASSERT( slots[WEAPON_SLOT].IsNothing() || slots[WEAPON_SLOT].IsWeapon() );
	GLASSERT( slots[ARMOR_SLOT].IsNothing() || slots[ARMOR_SLOT].IsArmor() );

	if ( item.IsWeapon() ) {
		if ( slots[WEAPON_SLOT].IsNothing() ) {
			slots[WEAPON_SLOT] = item;
			return WEAPON_SLOT;
		}
	}

	if ( item.IsArmor() ) {
		if ( slots[ARMOR_SLOT].IsNothing() ) {
			slots[ARMOR_SLOT] = item;
			return ARMOR_SLOT;
		}
	}

	for( int j=GENERAL_SLOT; j<NUM_SLOTS; ++j ) {
		if ( slots[j].IsNothing() ) {
			slots[j] = item;
			return j;
		}
	}

	return -1;
}


int Inventory::AddItem( int i, const Item& item )
{
	GLASSERT( slots[WEAPON_SLOT].IsNothing() || slots[WEAPON_SLOT].IsWeapon() );
	GLASSERT( slots[ARMOR_SLOT].IsNothing() || slots[ARMOR_SLOT].IsArmor() );
	GLASSERT( i >= 0 && i < NUM_SLOTS );

	if ( slots[i].IsSomething() )
		return -1;

	if ( i == WEAPON_SLOT && ( item.IsWeapon() || item.IsNothing() ) ) {	// adding nothing to nothing does nothing...except send back non-error.
		slots[i] = item;
		return i;
	}
	else if ( i == ARMOR_SLOT && ( item.IsArmor() || item.IsNothing() ) ) {
		slots[i] = item;
		return i;
	}
	else if ( i >= GENERAL_SLOT ) {
		slots[i] = item;
		return i;
	}
	return -1;
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


bool Inventory::Contains( const ItemDef* itemDef ) const
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slots[i].IsSomething() && slots[i].GetItemDef() == itemDef )
			return true;
	}
	return false;
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


int Inventory::GetArmorLevel() const
{
	int armor = 0;
	if ( slots[ARMOR_SLOT].IsArmor() ) {
		if ( StrEqual( slots[ARMOR_SLOT].Name(), "ARM-1" ) ) {
			armor = 1;
		}
		else if ( StrEqual( slots[ARMOR_SLOT].Name(), "ARM-2" ) ) {
			armor = 2;
		}
		else if ( StrEqual( slots[ARMOR_SLOT].Name(), "ARM-3" ) ) {
			armor = 3;
		}
	}
	return armor;
}


void Inventory::GetDamageReduction( DamageDesc* dd )
{
	float base = 1.0f;
	bool hasArmor = false;

	switch ( GetArmorLevel() ) {
	case 1:
		base = ARM1;
		hasArmor = true;
		break;

	case 2:
		base = ARM2;
		hasArmor = true;
		break;

	case 3:
		base = ARM3;
		hasArmor = true;
		break;

	default:
		break;
	}

	dd->Set( base, base, base );

	// Armor modifiers. Require Armor to modify.
	if ( hasArmor ) {
		bool k=false, e=false, i=false;
		for( int i=GENERAL_SLOT; i<NUM_SLOTS; ++i ) {
			if ( slots[i].IsSomething() && StrEqual( slots[i].Name(), "SG:E" ) ) {
				e = true;
				break;
			}
		}
		for( int i=GENERAL_SLOT; i<NUM_SLOTS; ++i ) {
			if ( slots[i].IsSomething() && StrEqual( slots[i].Name(), "SG:I" ) ) {
				i = true;
				break;
			}
		}
		for( int i=GENERAL_SLOT; i<NUM_SLOTS; ++i ) {
			if ( slots[i].IsSomething() && StrEqual( slots[i].Name(), "SG:K" ) ) {
				k = true;
				break;
			}
		}
		if ( k )
			dd->kinetic -= ARM_EXTRA;
		if ( e )
			dd->energy -= ARM_EXTRA;
		if ( i )
			dd->incend -= ARM_EXTRA;
	}
}


void Inventory::Save( FILE* fp, int depth ) const
{
	XMLUtil::OpenElement( fp, depth, "Inventory" );
	XMLUtil::SealElement( fp );
	for( int i=0; i<NUM_SLOTS; ++i ) {
		slots[i].Save( fp, depth+1 );
	}	
	XMLUtil::CloseElement( fp, depth, "Inventory" );
}


void Inventory::Load( const TiXmlElement* parent, Engine* engine, Game* game )
{
	const TiXmlElement* ele = parent->FirstChildElement( "Inventory" );
	int count = 0;
	if ( ele ) {
		for( const TiXmlElement* slot = ele->FirstChildElement( "Item" );
			 slot && count<NUM_SLOTS;
			 slot = slot->NextSiblingElement( "Item" ) )
		{
			Item item;
			item.Load( slot, game->GetItemDefArr() );
			AddItem( item );
			++count;
		}
	}
}


