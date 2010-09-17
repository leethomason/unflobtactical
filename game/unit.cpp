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

#include "unit.h"
#include "game.h"
#include "../engine/engine.h"
#include "material.h"
#include "../tinyxml/tinyxml.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

// Name first name length: 6
const char* gMaleFirstNames[32] = 
{
	"Lee",		"Jeff",		"Sean",		"Vlad",
	"Arnold",	"Max",		"Otto",		"James",
	"Jason",	"John",		"Hans",		"Rupen",
	"Ping",		"Yoshi",	"Ishal",	"Roy",
	"Eric",		"Jack",		"Dutch",	"Joe",
	"Asher",	"Andrew",	"Adam",		"Thane",
	"Seth",		"Nathan",	"Mal",		"Simon",
	"Joss",		"Mark",		"Luke",		"Alec",
};

// Name first name length: 6
const char* gFemaleFirstNames[32] = 
{
	"Rayne",	"Anne",		"Jade",		"Suzie",
	"Greta",	"Lilith",	"Luna",		"Riko",
	"Jane",		"Sarah",	"Jane",		"Ashley",
	"Liara",	"Rissa",	"Lea",		"Dahlia",
	"Abby",		"Liz",		"Tali",		"Tricia",
	"Gina",		"Zoe",		"Inara",	"River",
	"Ellen",	"Asa",		"Kasumi",	"Tia",
	"Liza",		"Eva",		"Sharon",	"Evie",
};


// Name last name length: 6
const char* gLastNames[32] = 
{
	"Payne",	"Havok",	"Fury",		"Scharz",
	"Bourne",	"Bond",		"Smith",	"Anders",
	"Dekard",	"Jones",	"Solo",		"Skye",
	"Picard",	"Kirk",		"Spock",	"Willis",
	"Red",		"Marsen",	"Baldwin",	"Black",
	"Blume",	"Green",	"Hale",		"Serra",
	"Cobb",		"Frye",		"Book",		"Wedon",
	"Ford",		"Fisher",	"Weaver",	"Hicks",

};


const char* gRank[NUM_RANKS] = {
	"Rki",
	"Prv",
	"Sgt",
	"Maj",
	"Cpt",
};


U32 Unit::GetValue( int which ) const
{
	const int NBITS[] = { 1, 2, 2, 5, 5 };

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
	GLASSERT( strlen( str ) <= 7 );
	return str;
}


const char* Unit::Rank() const
{
	GLASSERT( stats.Rank() >=0 && stats.Rank() < NUM_RANKS ); 
	return gRank[ stats.Rank() ];
}


/*static*/ void Unit::GenStats( int team, int type, int seed, Stats* s )
{
	Vector2I str, dex, psy;

	static const int SOLDIER_MEAN = 50;
	static const int ALIEN_MEAN = 75;
	static const int CIV_MEAN = 25;
	static const int VARIATION = 25;


	switch ( team ) {
		case TERRAN_TEAM:
			str.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN+VARIATION );
			dex.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN+VARIATION );
			psy.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN+VARIATION );
			break;

		case ALIEN_TEAM:
			{
				switch( type ) {
					case 0:
						// Grey. Similar to human, a little weaker & smarter.
						str.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN );
						dex.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN+VARIATION );
						psy.Set( ALIEN_MEAN-VARIATION, ALIEN_MEAN+VARIATION );
						break;
					case 1:
						// Mind-slayer
						str.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN );
						dex.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN+VARIATION );
						psy.Set( ALIEN_MEAN-VARIATION, ALIEN_MEAN+VARIATION  );
						break;
					case 2:
						// Assault
						str.Set( ALIEN_MEAN-VARIATION, ALIEN_MEAN+VARIATION  );
						dex.Set( ALIEN_MEAN-VARIATION, ALIEN_MEAN+VARIATION  );
						psy.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN );
						break;
					case 3:
						// Elite.
						str.Set( ALIEN_MEAN-VARIATION, ALIEN_MEAN+VARIATION );
						dex.Set( ALIEN_MEAN-VARIATION, ALIEN_MEAN+VARIATION );
						psy.Set( ALIEN_MEAN-VARIATION, ALIEN_MEAN+VARIATION );
						break;
					default:
						GLASSERT( 0 );
						break;
				}
			}
			break;

		case CIV_TEAM:
			str.Set( CIV_MEAN, CIV_MEAN );
			dex.Set( CIV_MEAN, CIV_MEAN );
			psy.Set( CIV_MEAN, CIV_MEAN );
			break;

		default:
			GLASSERT( 0 );
			break;
	}

	Random r( seed );
	s->SetSTR( Stats::GenStat( &r, str.x, str.y ) );
	s->SetDEX( Stats::GenStat( &r, dex.x, dex.y ) );
	s->SetPSY( Stats::GenStat( &r, psy.x, psy.y ) );
}


