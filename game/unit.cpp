#include "unit.h"
#include "game.h"
#include "../engine/engine.h"
#include "material.h"

using namespace grinliz;

const char* gMaleFirstNames[9] = 
{
	"Lee",
	"Jeff",
	"Sean",
	"Vlad",
	"Arnold",
	"Max",
	"Otto",
	"James",
	"Jason"
};


const char* gFemaleFirstNames[9] = 
{
	"Rayne",
	"Anne",
	"Jade",
	"Suzie",
	"Greta",
	"Lilith",
	"Luna",
	"Riko",
	"Jane",
};


const char* gLastNames[8] = 
{
	"Payne",
	"Havok",
	"Fury",
	"Schwartz",
	"Bourne",
	"Bond",
	"Smith",
	"Anderson",
};


const char* Unit::FirstName()
{
	const char* str = "";
	if ( team == SOLDIER ) {
		const int mfnLen = sizeof(gMaleFirstNames)/sizeof(const char*);
		const int ffnLen = sizeof(gFemaleFirstNames)/sizeof(const char*);

		U32 r = (body>>NAME0_SHIFT)&NAME0_MASK;

		if ( Gender() == MALE )
			str = gMaleFirstNames[ r % mfnLen ];
		else
			str = gFemaleFirstNames[ r % ffnLen ];
	}
	return str;
}


const char* Unit::LastName()
{
	const char* str = "";
	if ( team == SOLDIER ) {
		const int lnLen  = sizeof(gLastNames)/sizeof(const char*);

		U32 r = (body>>NAME1_SHIFT)&NAME1_MASK;
		str = gLastNames[ r % lnLen ];
	}
	return str;
}


void Unit::GenerateSoldier( U32 seed )
{
	status = STATUS_ALIVE;
	team = SOLDIER;
	body = seed;
	stats.Init( 50 );
}


void Unit::GenerateCiv( U32 seed )
{
	status = STATUS_ALIVE;
	team = CIVILIAN;
	body = seed;	// only gender...
	stats.Init( 25 );
}


void Unit::GenerateAlien( int type, U32 seed )
{
	status = STATUS_ALIVE;
	team = ALIEN;
	body = seed & (~ALIEN_TYPE_MASK);
	body |= (type & ALIEN_TYPE_MASK);

	switch ( type ) {
		case 0:	stats.Init( 40 );	break;
		case 1: stats.Init( 60 );	break;
		case 2: stats.Init( 150 );	break;
		case 3:
		default:
				stats.Init( 80 );	break;
	}
}


void Unit::Init(	Engine* engine, Game* game, 
					int team,	 
					int alienType,	// if alien...
					U32 seed )
{
	GLASSERT( status == STATUS_NOT_INIT );
	this->engine = engine;
	this->game = game;
	this->team = team;
	weapon = 0;
	weaponItem = 0;

	switch( team ) {
		case SOLDIER:	GenerateSoldier( seed );			break;
		case ALIEN:		GenerateAlien( alienType, seed );	break;
		case CIVILIAN:	GenerateCiv( seed );				break;
		default:
			GLASSERT( 0 );
	}
	CreateModel( true );
}


Unit::~Unit()
{
	Free();
}


void Unit::Free()
{
	if ( status == STATUS_NOT_INIT )
		return;

	if ( model ) {
		engine->FreeModel( model );
		model = 0;
	}
	if ( weapon ) {
		engine->FreeModel( weapon );
		weapon = 0;
	}
	status = STATUS_NOT_INIT;
}


void Unit::SetMapPos( int x, int z )
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );

	grinliz::Vector3F p = { (float)x + 0.5f, 0.0f, (float)z + 0.5f };
	model->SetPos( p );
	UpdateWeapon();
}


void Unit::SetYRotation( float rotation )
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );
	model->SetYRotation( rotation );
	UpdateWeapon();
}


void Unit::SetPos( const grinliz::Vector3F& pos, float rotation )
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );
	model->SetPos( pos );
	model->SetYRotation( rotation );
	UpdateWeapon();
}


void Unit::SetWeapon( const ItemDef* itemDef ) 
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );
	GLASSERT( itemDef );

	weaponItem = itemDef;
	if ( weapon ) {
		engine->FreeModel( weapon );
	}

	weapon = 0;	// don't render non-weapon items
	if ( itemDef->type == ItemDef::TYPE_WEAPON ) {
		weapon = engine->AllocModel( itemDef->resource );
		weapon->SetFlag( Model::MODEL_NO_SHADOW );
	}
	UpdateWeapon();
}


void Unit::UpdateWeapon()
{
	if ( weapon ) {
		Matrix4 r;
		r.SetYRotation( model->GetYRotation() );
		Vector4F mPos, gPos, pos4;

		mPos.Set( model->Pos(), 1 );
		gPos.Set( model->GetResource()->header.trigger, 1.0 );
		pos4 = mPos + r*gPos;
		weapon->SetPos( pos4.x, pos4.y, pos4.z );
		weapon->SetYRotation( model->GetYRotation() );
	}
}


void Unit::CalcPos( grinliz::Vector3F* vec ) const
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );

	*vec = model->Pos();
}


void Unit::CalcMapPos( grinliz::Vector2I* vec ) const
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );

	vec->x = (int)model->X();
	vec->y = (int)model->Z();
}


