#include "item.h"
#include "material.h"
#include "game.h"
#include "../engine/serialize.h"


/*
void ItemDef::Init( int _type, const char* _name, ModelResource* _resource, int _size )
{
	GLASSERT( _resource );
	type = _type;
	name = _name;
	resource = _resource;
	size = _size;
	material = hp = rounds = damageBase = 0;
}


void ItemDef::InitWeapon( const char* _name, ModelResource* _resource, int _size, int _flags, int damage, int rounds )
{
	Init( TYPE_WEAPON, _name, _resource, _size );
	material = _flags;
	this->damageBase = damage;
	this->rounds = rounds;
}
*/

void ItemDef::QueryWeaponRender( grinliz::Vector4F* beamColor, float* beamDecay, float* beamWidth, grinliz::Vector4F* impactColor ) const
{
	GLASSERT( type == ITEM_WEAPON );

	if ( weapon[0].shell & SH_KINETIC ) {
		beamColor->Set( 0.8f, 0.8f, 0.8f, 1.0f );
		*beamDecay = -3.0f;
		*beamWidth = 0.07f;
		impactColor->Set( 0.3f, 0.3f, 0.9f, 1.0f );
	}
	if ( weapon[0].shell & SH_ENERGY ) {
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
	//this->rounds = itemDef->rounds;
}


void Item::Save( UFOStream* s ) const
{
	s->WriteU8( 1 );	// version
	if ( itemDef ) {
		s->WriteU8( 1 );
		s->WriteStr( itemDef->name );
		s->WriteU16( 0 );	// rounds
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

	int filled = s->ReadU8();
	if ( filled ) {
		const char* name = s->ReadStr();
		//hp = s->ReadU16();
		int rounds = s->ReadU16();

		itemDef = game->GetItemDef( name );
		GLASSERT( itemDef );
	}
}

