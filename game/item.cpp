#include "item.h"
#include "material.h"
#include "game.h"
#include "../engine/serialize.h"


void WeaponItemDef::QueryWeaponRender( int select, grinliz::Vector4F* beamColor, float* beamDecay, float* beamWidth, grinliz::Vector4F* impactColor ) const
{
	GLASSERT( type == ITEM_WEAPON );

	if ( weapon[select].shell & SH_KINETIC ) {
		beamColor->Set( 0.8f, 0.8f, 0.8f, 1.0f );
		*beamDecay = -3.0f;
		*beamWidth = 0.07f;
		impactColor->Set( 0.3f, 0.3f, 0.9f, 1.0f );
	}
	if ( weapon[select].shell & SH_ENERGY ) {
		beamColor->Set( 1, 1, 0.8f, 1.0f );
		*beamDecay = -2.0f;
		*beamWidth = 0.12f;
		const float INV=1.0f/255.0f;
		impactColor->Set( 242.0f*INV, 101.0f*INV, 34.0f*INV, 1.0f );
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


bool Item::Combine( Item* with, bool* consumed )
{
	int which = 0;
	const WeaponItemDef* wid = part[0].IsWeapon();
	bool result = false;

	if (	wid 
		 && wid->CompatibleClip( with->part[0].itemDef, &which ) ) 
	{
		// The 'with' is the clip, and is defined by with.part[0].
		// this->part[0] is a weapon
		// this->part[1] is the primary fire
		// this->part[2] is the secondary fire
		//
		const ClipItemDef* clipDef = with->part[0].itemDef->IsClip();
		GLASSERT( clipDef );

		// have a match
		int roundsThatFit = clipDef->rounds - part[which+1].rounds;
		GLASSERT( roundsThatFit >= 0 );

		if ( with->part[0].rounds > roundsThatFit ) {
			part[which+1].rounds += roundsThatFit;
			with->part[0].rounds -= roundsThatFit;
			*consumed = false;
			result = true;
		}
		else { 
			part[which+1].rounds += with->part[0].rounds;
			with->part[0].rounds = 0;
			*consumed = true;
			result = true;
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

