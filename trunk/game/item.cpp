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


void Item::Init( const ItemDef* itemDef )
{
	this->itemDef = itemDef;
	//this->hp = itemDef->hp;
	this->rounds[0] = 10;
	this->rounds[1] = 10;
}


void Item::Save( UFOStream* s ) const
{
	s->WriteU8( 1 );	// version
	if ( itemDef ) {
		s->WriteU8( 1 );
		s->WriteStr( itemDef->name );
		GLASSERT( rounds[0] < 256 && rounds[1] < 256 );
		s->WriteU8( rounds[0] );
		s->WriteU8( rounds[1] );
	}
	else {
		s->WriteU8( 0 );
	}
}


void Item::Load( UFOStream* s, Engine* engine, Game* game )
{
	itemDef = 0;

	int version = s->ReadU8();
	GLASSERT( version == 1 );
	rounds[0] = rounds[1] = 0;

	int filled = s->ReadU8();
	if ( filled ) {
		const char* name = s->ReadStr();
		rounds[0] = s->ReadU8();
		rounds[1] = s->ReadU8();

		itemDef = game->GetItemDef( name );
		GLASSERT( itemDef );
	}
}

