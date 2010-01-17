#include "unit.h"
#include "game.h"
#include "../engine/engine.h"
#include "material.h"

using namespace grinliz;

// Name first name length: 6
const char* gMaleFirstNames[8] = 
{
	//"Lee",
	"Jeff",
	"Sean",
	"Vlad",
	"Arnold",
	"Max",
	"Otto",
	"James",
	"Jason"
};

// Name first name length: 6
const char* gFemaleFirstNames[8] = 
{
	"Rayne",
	"Anne",
	"Jade",
	"Suzie",
	"Greta",
	//"Lilith",
	"Luna",
	"Riko",
	"Jane",
};


// Name last name length: 6
const char* gLastNames[8] = 
{
	"Payne",
	"Havok",
	"Fury",
	"Scharz",
	"Bourne",
	"Bond",
	"Smith",
	"Andson",
};


const char* gRank[] = {
	"Rok",
	"Pri",
	"Sgt",
	"Maj",
	"Cpt",
	"Cmd"
};


U32 Unit::GetValue( int which ) const
{
	const int NBITS[] = { 2, 1, 2, 2, 3, 3 };	// ALIEN_TYPE, GENDER, ...

	int i;
	U32 shift = 0;
	for( i=0; i<which; ++i ) {
		shift += NBITS[i];
	}
	U32 mask = (1<<NBITS[which])-1;
	return (body>>shift) & mask;
}


const char* Unit::FirstName() const
{
	const char* str = "";
	if ( team == SOLDIER ) {
		if ( Gender() == MALE )
			str = gMaleFirstNames[ GetValue( FIRST_NAME ) ];
		else
			str = gFemaleFirstNames[ GetValue( FIRST_NAME ) ];
	}
	GLASSERT( strlen( str ) <= 6 );
	return str;
}


const char* Unit::LastName() const
{
	const char* str = "";
	if ( team == SOLDIER ) {
		str = gLastNames[ GetValue( LAST_NAME ) ];
	}
	GLASSERT( strlen( str ) <= 6 );
	return str;
}


const char* Unit::Rank() const
{
	GLASSERT( stats.Level() >=0 && stats.Level() < 6 ); 
	return gRank[ stats.Level() ];
}

void Unit::GenerateSoldier( U32 seed )
{
	status = STATUS_ALIVE;
	team = SOLDIER;
	body = seed;

	Random random( seed );
	stats.SetSTR( stats.GenStat( &random, 20, 80 ) );
	stats.SetDEX( stats.GenStat( &random, 20, 80 ) );
	stats.SetPSY( stats.GenStat( &random, 20, 80 ) );
	stats.SetLevel( 2 );
	stats.CalcBaselines();
}


void Unit::GenerateCiv( U32 seed )
{
	status = STATUS_ALIVE;
	team = CIVILIAN;
	body = seed;	// only gender...

	Random random( seed );
	stats.SetLevel( 0 );
	stats.SetSTR( stats.GenStat( &random, 10, 60 ) );
	stats.SetDEX( stats.GenStat( &random, 10, 60 ) );
	stats.SetPSY( stats.GenStat( &random, 10, 60 ) );
	stats.CalcBaselines();
}


void Unit::GenerateAlien( int type, U32 seed )
{
	status = STATUS_ALIVE;
	team = ALIEN;
	U32 MASK = 0x03;
	body  = seed & (~MASK);
	body |= (type & MASK);
	Random random( seed );

	switch ( type ) {
		case 0:	
			stats.SetLevel( 1 );
			stats.SetSTR( stats.GenStat( &random, 10, 50 ) );
			stats.SetDEX( stats.GenStat( &random, 20, 80 ) );
			stats.SetPSY( stats.GenStat( &random, 20, 100 ) );
			break;

		case 1: 
			stats.SetLevel( 1 );
			stats.SetSTR( stats.GenStat( &random, 30, 70 ) );
			stats.SetDEX( stats.GenStat( &random, 20, 80 ) );
			stats.SetPSY( stats.GenStat( &random, 40, 120 ) );
			break;

		case 2: 
			stats.SetLevel( 1 );
			stats.SetSTR( stats.GenStat( &random, 60, 140 ) );
			stats.SetDEX( stats.GenStat( &random, 40, 100 ) );
			stats.SetPSY( stats.GenStat( &random, 20, 90 ) );
			break;

		case 3:
		default:
			stats.SetLevel( 1 );
			stats.SetSTR( stats.GenStat( &random, 20, 70 ) );
			stats.SetDEX( stats.GenStat( &random, 40, 100 ) );
			stats.SetPSY( stats.GenStat( &random, 80, 140 ) );
			break;
	}
	stats.CalcBaselines();
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
	visibilityCurrent = false;

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
	visibilityCurrent = false;
}


