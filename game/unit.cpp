#include "unit.h"
#include "game.h"
#include "../engine/engine.h"

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
}


void Unit::GenerateCiv( U32 seed )
{
	status = STATUS_ALIVE;
	team = CIVILIAN;
	body = seed;	// only gender...
}


void Unit::GenerateAlien( int type, U32 seed )
{
	status = STATUS_ALIVE;
	team = ALIEN;
	body = seed & (~ALIEN_TYPE_MASK);
	body |= (type & ALIEN_TYPE_MASK);
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

	switch( team ) {
		case SOLDIER:	GenerateSoldier( seed );			break;
		case ALIEN:		GenerateAlien( alienType, seed );	break;
		case CIVILIAN:	GenerateCiv( seed );				break;
		default:
			GLASSERT( 0 );
	}
	CreateModel();
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
	status = STATUS_NOT_INIT;
}


void Unit::SetPos( int x, int z )
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );

	grinliz::Vector3F p = { (float)x + 0.5f, 0.0f, (float)z + 0.5f };
	model->SetPos( p );
}


void Unit::CalcPos( grinliz::Vector3F* vec )
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );

	*vec = model->Pos();
}


void Unit::CreateModel()
{
	ModelResource* resource = 0;
	switch ( team ) {
		case SOLDIER:
			resource = game->GetResource( ( Gender() == MALE ) ? "maleMarine" : "femaleMarine" );
			break;

		case CIVILIAN:
			resource = game->GetResource( ( Gender() == MALE ) ? "maleCiv" : "femaleCiv" );
			break;

		case ALIEN:
			{
				switch( AlienType() ) {
					case 0:	resource = game->GetResource( "alien0" );	break;
					case 1:	resource = game->GetResource( "alien1" );	break;
					case 2:	resource = game->GetResource( "alien2" );	break;
					case 3:	resource = game->GetResource( "alien3" );	break;
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
	status = s->ReadU8();
	GLASSERT( status == STATUS_NOT_INIT || status == STATUS_ALIVE || status == STATUS_DEAD );
	if ( status != STATUS_NOT_INIT ) {
		team = s->ReadU8();
		body = s->ReadU32();

		Init( engine, game, team, ((body>>ALIEN_TYPE_SHIFT) & ALIEN_TYPE_MASK), body );
		
		Vector3F pos = { 0, 0, 0 };
		float rot;

		pos.x = s->ReadFloat();
		pos.z = s->ReadFloat();
		rot   = s->ReadFloat();

		model->SetPos( pos );
		model->SetYRotation( rot );
	}
}

