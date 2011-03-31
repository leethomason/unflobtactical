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
#include "ai.h"
#include "ufosound.h"
#include "tacmap.h"
#include "../engine/loosequadtree.h"

using namespace grinliz;

// Name first name length: 6
const char* gMaleFirstNames[64] = 
{
	"Lee",		"Jeff",		"Sean",		"Vlad",
	"Arnold",	"Max",		"Otto",		"James",
	"Jason",	"John",		"Hans",		"Rupen",
	"Ping",		"Yoshi",	"Ishal",	"Roy",
	"Eric",		"Jack",		"Dutch",	"Joe",
	"Asher",	"Andrew",	"Adam",		"Thane",
	"Seth",		"Nathan",	"Mal",		"Simon",
	"Joss",		"Mark",		"Luke",		"Alec",
	"Robert",	"David",	"Jules",	"Paul",
	"George",	"Ken",		"Steve",	"Ed",
	"Duke",		"Ron",		"Tony",		"Kevin",
	"Gary",		"Jose",		"Scott",	"Josh",
	"Peter",	"Carl",		"Juan",		"Will",
	"Aaron",	"Victor",	"Todd",		"Earl",
	"Manuel",	"Kyle",		"Calvin",	"Leo",
	"Andre",	"Brad",		"Javier",	"Tyrone"
};