void Unit::SetYRotation( float rotation )
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );
	model->SetYRotation( rotation );
	UpdateWeapon();
	visibilityCurrent = false;
}


void Unit::SetPos( const grinliz::Vector3F& pos, float rotation )
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );
	model->SetPos( pos );
	model->SetYRotation( rotation );
	UpdateWeapon();
	visibilityCurrent = false;
}


Item* Unit::GetWeapon()
{
	GLASSERT( status == STATUS_ALIVE );
	return inventory.ArmedWeapon();
}


const Item* Unit::GetWeapon() const
{
	GLASSERT( status == STATUS_ALIVE );
	return inventory.ArmedWeapon();
}

Inventory* Unit::GetInventory()
{
	return &inventory;
}


void Unit::UpdateInventory() 
{
	GLASSERT( status != STATUS_NOT_INIT );

	if ( weapon ) {
		engine->FreeModel( weapon );
	}
	weapon = 0;	// don't render non-weapon items

	const Item* weaponItem = inventory.ArmedWeapon();
	if ( weaponItem  ) {
		weapon = engine->AllocModel( weaponItem->GetItemDef()->resource );
		weapon->SetFlag( Model::MODEL_NO_SHADOW );
		weapon->SetFlag( Model::MODEL_MAP_TRANSPARENT );
	}
	UpdateWeapon();
}


void Unit::UpdateWeapon()
{
	GLASSERT( status == STATUS_ALIVE );
	if ( weapon && model ) {
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


void Unit::CalcVisBounds( grinliz::Rectangle2I* b ) const
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );

	Vector2I p;
	CalcMapPos( &p, 0 );
	b->Set( Max( 0, p.x-MAX_EYESIGHT_RANGE ), 
			Max( 0, p.y-MAX_EYESIGHT_RANGE ),
			Min( p.x+MAX_EYESIGHT_RANGE, MAP_SIZE-1),
			Min( p.y+MAX_EYESIGHT_RANGE, MAP_SIZE-1) );
}


void Unit::CalcMapPos( grinliz::Vector2I* vec, float* rot ) const
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );

	// Account that model can be incrementally moved when animating.
	if ( vec ) {
		vec->x = LRintf( model->X() - 0.5f );
		vec->y = LRintf( model->Z() - 0.5f );
	}
	if ( rot ) {
		float r = model->GetYRotation() + 45.0f/2.0f;
		int ir = (int)( r / 45.0f );
		*rot = (float)(ir*45);
	}
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
	visibilityCurrent = false;
}


void Unit::DoDamage( const DamageDesc& damage )
{
	GLASSERT( status != STATUS_NOT_INIT );
	if ( status == STATUS_ALIVE ) {
		// FIXME: account for armor
		stats.DoDamage( (int)damage.Total() );
		if ( !stats.HP() ) {
			Kill();
			visibilityCurrent = false;
		}
	}
}


void Unit::NewTurn()
{
	if ( status == STATUS_ALIVE ) {
		stats.RestoreTU();
		userDone = false;
	}
}


