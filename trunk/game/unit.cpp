#include "unit.h"
#include "game.h"
#include "../engine/engine.h"


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


void Unit::GenerateTerran( U32 seed )
{
	status = STATUS_ALIVE;
	team = TERRAN_MARINE;
	body = seed;

	const int mfnLen = sizeof(gMaleFirstNames)/sizeof(const char*);
	const int ffnLen = sizeof(gFemaleFirstNames)/sizeof(const char*);
	const int lnLen  = sizeof(gLastNames)/sizeof(const char*);

	grinliz::Random random( seed );
	if ( Gender() == MALE ) {
		firstName = gMaleFirstNames[ random.Rand(mfnLen) ];
	}
	else {
		firstName = gFemaleFirstNames[ random.Rand(ffnLen) ];
	}
	lastName = gLastNames[ random.Rand(lnLen) ];
}


void Unit::GenerateCiv( U32 seed )
{
	status = STATUS_ALIVE;
	team = CIVILIAN;
	body = seed;	// only gender...
	firstName = "";
	lastName = "";
}


void Unit::GenerateAlien( int type, U32 seed )
{
	status = STATUS_ALIVE;
	team = ALIEN;
	body = seed & (~ALIEN_TYPE_MASK);
	body |= (type & ALIEN_TYPE_MASK);

	firstName = "";
	lastName = "";
}



void Unit::FreeModel()
{
	if ( model ) {
		engine->FreeModel( model );
		model = 0;
	}
}


void Unit::SetPos( int x, int z )
{
	if ( model ) {
		grinliz::Vector3F p = { (float)x + 0.5f, 0.0f, (float)z + 0.5f };
		model->SetPos( p );
	}
}

void Unit::CreateModel( Game* game, Engine* engine )
{
	this->game = game;
	this->engine = engine;

	if ( model ) {
		engine->FreeModel( model );
		model = 0;
	}
	if ( status == STATUS_UNUSED ) {
		GLASSERT( 0 );
	}

	ModelResource* resource = 0;
	switch ( team ) {
		case TERRAN_MARINE:
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
	if ( !model || status == STATUS_UNUSED ) {
		return;
	}
	switch ( team ) {
		case TERRAN_MARINE:
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