void Unit::Kill()
{
	GLASSERT( status == STATUS_ALIVE );
	GLASSERT( model );
	float r = model->GetYRotation();
	Vector3F pos = model->Pos();

	Free();
	status = STATUS_DEAD;
	
	CreateModel( false );

	stats.ZeroHP();
	model->SetYRotation( r );
	model->SetPos( pos );
	model->SetFlag( Model::MODEL_NO_SHADOW );
}


void Unit::DoDamage( int damageBase, int shell )
{
	int hp = MaterialDef::CalcDamage( damageBase, shell, MaterialDef::MAT_GENERIC );

	stats.DoDamage( hp );
	if ( !stats.HP() ) {
		Kill();
	}
}


void Unit::CreateModel( bool alive )
{
	ModelResource* resource = 0;
	switch ( team ) {
		case SOLDIER:
			if ( alive )
				resource = game->GetResource( ( Gender() == MALE ) ? "maleMarine" : "femaleMarine" );
			else
				resource = game->GetResource( ( Gender() == MALE ) ? "maleMarine_D" : "femaleMarine_D" );
			break;

		case CIVILIAN:
			if ( alive )
				resource = game->GetResource( ( Gender() == MALE ) ? "maleCiv" : "femaleCiv" );
			else
				resource = game->GetResource( ( Gender() == MALE ) ? "maleCiv_D" : "femaleCiv_D" );
			break;

		case ALIEN:
			{
				switch( AlienType() ) {
					case 0:	resource = game->GetResource( alive ? "alien0" : "alien0_D" );	break;
					case 1:	resource = game->GetResource( alive ? "alien1" : "alien1_D" );	break;
					case 2:	resource = game->GetResource( alive ? "alien2" : "alien2_D" );	break;
					case 3:	resource = game->GetResource( alive ? "alien3" : "alien3_D" );	break;
					default: GLASSERT( 0 );	break;
				}
			}
			break;
		
		default:
			GLASSERT( 0 );
			break;
	};
	GLASSERT( resource );
	model = engine->AllocModel( resource );
	GLASSERT( model->stats == 0 );
	model->stats = &stats;
	UpdateModel();
}


void Unit::UpdateModel()
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );

	switch ( team ) {
		case SOLDIER:
			{
				int armor = 0;
				int hair = ( body >> HAIR_SHIFT ) & HAIR_MASK;
				int skin = ( body >> SKIN_SHIFT ) & SKIN_MASK;
				model->SetSkin( armor, skin, hair );
			}
			break;

		case CIVILIAN:
			break;

		case ALIEN:
			break;
		
		default:
			GLASSERT( 0 );
			break;
	};
}


void Unit::Save( UFOStream* s )
{
	s->WriteU8( status );
	if ( status != STATUS_NOT_INIT ) {
		s->WriteU8( team );
		s->WriteU32( body );

		s->WriteFloat( model->Pos().x );
		s->WriteFloat( model->Pos().z );
		s->WriteFloat( model->GetYRotation() );
	}
}


void Unit::Load( UFOStream* s, Engine* engine, Game* game )
{
	Free();
	int _status = s->ReadU8();
	GLASSERT( _status == STATUS_NOT_INIT || _status == STATUS_ALIVE || _status == STATUS_DEAD );
	if ( _status != STATUS_NOT_INIT ) {
		team = s->ReadU8();
		body = s->ReadU32();

		Init( engine, game, team, ((body>>ALIEN_TYPE_SHIFT) & ALIEN_TYPE_MASK), body );
		status = _status;
		
		Vector3F pos = { 0, 0, 0 };
		float rot;

		pos.x = s->ReadFloat();
		pos.z = s->ReadFloat();
		rot   = s->ReadFloat();

		model->SetPos( pos );
		model->SetYRotation( rot );
	}
}


float Unit::AngleBetween( const Unit* target, bool quantize ) const 
{
	Vector2I p0, p1;
	CalcMapPos( &p0 );
	target->CalcMapPos( &p1 );

	float angle = atan2( (float)(p1.x-p0.x), (float)(p1.y-p0.y) );
	angle = ToDegree( angle );

	if ( quantize ) {
		if ( angle < 0.0f ) angle += 360.0f;
		if ( angle >= 360.0f ) angle -= 360.0f;

		int r = (int)( (angle+45.0f/2.0f) / 45.0f );
		return (float)(r*45.0f);
	}
	return angle;
}


void ItemDef::Init( int _type, const char* _name, ModelResource* _resource )
{
	GLASSERT( _resource );
	type = _type;
	name = _name;
	resource = _resource;
	material = 0;
}


void ItemDef::InitWeapon( const char* _name, ModelResource* _resource, int _flags, int damage )
{
	Init( TYPE_WEAPON, _name, _resource );
	material = _flags;
	this->damage = damage;
}


void ItemDef::QueryWeaponRender( grinliz::Vector4F* beamColor, float* beamDecay, float* beamWidth, grinliz::Vector4F* impactColor ) const
{
	GLASSERT( type == TYPE_WEAPON );

	if ( material & MaterialDef::SH_KINETIC ) {
		beamColor->Set( 0.8f, 0.8f, 0.8f, 1.0f );
		*beamDecay = -3.0f;
		*beamWidth = 0.07f;
		impactColor->Set( 0.3f, 0.3f, 0.9f, 1.0f );
	}
	if ( material & MaterialDef::SH_ENERGY ) {
		beamColor->Set( 1, 1, 0.8f, 1.0f );
		*beamDecay = -2.0f;
		*beamWidth = 0.12f;
		const float INV=1.0f/255.0f;
		impactColor->Set( 242.0f*INV, 101.0f*INV, 34.0f*INV, 1.0f );
	}
}