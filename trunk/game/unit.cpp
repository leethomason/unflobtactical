#include "unit.h"
#include "game.h"
#include "../engine/engine.h"
#include "material.h"
#include "../tinyxml/tinyxml.h"
#include "../grinliz/glstringutil.h"

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
	if ( team == TERRAN_TEAM ) {
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
	if ( team == TERRAN_TEAM ) {
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
	team = TERRAN_TEAM;
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
	team = CIV_TEAM;
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
	team = ALIEN_TEAM;
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
					int status,
					int alienType,	// if alien...
					U32 seed )
{
	GLASSERT( this->status == STATUS_NOT_INIT );
	this->engine = engine;
	this->game = game;
	this->team = team;
	this->status = status;
	weapon = 0;
	visibilityCurrent = false;

	switch( team ) {
		case TERRAN_TEAM:	GenerateSoldier( seed );			break;
		case ALIEN_TEAM:		GenerateAlien( alienType, seed );	break;
		case CIV_TEAM:	GenerateCiv( seed );				break;
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


const Inventory* Unit::GetInventory() const
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
		case TERRAN_TEAM:
			if ( alive )
				resource = modman->GetModelResource( ( Gender() == MALE ) ? "maleMarine" : "femaleMarine" );
			else
				resource = modman->GetModelResource( ( Gender() == MALE ) ? "maleMarine_D" : "femaleMarine_D" );
			break;

		case CIV_TEAM:
			if ( alive )
				resource = modman->GetModelResource( ( Gender() == MALE ) ? "maleCiv" : "femaleCiv" );
			else
				resource = modman->GetModelResource( ( Gender() == MALE ) ? "maleCiv_D" : "femaleCiv_D" );
			break;

		case ALIEN_TEAM:
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
		case TERRAN_TEAM:
			{
				int armor = 0;
				int hair = GetValue( HAIR );
				int skin = GetValue( SKIN );
				model->SetSkin( armor, skin, hair );
			}
			break;

		case CIV_TEAM:
			break;

		case ALIEN_TEAM:
			break;
		
		default:
			GLASSERT( 0 );
			break;
	};
}


void Unit::Save( TiXmlElement* doc ) const
{
	if ( status != STATUS_NOT_INIT ) {
		TiXmlElement* unitElement = new TiXmlElement( "Unit" );
		doc->LinkEndChild( unitElement );

		unitElement->SetAttribute( "team", team );
		unitElement->SetAttribute( "status", status );
		unitElement->SetAttribute( "body", body );
		unitElement->SetDoubleAttribute( "modelX", model->Pos().x );
		unitElement->SetDoubleAttribute( "modelZ", model->Pos().z );
		unitElement->SetDoubleAttribute( "yRot", model->GetYRotation() );

		stats.Save( unitElement );
		inventory.Save( unitElement );
	}
}


void Unit::Load( const TiXmlElement* ele, Engine* engine, Game* game  )
{
	Free();

	team = TERRAN_TEAM;
	body = 0;
	Vector3F pos = { 0, 0, 0 };
	float rot = 0;
	int _status = 0;

	GLASSERT( StrEqual( ele->Value(), "Unit" ) );
	ele->QueryIntAttribute( "status", &_status );
	GLASSERT( _status == STATUS_NOT_INIT || _status == STATUS_ALIVE || _status == STATUS_DEAD );

	if ( _status != STATUS_NOT_INIT ) {
		ele->QueryValueAttribute( "team", &team );
		ele->QueryValueAttribute( "body", &body );
		ele->QueryValueAttribute( "modelX", &pos.x );
		ele->QueryValueAttribute( "modelZ", &pos.z );
		ele->QueryValueAttribute( "yRot", &rot );

		Init( engine, game, team, _status, body&ALIEN_MASK, body );
		
		if ( model ) {
			model->SetPos( pos );
			model->SetYRotation( rot );
		}

		stats.Load( ele );
		inventory.Load( ele, engine, game );
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


bool Unit::CanFire( int select, int type ) const
{
	int nShots = (type == AUTO_SHOT) ? 3 : 1;
	float tu = FireTimeUnits( select, type );

	if ( tu > 0.0f && tu <= stats.TU() ) {
		int rounds = inventory.CalcClipRoundsTotal( GetWeapon()->IsWeapon()->weapon[select].clipItemDef );
		if ( rounds >= nShots ) 
			return true;
	}
	return false;
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


bool Unit::FireModeToType( int mode, int* select, int* type ) const
{
	if ( GetWeapon() && GetWeapon()->IsWeapon() ) {
		GetWeapon()->IsWeapon()->FireModeToType( mode, select, type );
		return true;
	}
	return false;
}


bool Unit::FireStatistics( int select, int type, float distance, float* chanceToHit, float* chanceAnyHit, float* tu, float* damagePerTU ) const
{
	*chanceToHit = 0.0f;
	*tu = 0.0f;
	*damagePerTU = 0.0f;
	float damage;

	if ( GetWeapon() && GetWeapon()->IsWeapon() ) {
		float acc = FireAccuracy( select, type );
		if ( acc > 0.0f ) {
			*tu = FireTimeUnits( select, type );
			if ( *tu > 0.0f ) {
				return GetWeapon()->IsWeapon()->FireStatistics(	select, type, acc, distance, 
																chanceToHit, chanceAnyHit, &damage, damagePerTU );
			}
		}
	}
	return false;
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