// Name first name length: 6
const char* gFemaleFirstNames[64] = 
{
	"Rayne",	"Anne",		"Jade",		"Suzie",
	"Greta",	"Lilith",	"Luna",		"Riko",
	"Jane",		"Sarah",	"Jane",		"Ashley",
	"Liara",	"Rissa",	"Lea",		"Dahlia",
	"Abby",		"Liz",		"Tali",		"Tricia",
	"Gina",		"Zoe",		"Inara",	"River",
	"Ellen",	"Asa",		"Kasumi",	"Tia",
	"Donna",	"Eva",		"Sharon",	"Evie",
	"Maria",	"Lisa",		"Kim",		"Jess",
	"Amy",		"Angela",	"Kate",		"Nicole",
	"Julia",	"Paula",	"Dawn",		"Sally",
	"Alicia",	"Yvonne",	"Vivian",	"Alma",
	"Vera",		"Heidi",	"Gwen",		"Sonia",
	"Miriam",	"Violet",	"Misty",	"Claire",
	"Isabel",	"Iris",		"Amelia",	"Hanna",
	"Kari",		"Freda",	"Jena",		"Olive"

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


struct AlienDef
{
	const char* name;
	float	kinetic;	// armor
	float	energy;
	float	incin;
	int		str;
	int		dex;
	int		psy;
};

static const int NUM_ALIENS = 5;

AlienDef gAlienDef[NUM_ALIENS] = {
	{ "green",	ARM0, ARM0, ARM0,				30,	60, 50	},
	{ "prime",	ARM2, ARM3, ARM2,				60, 70, 90  },
	{ "hornet",	ARM1, ARM2, ARM2,				40, 65, 55  },
	{ "jackal",	ARM3, ARM2, ARM1,				80, 50, 80  },
	{ "viper",	ARM2, ARM1, ARM2,				70, 70, 70  }
};


/*static*/ int Unit::XPToRank( int xp )
{
	GLASSERT( NUM_RANKS == 5 );
	if      ( xp > 25 ) return 4;
	else if ( xp > 12 ) return 3;
	else if ( xp > 5 )  return 2;
	else if ( xp > 1 )  return 1;
	return 0;
}


U32 Unit::GetValue( int which ) const
{
	const int NBITS[NUM_TRAITS] = { 1, 4, 5, 6 };

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
	static const int CIV_MEAN = 25;
	static const int VARIATION = 25;

	str.Set( 0, 0 );
	dex.Set( 0, 0 );
	psy.Set( 0, 0 );

	switch ( team ) {
		case TERRAN_TEAM:
			str.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN+VARIATION );
			dex.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN+VARIATION );
			psy.Set( SOLDIER_MEAN-VARIATION, SOLDIER_MEAN+VARIATION );
			break;

		case ALIEN_TEAM:
			str.Set( gAlienDef[type].str, gAlienDef[type].str );
			dex.Set( gAlienDef[type].dex, gAlienDef[type].dex );
			psy.Set( gAlienDef[type].psy, gAlienDef[type].psy );
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


void Unit::Init(	int team,	 
					int p_status,
					int alienType,
					int body )
{
	kills = 0;
	nMissions = 0;
	allMissionKills = 0;
	allMissionOvals = 0;
	gunner = 0;
	model = 0;

	GLASSERT( this->status == STATUS_NOT_INIT );
//	this->game = game;
	tree = 0;
	this->team = team;
	this->status = p_status;
	this->type = alienType;
	this->body = body & 0x7fffffff;	// strip off the high bit. Makes serialization odd to deal with negative numbers.
	GLASSERT( type >= 0 && type < Unit::NUM_ALIEN_TYPES );
	
	random.SetSeed( body ^ ((U32)this) );
	random.Rand();
	random.Rand();

	weapon = 0;
	visibilityCurrent = false;
	
	if ( team == TERRAN_TEAM )
		ai = AI::AI_TRAVEL;
	else
		ai = AI::AI_NORMAL;

	if ( p_status == STATUS_ALIVE ) {
		tu = 1.0f;
		hp = 1;
	}
	else {
		tu = 0;
		hp = 0;
	}
}


Unit::~Unit()
{
	Free();
}


void Unit::Free()
{
	if ( status == STATUS_NOT_INIT )
		return;
	FreeModels();
	status = STATUS_NOT_INIT;
}


void Unit::FreeModels() 
{
	if ( status == STATUS_NOT_INIT )
		return;

	if ( model ) {
		GLASSERT( tree );
		tree->FreeModel( model );
		model = 0;
	}
	if ( weapon ) {
		GLASSERT( tree );
		tree->FreeModel( weapon );
		weapon = 0;
	}
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
	//GLASSERT( status == STATUS_ALIVE );
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

	if ( weapon  ) {
		GLASSERT( tree );
		tree->FreeModel( weapon );
	}
	weapon = 0;	// don't render non-weapon items

	if ( IsAlive() ) {
		const Item* weaponItem = inventory.ArmedWeapon();
		if ( weaponItem && tree ) {
			weapon = tree->AllocModel( weaponItem->GetItemDef()->resource );
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


void Unit::Kill( TacMap* map )
{
	GLASSERT( status == STATUS_ALIVE );
	
	bool hasModel = false;
	Vector3F pos = { 0, 0, 0 };
	if ( model ) { 
		hasModel = true;
		pos = model->Pos();
	}

	Free();

	status = STATUS_KIA;
	hp = 0;

	if ( team == TERRAN_TEAM ) {
		if ( hasModel )
			SoundManager::Instance()->QueueSound( "terrandown0" );
		if ( random.Rand( 100 ) < stats.Constitution() )
			status = STATUS_UNCONSCIOUS;
	}
	else if ( team == ALIEN_TEAM ) {
		if ( hasModel )
			SoundManager::Instance()->QueueSound( "aliendown0" );
	}
	if ( hasModel ) {
		CreateModel();

		model->SetRotation( 0 );	// set the y rotation to 0 for the "splat" icons
		model->SetPos( pos );
		model->SetFlag( Model::MODEL_NO_SHADOW );
	}
	visibilityCurrent = false;

	if ( map && !inventory.Empty() ) {
		Vector2I pos = Pos();
		Storage* storage = map->LockStorage( pos.x, pos.y );
		GLASSERT( storage );

		for( int i=0; i<Inventory::NUM_SLOTS; ++i ) {
			Item* item = inventory.AccessItem( i );
			if ( item->IsSomething() ) {
				storage->AddItem( item->GetItemDef() );
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


void Unit::DoDamage( const DamageDesc& damage, TacMap* map )
{
	GLASSERT( status != STATUS_NOT_INIT );
	if ( status == STATUS_ALIVE ) {

		DamageDesc norm;

		if ( team == ALIEN_TEAM ) {
			norm.Set( gAlienDef[AlienType()].kinetic, gAlienDef[AlienType()].energy, gAlienDef[AlienType()].incin );
		}
		else {
			inventory.GetDamageReduction( &norm );
		}

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
	GLASSERT( tree );

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
				resource = modman->GetModelResource( gAlienDef[AlienType()].name );
				break;
			
			default:
				GLASSERT( 0 );
				break;
		}
		GLASSERT( resource );
		if ( resource ) {
			model =tree->AllocModel( resource );
			model->SetFlag( Model::MODEL_MAP_TRANSPARENT );
		}
	}
	else {
		model = tree->AllocModel( modman->GetModelResource( "unitplate" ) );
		model->SetFlag( Model::MODEL_MAP_TRANSPARENT );
		model->SetFlag( Model::MODEL_NO_SHADOW );

		Texture* texture = TextureManager::Instance()->GetTexture( "particleQuad" );
		model->SetTexture( texture );

		if ( team != ALIEN_TEAM ) {
			model->SetTexXForm( 0, 0.25f, 0.25f, 0.75f, 0.0f );
		}
		else {
			model->SetTexXForm( 0, 0.25f, 0.25f, 0.75f, 0.25f );
		}
	}
	GLASSERT( model );

	model->SetPos( initPos );
	if ( IsAlive() )
		model->SetRotation( initRot );
	UpdateModel();
}


void Unit::UpdateModel()
{
	GLASSERT( status != STATUS_NOT_INIT );

	if ( IsAlive() && model && team == TERRAN_TEAM ) {
		int armor = inventory.GetArmorLevel();
		int appearance = GetValue( APPEARANCE );
		int gender = GetValue( GENDER );
		model->SetSkin( gender, armor, appearance );
	}
}


void Unit::Save( FILE* fp, int depth ) const
{
	if ( status != STATUS_NOT_INIT ) {
		XMLUtil::OpenElement( fp, depth, "Unit" );

		XMLUtil::Attribute( fp, "team", team );
		XMLUtil::Attribute( fp, "type", type );
		XMLUtil::Attribute( fp, "status", status );
		XMLUtil::Attribute( fp, "body", body );
		XMLUtil::Attribute( fp, "hp", hp );
		XMLUtil::Attribute( fp, "kills", kills );
		XMLUtil::Attribute( fp, "nMissions", nMissions );
		XMLUtil::Attribute( fp, "allMissionKills", allMissionKills );
		XMLUtil::Attribute( fp, "allMissionOvals", allMissionOvals );
		XMLUtil::Attribute( fp, "gunner", gunner );
		XMLUtil::Attribute( fp, "tu", tu );
		if ( ai == AI::AI_GUARD )
			XMLUtil::Attribute( fp, "ai", "guard" );

		if ( model ) {
			XMLUtil::Attribute( fp, "modelX", model->Pos().x );
			XMLUtil::Attribute( fp, "modelZ", model->Pos().z );
			XMLUtil::Attribute( fp, "yRot", model->GetRotation() );
		}

		XMLUtil::SealElement( fp );

		stats.Save( fp, depth+1 );
		inventory.Save( fp, depth+1 );

		XMLUtil::CloseElement( fp, depth, "Unit" );
	}
}


void Unit::InitModel( SpaceTree* tree, TacMap* tacmap )
{
	this->tree = tree;

	if ( initPos.x == 0.0f ) {
		GLASSERT( initPos.z == 0.0f );

		Vector2I pi;
		tacmap->PopLocation( team, ai == AI::AI_GUARD, &pi, &initRot );
		initPos.Set( (float)pi.x+0.5f, 0.0f, (float)pi.y+0.5f );
	}

	CreateModel();
	UpdateInventory();
}


void Unit::Load( const TiXmlElement* ele, const ItemDefArr& itemDefArr )
{
	Free();

	// Give ourselves a random body based on the element address. 
	// Hard to get good randomness here (gender bit keeps being
	// consistent.) A combination of the element address and 'this'
	// seems to work pretty well.
	random.SetSeed( body ^ ((U32)ele) ^ ((U32)this) );
	random.Rand();
	random.Rand();

	team = TERRAN_TEAM;
	body = random.Rand() & 0x7fffffff;
	initPos.Set( 0, 0, 0 );
	initRot = 0;
	type = 0;
	int a_status = 0;
	ai = AI::AI_NORMAL;
	kills = 0;
	nMissions = 0;
	allMissionKills = 0;
	allMissionOvals = 0;
	gunner = 0;

	GLASSERT( StrEqual( ele->Value(), "Unit" ) );
	ele->QueryIntAttribute( "status", &a_status );
	GLASSERT( a_status >= 0 && a_status <= STATUS_MIA );

	if ( a_status != STATUS_NOT_INIT ) {
		ele->QueryIntAttribute( "team", &team );
		ele->QueryIntAttribute( "type", &type );
		ele->QueryIntAttribute( "body", (int*) &body );
		ele->QueryFloatAttribute( "modelX", &initPos.x );
		ele->QueryFloatAttribute( "modelZ", &initPos.z );
		ele->QueryFloatAttribute( "yRot", &initRot );

		GenStats( team, type, body, &stats );		// defaults if not provided
		stats.Load( ele );
		inventory.Load( ele, itemDefArr );

		Init( team, a_status, type, body );

		hp = stats.TotalHP();
		tu = stats.TotalTU();
		// Wait until everything that changes tu and hp have been set
		// before loading, just so we get the correct defaults.
		ele->QueryIntAttribute( "hp", &hp );
		ele->QueryFloatAttribute( "tu", &tu );
		
		ele->QueryIntAttribute( "kills", &kills );
		ele->QueryIntAttribute( "nMissions", &nMissions );
		ele->QueryIntAttribute( "allMissionKills", &allMissionKills );
		ele->QueryIntAttribute( "allMissionOvals", &allMissionOvals );
		ele->QueryIntAttribute( "gunner", &gunner );

		if ( StrEqual( ele->Attribute( "ai" ), "guard" ) ) {
			ai = AI::AI_GUARD;
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


void Unit::Create(	int team,
					int alienType,
					int rank,
					int seed )
{
	Free();
	Init( team, STATUS_ALIVE, alienType, seed );
	GenStats( team, type, body, &stats );		// defaults if not provided
	allMissionKills = rank / 2;
	allMissionOvals = rank / 2;
	gunner = 0;
	stats.SetRank( XPToRank( XP() ));

	hp = stats.TotalHP();
	tu = stats.TotalTU();
	UpdateInventory();
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
	//int select = 0, type = 0;

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
bool Unit::FireStatistics(	WeaponMode mode, 
							const BulletTarget& target,
							float* chanceToHit, float* chanceAnyHit, float* tu, float* damagePerTU ) const
{
	*chanceToHit = 0.0f;
	*chanceAnyHit = 0.0f;
	*tu = 0.0f;
	*damagePerTU = 0.0f;
	bool result = false;
	
	float damage;

	const WeaponItemDef* wid = GetWeaponDef();
	if ( wid && wid->HasWeapon( mode ) ) {
		*tu = wid->TimeUnits( mode );
		result = GetWeapon()->IsWeapon()->FireStatistics(	mode,
															stats.AccuracyArea(), 
															target,
															chanceToHit, 
															chanceAnyHit, 
															&damage, 
															damagePerTU );
	}
	GLASSERT( InRange( *chanceToHit, 0.0f, 1.0f ) );
	GLASSERT( InRange( *chanceAnyHit, 0.0f, 1.0f ) );
	return result;
}


Accuracy Unit::CalcAccuracy( WeaponMode mode ) const
{
	const WeaponItemDef* wid = GetWeaponDef();
	if ( wid && wid->HasWeapon( mode ) ) {
		return wid->CalcAccuracy( stats.AccuracyArea(), mode );
	}
	return Accuracy();
}


