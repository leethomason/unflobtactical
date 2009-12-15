#include "item.h"
#include "material.h"
#include "game.h"
#include "../engine/serialize.h"
#include "gamelimits.h"


void WeaponItemDef::QueryWeaponRender( int select, grinliz::Vector4F* beamColor, float* beamDecay, float* beamWidth, grinliz::Vector4F* impactColor ) const
{
	GLASSERT( type == ITEM_WEAPON );
	const float INV=1.0f/255.0f;

	switch( weapon[select].clipType ) {
		case ITEM_CLIP_SHELL:
		case ITEM_CLIP_AUTO:
			beamColor->Set( 0.8f, 0.8f, 0.8f, 1.0f );
			*beamDecay = -3.0f;
			*beamWidth = 0.07f;
			impactColor->Set( 0.3f, 0.3f, 0.9f, 1.0f );
			break;

		case ITEM_CLIP_CELL:
			beamColor->Set( 1, 1, 0.8f, 1.0f );
			*beamDecay = -2.0f;
			*beamWidth = 0.12f;
			impactColor->Set( 242.0f*INV, 101.0f*INV, 34.0f*INV, 1.0f );
			break;

		default:
			GLASSERT( 0 );	// need to implement others
			break;
	}
}


bool WeaponItemDef::CompatibleClip( const ItemDef* id, int* which ) const
{
	if ( id->IsClip() ) {
		if ( weapon[0].clipType == id->type ) {
			*which = 0;
			return true;
		}
		else if ( weapon[1].clipType == id->type ) {
			*which = 1;
			return true;
		}
	}
	return false;
}


void WeaponItemDef::DamageBase( int select, float* damageArray ) const
{

	switch( weapon[select].clipType ) {
		case ITEM_CLIP_SHELL:
		case ITEM_CLIP_AUTO:
			damageArray[DAMAGE_KINETIC]		= 1.0f;				
			damageArray[DAMAGE_ENERGY]		= 0;
			damageArray[DAMAGE_INCINDIARY]	= 0;
			break;

		case ITEM_CLIP_CELL:
			if ( weapon[select].flags & WEAPON_EXPLOSIVE ) {
				damageArray[DAMAGE_KINETIC]		= 0.5f;				
				damageArray[DAMAGE_ENERGY]		= 0.5f;
				damageArray[DAMAGE_INCINDIARY]	= 0;
			}
			else {
				damageArray[DAMAGE_KINETIC]		= 0;				
				damageArray[DAMAGE_ENERGY]		= 1.0f;
				damageArray[DAMAGE_INCINDIARY]	= 0;
			}
			break;

		case ITEM_CLIP_CANON:

		default:
			GLASSERT( 0 );	// need to implement
	}
	for( int i=0; i<NUM_DAMAGE; ++i ) 
		damageArray[i] *= weapon[select].damage;
}


float WeaponItemDef::TimeBase( int select, int type ) const
{
	GLASSERT( select == 0 || select == 1 );
	GLASSERT( type >= 0 && type < 3 );

	float s = 0.0f;
	switch ( type ) {
		case SNAP_SHOT:		
			s = TU_SNAP_SHOT;	
			break;
		case AUTO_SHOT:		
			if ( weapon[select].flags & WEAPON_AUTO )
				s = TU_AUTO_SHOT;	
			break;
		case AIMED_SHOT:	
			s = TU_AIMED_SHOT;	
			break;
		default:
			GLASSERT( 0 );
	}
	
	s *= speed;

	// Secondary weapon is slower:
	if ( select == 1 )
		s *= 1.5f;

	return s;
}


void ItemPart::Init( const ItemDef* itemDef, int rounds )
{
	this->itemDef = itemDef;
	if ( rounds < 0 ) {
		const ClipItemDef* cid = itemDef->IsClip();
		if ( cid ) 
			this->rounds = cid->rounds;
		else
			this->rounds = 1;
	}
	else {
		this->rounds = rounds;
	}
}


Item::Item( const ItemDef* itemDef, int rounds )
{
	part[0].Init( itemDef, rounds );
	part[1].Clear();
	part[2].Clear();
}

Item::Item( Game* game, const char* name, int rounds )
{
	part[0].Init( game->GetItemDef( name ), rounds );
	part[1].Clear();
	part[2].Clear();
}


/*
void Item::Init( const ItemDef* itemDef )
{
	part[0].Init( itemDef, 1 );
	part[1].Clear();
	part[2].Clear();
}
*/


/*
void Item::Init( Game* game, const char* name )
{
	Init( game->GetItemDef( name ) );
}
*/


bool Item::Insert( const Item& withThis )
{
	Item item( withThis );
	bool consumed;
	return Combine( &item, &consumed );
}