void Unit::CreateModel( bool alive )
{
	GLASSERT( status != 0 );

	const ModelResource* resource = 0;
	ModelResourceManager* modman = ModelResourceManager::Instance();

	switch ( team ) {
		case SOLDIER:
			if ( alive )
				resource = modman->GetModelResource( ( Gender() == MALE ) ? "maleMarine" : "femaleMarine" );
			else
				resource = modman->GetModelResource( ( Gender() == MALE ) ? "maleMarine_D" : "femaleMarine_D" );
			break;

		case CIVILIAN:
			if ( alive )
				resource = modman->GetModelResource( ( Gender() == MALE ) ? "maleCiv" : "femaleCiv" );
			else
				resource = modman->GetModelResource( ( Gender() == MALE ) ? "maleCiv_D" : "femaleCiv_D" );
			break;

		case ALIEN:
			{
				switch( AlienType() ) {
					case 0:	resource = modman->GetModelResource( alive ? "alien0" : "alien0_D" );	break;
					case 1:	resource = modman->GetModelResource( alive ? "alien1" : "alien1_D" );	break;
					case 2:	resource = modman->GetModelResource( alive ? "alien2" : "alien2_D" );	break;
					case 3:	resource = modman->GetModelResource( alive ? "alien3" : "alien3_D" );	break;
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
	model->SetFlag( Model::MODEL_MAP_TRANSPARENT );
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
				int hair = GetValue( HAIR );
				int skin = GetValue( SKIN );
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


void Unit::Save( UFOStream* s ) const
{
	s->WriteU8( status );
	if ( status != STATUS_NOT_INIT ) {
		s->WriteU8( 1 );	// version
		s->WriteU8( team );
		s->WriteU32( body );

		s->WriteFloat( model->Pos().x );
		s->WriteFloat( model->Pos().z );
		s->WriteFloat( model->GetYRotation() );

		inventory.Save( s );
	}
}


void Unit::Load( UFOStream* s, Engine* engine, Game* game )
{
	Free();

	int _status = s->ReadU8();
	GLASSERT( _status == STATUS_NOT_INIT || _status == STATUS_ALIVE || _status == STATUS_DEAD );
	if ( _status != STATUS_NOT_INIT ) {
		Vector3F pos = { 0, 0, 0 };
		float rot;

		int version = s->ReadU8();
		GLASSERT( version == 1 );

		team = s->ReadU8();
		body = s->ReadU32();

		pos.x = s->ReadFloat();
		pos.z = s->ReadFloat();
		rot   = s->ReadFloat();

		Init( engine, game, team, body&0x03, body );
		status = _status;
		
		if ( model ) {
			model->SetPos( pos );
			model->SetYRotation( rot );
		}

		inventory.Load( s, engine, game );
		UpdateInventory();
	}
}


float Unit::AngleBetween( const Vector2I& p1, bool quantize ) const 
{
	Vector2I p0;
	CalcMapPos( &p0, 0 );

	//target->CalcMapPos( &p1, 0 );

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


float Unit::FireTimeUnits( int select, int type ) const
{
	float time = 0.0f;
	const Item* item = GetWeapon();
	if ( item && item->IsWeapon() ) {
		const WeaponItemDef* weaponItemDef = item->IsWeapon();
		time = weaponItemDef->TimeUnits( select, type );
	}
	// Note: don't adjust for stats. TU is based on DEX. Adjusting again double-applies.
	return time;
}


void Unit::FireStatistics( int select, int type, float distance, float* chanceToHit, float* chanceAnyHit, float* tu, float* damagePerTU )
{
	*chanceToHit = 0.0f;
	*tu = 0.0f;
	*damagePerTU = 0.0f;
	float damage;

	if ( GetWeapon() && GetWeapon()->IsWeapon() ) {
		float acc = FireAccuracy( select, type );
		if ( acc > 0.0f ) {
			*tu = FireTimeUnits( select, type );
			if ( *tu > 0.0f )
				GetWeapon()->IsWeapon()->FireStatistics( select, type, acc, distance, 
														 chanceToHit, chanceAnyHit, &damage, damagePerTU );
		}
	}
}


float Unit::FireAccuracy( int select, int type ) const
{
	float acc = 0.0f;

	const Item* item = GetWeapon();
	if ( item && item->IsWeapon() ) {
		const WeaponItemDef* weaponItemDef = item->IsWeapon();
		acc = stats.Accuracy() * weaponItemDef->AccuracyBase( select, type );
	}
	return acc;
}