void Unit::Init(	Engine* engine, 
					Game* game, 
					int team,	 
					int p_status,
					int alienType,
					int body )
{
	kills = 0;
	GLASSERT( this->status == STATUS_NOT_INIT );
	this->engine = engine;
	this->game = game;
	this->team = team;
	this->status = p_status;
	this->type = alienType;
	this->body = body;
	GLASSERT( type >= 0 && type < 4 );

	weapon = 0;
	visibilityCurrent = false;
	ai = AI_NORMAL;

	if ( p_status == STATUS_ALIVE ) {
		tu = 1.0f;
		hp = 1;
	}
	else {
		tu = 0;
		hp = 0;
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
	if ( IsAlive() )
		model->SetRotation( rotation );
	UpdateWeapon();
	visibilityCurrent = false;
}


void Unit::SetPos( const grinliz::Vector3F& pos, float rotation )
{
	GLASSERT( status != STATUS_NOT_INIT );
	GLASSERT( model );
	model->SetPos( pos );
	if ( IsAlive() )
		model->SetRotation( rotation );
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

const WeaponItemDef* Unit::GetWeaponDef() const
{
	GLASSERT( status == STATUS_ALIVE );
	const Item* item = GetWeapon();
	if ( item ) 
		return item->GetItemDef()->IsWeapon();
	return 0;
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

	if ( IsAlive() ) {
		const Item* weaponItem = inventory.ArmedWeapon();
		if ( weaponItem  ) {
			weapon = engine->AllocModel( weaponItem->GetItemDef()->resource );
			weapon->SetFlag( Model::MODEL_NO_SHADOW );
			weapon->SetFlag( Model::MODEL_MAP_TRANSPARENT );
		}
		UpdateWeapon();
	}
}


void Unit::UpdateWeapon()
{
	GLASSERT( status == STATUS_ALIVE );
	if ( weapon && model ) {
		Matrix4 r;
		r.SetYRotation( model->GetRotation() );

		Vector4F mPos, gPos, pos4;

		mPos.Set( model->Pos(), 1 );
		gPos.Set( model->GetResource()->header.trigger, 1.0 );
		pos4 = mPos + r*gPos;
		weapon->SetPos( pos4.x, pos4.y, pos4.z );
		weapon->SetRotation( model->GetRotation() );
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
		float r = model->GetRotation() + 45.0f/2.0f;
		int ir = (int)( r / 45.0f );
		*rot = (float)(ir*45);
	}
}


void Unit::Kill( Map* map )
{
	GLASSERT( status == STATUS_ALIVE );
	GLASSERT( model );
	float r = model->GetRotation();
	Vector3F pos = model->Pos();

	Free();

	status = STATUS_KIA;
	if ( team == TERRAN_TEAM ) {
		if ( map->random.Rand( 100 ) < stats.Constitution() )
			status = STATUS_UNCONSCIOUS;
	}
	CreateModel();

	hp = 0;
	model->SetRotation( 0 );	// set the y rotation to 0 for the "splat" icons
	model->SetPos( pos );
	model->SetFlag( Model::MODEL_NO_SHADOW );
	visibilityCurrent = false;

	if ( map && !inventory.Empty() ) {
		Vector2I pos = Pos();
		Storage* storage = map->LockStorage( pos.x, pos.y, game->GetItemDefArr() );
		GLASSERT( storage );

		for( int i=0; i<Inventory::NUM_SLOTS; ++i ) {
			Item* item = inventory.AccessItem( i );
			if ( item->IsSomething() ) {
				storage->AddItem( *item );
				item->Clear();
			}
		}
		map->ReleaseStorage( storage );
	}
}


void Unit::Leave()
{
	if (    status == STATUS_ALIVE
		 || status == STATUS_UNCONSCIOUS ) 
	{
		status = STATUS_MIA;
	}
	// STATUS_KIA no change, of course.
}


void Unit::DoDamage( const DamageDesc& damage, Map* map )
{
	GLASSERT( status != STATUS_NOT_INIT );
	if ( status == STATUS_ALIVE ) {

		int baseArmor = 0;

		if ( team == ALIEN_TEAM ) {
			static const int bonus[NUM_ALIEN_TYPES] = { 0, 1, 3, 2 };
			baseArmor = bonus[ AlienType() ];
		}

		DamageDesc norm;
		inventory.GetDamageReduction( &norm, baseArmor );

		float damageDone = damage.Dot( norm );

		hp = Max( 0, hp-(int)LRintf( damageDone ) );
		if ( hp == 0 ) {
			Kill( map );
			visibilityCurrent = false;
		}
	}
}


void Unit::NewTurn()
{
	if ( status == STATUS_ALIVE ) {
		tu = stats.TotalTU();
	}
}


void Unit::CreateModel()
{
	GLASSERT( status != STATUS_NOT_INIT );

	const ModelResource* resource = 0;
	ModelResourceManager* modman = ModelResourceManager::Instance();
	bool alive = IsAlive();

	if ( alive ) {
		switch ( team ) {
			case TERRAN_TEAM:
				resource = modman->GetModelResource( ( Gender() == MALE ) ? "maleMarine" : "femaleMarine" );
				break;

			case CIV_TEAM:
				resource = modman->GetModelResource( ( Gender() == MALE ) ? "maleCiv" : "femaleCiv" );
				break;

			case ALIEN_TEAM:
				{
					switch( AlienType() ) {
						case 0:	resource = modman->GetModelResource( "alien0" );	break;
						case 1:	resource = modman->GetModelResource( "alien1" );	break;
						case 2:	resource = modman->GetModelResource( "alien2" );	break;
						case 3:	resource = modman->GetModelResource( "alien3" );	break;
						default: GLASSERT( 0 );	break;
					}
				}
				break;
			
			default:
				GLASSERT( 0 );
				break;
		}
		if ( resource ) {
			model = engine->AllocModel( resource );
			model->SetFlag( Model::MODEL_MAP_TRANSPARENT );
		}
	}
	else {
		if ( team != CIV_TEAM ) {
			model = engine->AllocModel( modman->GetModelResource( "unitplate" ) );
			model->SetFlag( Model::MODEL_MAP_TRANSPARENT );
			model->SetFlag( Model::MODEL_NO_SHADOW );

			Texture* texture = TextureManager::Instance()->GetTexture( "particleQuad" );
			model->SetTexture( texture );

			if ( team == TERRAN_TEAM ) {
				model->SetTexXForm( 0.25, 0.25, 0.75f, 0.0f );
			}
			else {
				model->SetTexXForm( 0.25, 0.25, 0.75f, 0.25f );
			}
		}
	}
	UpdateModel();
}


void Unit::UpdateModel()
{
	GLASSERT( status != STATUS_NOT_INIT );

	if ( IsAlive() && model && team == TERRAN_TEAM ) {
		int armor = 0;
		int hair = GetValue( HAIR );
		int skin = GetValue( SKIN );
		model->SetSkin( armor, skin, hair );
	}
}


void Unit::Save( TiXmlElement* doc ) const
{
	if ( status != STATUS_NOT_INIT ) {
		TiXmlElement* unitElement = new TiXmlElement( "Unit" );
		doc->LinkEndChild( unitElement );

		unitElement->SetAttribute( "team", team );
		unitElement->SetAttribute( "type", type );
		unitElement->SetAttribute( "status", status );
		unitElement->SetAttribute( "body", body );
		unitElement->SetAttribute( "hp", hp );
		unitElement->SetAttribute( "kills", kills );
		unitElement->SetDoubleAttribute( "tu", tu );
		unitElement->SetDoubleAttribute( "modelX", model->Pos().x );
		unitElement->SetDoubleAttribute( "modelZ", model->Pos().z );
		unitElement->SetDoubleAttribute( "yRot", model->GetRotation() );

		stats.Save( unitElement );
		inventory.Save( unitElement );
	}
}


void Unit::Load( const TiXmlElement* ele, Engine* engine, Game* game  )
{
	Free();

	// Give ourselves a random body based on the element address. 
	// Hard to get good randomness here (gender bit keeps being
	// consistent.) A combination of the element address and 'this'
	// seems to work pretty well.
	UPTR key[2] = { (UPTR)ele, (UPTR)this };
	Random random( Random::Hash( key, sizeof(UPTR)*2 ));
	random.Rand();
	random.Rand();

	team = TERRAN_TEAM;
	body = random.Rand();
	Vector3F pos = { 0, 0, 0 };
	float rot = 0;
	type = 0;
	int a_status = 0;
	ai = AI_NORMAL;
	kills = 0;

	GLASSERT( StrEqual( ele->Value(), "Unit" ) );
	ele->QueryIntAttribute( "status", &a_status );
	GLASSERT( a_status >= 0 && a_status <= STATUS_MIA );

	if ( a_status != STATUS_NOT_INIT ) {
		ele->QueryIntAttribute( "team", &team );
		ele->QueryIntAttribute( "type", &type );
		ele->QueryIntAttribute( "body", (int*) &body );
		ele->QueryFloatAttribute( "modelX", &pos.x );
		ele->QueryFloatAttribute( "modelZ", &pos.z );
		ele->QueryFloatAttribute( "yRot", &rot );

		GenStats( team, type, body, &stats );		// defaults if not provided
		stats.Load( ele );
		inventory.Load( ele, engine, game );

		Init( engine, game, team, a_status, type, body );

		hp = stats.TotalHP();
		tu = stats.TotalTU();
		// Wait until everything that changes tu and hp have been set
		// before loading, just so we get the correct defaults.
		ele->QueryIntAttribute( "hp", &hp );
		ele->QueryFloatAttribute( "tu", &tu );
		ele->QueryIntAttribute( "kills", &kills );

		if ( StrEqual( ele->Attribute( "ai" ), "guard" ) ) {
			ai = AI_GUARD;
		}

		if ( model ) {
			if ( pos.x == 0.0f ) {
				GLASSERT( pos.z == 0.0f );

				Vector2I pi;
				engine->GetMap()->PopLocation( team, ai == AI_GUARD, &pi, &rot );
				pos.Set( (float)pi.x+0.5f, 0.0f, (float)pi.y+0.5f );
			}

			model->SetPos( pos );
			if ( IsAlive() )
				model->SetRotation( rot );
		}
		UpdateInventory();

#if 0
		GLOUTPUT(( "Unit loaded: team=%d STR=%d DEX=%d PSY=%d rank=%d hp=%d/%d acc=%.2f\n",
					this->team,
					this->stats.STR(),
					this->stats.DEX(),
					this->stats.PSY(),
					this->stats.Rank(),
					hp,
					this->stats.TotalHP(),
					this->stats.AccuracyArea() ));
#endif
	}
}


float Unit::AngleBetween( const Vector2I& p1, bool quantize ) const 
{
	Vector2I p0;
	CalcMapPos( &p0, 0 );

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


bool Unit::HasGunAndAmmo( bool atReady ) const
{
	const WeaponItemDef* wid = GetWeaponDef();
	if ( wid && inventory.CalcClipRoundsTotal( wid->GetClipItemDef( kSnapFireMode ) ) > 0 ) {
		// Unit has a gun. Has ammo. Can shoot something.
		return true;
	}
	if (!atReady) {
		// COULD have a gun, if we get it out of the backpack
		for( int i=0; i<Inventory::NUM_SLOTS; ++i ) {
			Item item = inventory.GetItem( i );
			wid = item.IsWeapon();
			if ( wid && inventory.CalcClipRoundsTotal( wid->GetClipItemDef( kSnapFireMode ) ) > 0 ) {
				return true;
			}
		}
	}
	return false;
}


bool Unit::CanFire( WeaponMode mode ) const
{
	const WeaponItemDef* wid = GetWeaponDef();
	if ( wid ) {
		int nShots = wid->RoundsNeeded( mode );
		float neededTU = FireTimeUnits( mode );
		int rounds = inventory.CalcClipRoundsTotal( wid->GetClipItemDef( mode ) );

		if (    ( neededTU > 0.0f && neededTU <= tu )
			 && rounds >= nShots )
		{
			return true;
		}
	}
	return false;
}


float Unit::FireTimeUnits( WeaponMode mode ) const
{
	float time = 0.0f;

	const WeaponItemDef* wid = GetWeaponDef();
	if ( wid ) {
		time = wid->TimeUnits( mode );
	}
	// Note: don't adjust for stats. TU is based on DEX. Adjusting again double-applies.
	return time;
}


void Unit::AllFireTimeUnits( float *snapTU, float* autoTU, float* altTU )
{
	*snapTU = FireTimeUnits( kSnapFireMode );
	*autoTU = FireTimeUnits( kAutoFireMode );
	*altTU = FireTimeUnits( kAltFireMode );
}


int Unit::CalcWeaponTURemaining( float subtract ) const
{
	const Item* item = GetWeapon();
	if ( !item )
		return NO_WEAPON;
	const WeaponItemDef* wid = item->IsWeapon();
	if ( !wid )
		return NO_WEAPON;

	float snappedTU = 0.0f;
	float autoTU = 0.0f;
	float secondaryTU = 0.0f;
	int select = 0, type = 0;

	snappedTU = FireTimeUnits( kSnapFireMode );
	autoTU = FireTimeUnits( kAutoFireMode );
	secondaryTU = FireTimeUnits( kAltFireMode );

	GLASSERT( secondaryTU >= autoTU );
	GLASSERT( autoTU >= snappedTU );

	float remainingTU = TU() - subtract;

	if ( remainingTU >= secondaryTU )
		return SECONDARY_SHOT;
	else if ( remainingTU >= autoTU )
		return AUTO_SHOT;
	else if ( remainingTU >= snappedTU )
		return SNAP_SHOT;
	return NO_TIME;
}


// Used by the AI
bool Unit::FireStatistics( WeaponMode mode, float distance, 
						   float* chanceToHit, float* chanceAnyHit, float* tu, float* damagePerTU ) const
{
	*chanceToHit = 0.0f;
	*chanceAnyHit = 0.0f;
	*tu = 0.0f;
	*damagePerTU = 0.0f;
	
	float damage;

	const WeaponItemDef* wid = GetWeaponDef();
	if ( wid && wid->HasWeapon( mode ) ) {
		*tu = wid->TimeUnits( mode );
		return GetWeapon()->IsWeapon()->FireStatistics(	mode,
														stats.AccuracyArea(), 
														distance, 
														chanceToHit, 
														chanceAnyHit, 
														&damage, 
														damagePerTU );
	}
	return false;
}


Accuracy Unit::CalcAccuracy( WeaponMode mode ) const
{
	const WeaponItemDef* wid = GetWeaponDef();
	if ( wid && wid->HasWeapon( mode ) ) {
		return wid->CalcAccuracy( stats.AccuracyArea(), mode );
	}
	return Accuracy();
}