void Item::RemovePart( int i, Item* item )
{
	GLASSERT( i>0 && i<3 );
	item->part[0] = part[i];
	item->part[1].Clear();
	item->part[2].Clear();
	part[i].Clear();
}

void Item::Clear()
{
	for( int i=0; i<3; ++i )
		part[i].Clear();
}


bool Item::Combine( Item* with, bool* consumed )
{
	int which = 0;
	const WeaponItemDef* wid = part[0].IsWeapon();
	bool result = false;

	if (	wid 
		 && wid->CompatibleClip( with->part[0].itemDef, &which ) ) 
	{
		// weapon0,1 maps to part 1,2
		which++;

		// The 'with' is the clip, and is defined by with.part[0].
		// this->part[0] is a weapon
		// this->part[1] is the primary fire
		// this->part[2] is the secondary fire
		//
		const ClipItemDef* clipDef = with->part[0].itemDef->IsClip();
		GLASSERT( clipDef );

		// The clip *type* is compatible (ITEM_CLIP_CANON), but maybe not
		// the specific ammo (MC-AC). Check that.
		if (    part[which].None()								// ammo not yet set
			 || part[which].itemDef == with->part[0].itemDef )	// same itemDef means the same exact type.
		{
			// then we have a match.

			if ( part[which].None() ) {
				// need to create the part to add to
				part[which].Init( with->part[0].itemDef, 0 );
			}

			int roundsThatFit = clipDef->rounds - part[which].rounds;
			GLASSERT( roundsThatFit >= 0 );

			if ( with->part[0].rounds > roundsThatFit ) {
				part[which].rounds += roundsThatFit;
				with->part[0].rounds -= roundsThatFit;
				*consumed = false;
				result = true;
			}
			else { 
				part[which].rounds += with->part[0].rounds;
				with->part[0].rounds = 0;
				*consumed = true;
				result = true;
			}
		}
	}
	return result;
}


void Item::Save( UFOStream* s ) const
{
	s->WriteU8( 1 );	// version
	for( int i=0; i<NUM_PARTS; ++i ) {
		if ( part[i].itemDef ) {
			s->WriteU8( 1 );
			s->WriteStr( part[i].itemDef->name );
			s->WriteU8( part[i].rounds );
		}
		else {
			s->WriteU8( 0 );
		}
	}
}


void Item::Load( UFOStream* s, Engine* engine, Game* game )
{
	Clear();

	int version = s->ReadU8();
	GLASSERT( version == 1 );

	for( int i=0; i<NUM_PARTS; ++i ) {
		int filled = s->ReadU8();
		if ( filled ) {
			const char* name = s->ReadStr();
			const ItemDef* itemDef = game->GetItemDef( name );
			GLASSERT( itemDef );
			part[i].itemDef = itemDef;
			part[i].rounds = s->ReadU8();
		}
	}
}


Storage::~Storage()
{
}


bool Storage::Empty() const
{
	for( int i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( rounds[i] )
			return false;
	}
	return true;
}


void Storage::AddItem( const Item& item )
{
	for( int i=0; i<3; ++i ) {
		if ( item.GetItemDef( i ) ) {
			int index = GetIndex( item.GetItemDef(i) );
			rounds[index] += item.Rounds(i);
		}
	}
}


void Storage::RemoveItem( const ItemDef* _itemDef, Item* _item )
{
	int index = GetIndex( _itemDef );
	int r = grinliz::Min( rounds[index], _itemDef->Rounds() );

	if ( r == 0 ) {
		_item->Clear();
	}
	else {
		Item item( _itemDef, r );
		rounds[index] -= r;
		*_item = item;
	}
}


void Storage::SetCount( const ItemDef* itemDef, int count )
{
	int index = GetIndex( itemDef );
	rounds[index] = count*itemDef->Rounds();
}


int Storage::GetCount( const ItemDef* itemDef) const
{
	int index = GetIndex( itemDef );
	int r = rounds[index];
	return (r+itemDef->Rounds()-1)/itemDef->Rounds();
}


// Return the "best" item for on-screen rendering.
/*const ItemDef* Storage::SomeItem() const
{
	int bestScore = -1;
	const ItemDef* best = 0;

	for( unsigned i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( rounds[i] > 0 ) {
			int score = rounds[i];
			
			if ( itemDefs[i]->IsWeapon() )
				score *= 20;

			if ( score > bestScore ) {
				bestScore = score;
				best = itemDefs[i];
			}
		}
	}
	return best;
}
*/

/*
void Storage::Save( UFOStream* s ) const
{
	s->WriteU32Arary( itemDefs.Size(), (const U32*) rounds.Mem() );
}

void Storage::Load( UFOStream* s )
{
	s->ReadU32Arary( itemDefs.Size(), (U32*) rounds.Mem() );
}
*/
