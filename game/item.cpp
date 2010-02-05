#include "item.h"
#include "material.h"
#include "game.h"
#include "../engine/serialize.h"
#include "gamelimits.h"
#include "../engine/particleeffect.h"
#include "../engine/particle.h"
#include "../tinyxml/tinyxml.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;


bool WeaponItemDef::IsAlien() const 
{ 
	return weapon[0].clipItemDef->IsAlien(); 
}


void WeaponItemDef::RenderWeapon(	int select,
									ParticleSystem* system,
									const Vector3F& p0, 
									const Vector3F& p1,
									bool useImpact,
									U32 currentTime,
									U32* duration ) const
{
	enum { BEAM, TRAIL };
	enum { SPLASH, EXPLOSION };
	int first, second;

	const ClipItemDef* cid = weapon[select].clipItemDef;

	if ( cid->IsAlien() ) {
		first = BEAM;
		second = (weapon[select].flags & WEAPON_EXPLOSIVE) ? EXPLOSION : SPLASH;
	}
	else {
		first = (weapon[select].flags & WEAPON_EXPLOSIVE) ? TRAIL : BEAM;
		second = (weapon[select].flags & WEAPON_EXPLOSIVE) ? EXPLOSION : SPLASH;
	}

	// effects: 
	//		bolt then particle
	//		beam

	if ( first == BEAM ) {
		BoltEffect* bolt = (BoltEffect*) system->EffectFactory( "BoltEffect" );
		if ( !bolt ) { 
			bolt = new BoltEffect( system );
		}
		
		bolt->Clear();
		bolt->SetColor( cid->color );
		bolt->SetSpeed( cid->speed );
		bolt->SetLength( cid->length );
		bolt->SetWidth( cid->width );
		bolt->Init( p0, p1, currentTime );

		*duration = bolt->CalcDuration();
		system->AddEffect( bolt );
	}
	else if ( first == TRAIL ) {
		SmokeTrailEffect* trail = (SmokeTrailEffect*) system->EffectFactory( "SmokeTrailEffect" );
		if ( !trail ) {
			trail = new SmokeTrailEffect( system );
		}

		trail->Clear();
		Color4F c = { 1, 1, 1, 1 };
		trail->SetColor( c );
		trail->SetSpeed( cid->speed*0.4f );
		trail->Init( p0, p1, currentTime );
		*duration = trail->CalcDuration();
		system->AddEffect( trail );
	}
	else {
		GLASSERT( 0 );
	}

	if ( useImpact ) {

		ImpactEffect* impact = (ImpactEffect*) system->EffectFactory( "ImpactEffect" );
		if ( !impact ) {
			impact = new ImpactEffect( system );
		}

		Vector3F n = p0 - p1;
		n.Normalize();
		impact->Clear();
		impact->Init( p1, currentTime + *duration );
		impact->SetColor( cid->color );
		impact->SetNormal( n );
		if ( second == SPLASH ) {
			impact->SetRadius( 1.5f );
		}
		else {
			// explosion
			impact->SetRadius( 3.5f );
			impact->SetConfig( ParticleSystem::PARTICLE_SPHERE );

			// 2nd set of particles:
			ImpactEffect* impact2 = (ImpactEffect*) system->EffectFactory( "ImpactEffect" );
			if ( !impact2 ) {
				impact2 = new ImpactEffect( system );
			}
			impact2->Clear();
			impact2->Init( p1, currentTime + *duration + 250 );
			impact2->SetColor( cid->color );
			impact2->SetNormal( n );
			impact2->SetRadius( 3.5f );
			impact2->SetConfig( ParticleSystem::PARTICLE_SPHERE );
			system->AddEffect( impact2 );
		}
		system->AddEffect( impact );
	}
}


bool WeaponItemDef::CompatibleClip( const ItemDef* id, int* which ) const
{
	if ( id->IsClip() ) {
		if ( weapon[0].clipItemDef == id ) {
			*which = 0;
			return true;
		}
		else if ( weapon[1].clipItemDef == id ) {
			*which = 1;
			return true;
		}
	}
	return false;
}


void WeaponItemDef::DamageBase( int select, DamageDesc* d ) const
{
	GLASSERT( weapon[select].clipItemDef );
	*d = weapon[select].clipItemDef->dd;
	d->Scale( weapon[select].damage );
}


float WeaponItemDef::TimeUnits( int select, int type ) const
{
	GLASSERT( select == 0 || select == 1 );
	GLASSERT( type >= 0 && type < 3 );

	float s = 0.0f;
	switch ( type ) {
		case SNAP_SHOT:		
			s = TU_SNAP_SHOT;	
			break;
		case AUTO_SHOT:		
			if ( weapon[select].flags & WEAPON_AUTO )
				s = TU_AUTO_SHOT;	
			break;
		case AIMED_SHOT:	
			s = TU_AIMED_SHOT;	
			break;
		default:
			GLASSERT( 0 );
	}
	
	s *= speed;

	// Secondary weapon is slower:
	if ( select == 1 )
		s *= SECONDARY_SHOT_SPEED_MULT;

	return s;
}


float WeaponItemDef::AccuracyBase( int select, int type ) const
{
	GLASSERT( select >= 0 && type >= 0 && select < 2 && type < 3 );

	static const float MULT[3] = { ACC_SNAP_SHOT_MULTIPLIER, ACC_AUTO_SHOT_MULTIPLIER, ACC_AIMED_SHOT_MULTIPLIER };
	float acc = weapon[select].accuracy * MULT[type];
	return acc;
}


void WeaponItemDef::FireModeToType( int mode, int* select, int* type ) const 
{
	GLASSERT( mode >= 0 && mode < 3 );
	if ( mode == 0 ) {
		*select = 0;
		*type = SNAP_SHOT;
	}
	else if ( mode == 1 ) {
		if ( SupportsType( 0, AUTO_SHOT ) ) {
			*select = 0;
			*type = AUTO_SHOT;
		}
		else {
			GLASSERT( SupportsType( 0, AIMED_SHOT ) );
			*select = 0;
			*type = AIMED_SHOT;
		}
	}
	else {
		*select = 1;
		*type = AIMED_SHOT;
	}
}


bool WeaponItemDef::SupportsType( int select, int type ) const
{ 
	if ( select == 0 ) {
		if ( type == SNAP_SHOT )
			return true;
		else if ( type == AUTO_SHOT && (weapon[0].flags & WEAPON_AUTO) )
			return true;
		else if ( type == AIMED_SHOT && !(weapon[0].flags & WEAPON_AUTO) )
			return true;
	}
	else if ( select ==1 ) {
		if ( type == AIMED_SHOT && HasWeapon( select ) )
			return true;
	}
	return false;
}


bool WeaponItemDef::FireStatistics( int select, int type, 
								    float accuracy, float distance, 
									float* chanceToHit, float* anyChanceToHit,
									float* totalDamage, float* damagePerTU ) const
{
	GLASSERT( SupportsType( select, type ) );

	*chanceToHit = 0.0f;
	*damagePerTU = 0.0f;
	*totalDamage = 0.0f;
	float tu = TimeUnits( select, type );
	DamageDesc dd;

	if ( tu > 0.0f ) {
		float targetRad = distance * accuracy * AccuracyBase( select, type );
		float targetArea = PI * targetRad*targetRad;

		*chanceToHit = STANDARD_TARGET_AREA / targetArea;
		if ( *chanceToHit > 0.98f )
			*chanceToHit = 0.98f;

		*anyChanceToHit = *chanceToHit;
		int nRounds = (type==AUTO_SHOT) ? 3 : 1;

		if ( nRounds == 3 ) {
			float chanceMiss = (1.0f-*chanceToHit);
			*anyChanceToHit = 1.0f - chanceMiss*chanceMiss*chanceMiss;
		}

		DamageBase( select, &dd );
		*totalDamage = dd.Total();

		*damagePerTU = (*chanceToHit) * (*totalDamage) / tu;
		if ( type == AUTO_SHOT )
			*damagePerTU *= 3.0f;
		return true;
	}
	return false;
}


Item::Item( const ItemDef* itemDef, int rounds )
{
	this->itemDef = itemDef;
	this->rounds = rounds;
}


Item::Item( Game* game, const char* name, int rounds )
{
	const ItemDef* itemDef = game->GetItemDef( name );
	this->itemDef = itemDef;
	this->rounds = rounds;
}


void Item::UseRounds( int i ) 
{
	GLASSERT( i <= rounds );
	rounds -= i;
}


void Item::Save( TiXmlElement* doc ) const
{
	if ( itemDef && rounds > 0 ) {
		TiXmlElement* itemEle = new TiXmlElement( "Item" );
		doc->LinkEndChild( itemEle );
		itemEle->SetAttribute( "name", itemDef->name );
		if ( rounds != 1 )
			itemEle->SetAttribute( "rounds", rounds );
	}
}


void Item::Load( const TiXmlElement* ele, Engine* engine, Game* game )
{
	Clear();
	GLASSERT( StrEqual( ele->Value(), "Item" ) );
	const char* name = ele->Attribute( "name" );
	
	itemDef = game->GetItemDef( name );
	rounds = 1;
	ele->QueryIntAttribute( "rounds", &rounds );
}




Storage::~Storage()
{
}


bool Storage::Empty() const
{
	for( int i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( rounds[i] )
			return false;
	}
	return true;
}


void Storage::AddItem( const Item& item )
{
	int index = GetIndex( item.GetItemDef() );
	rounds[index] += item.Rounds();
}


void Storage::RemoveItem( const ItemDef* _itemDef, Item* _item )
{
	int index = GetIndex( _itemDef );
	int r = grinliz::Min( rounds[index], _itemDef->Rounds() );

	if ( r == 0 ) {
		_item->Clear();
	}
	else {
		Item item( _itemDef, r );
		rounds[index] -= r;
		*_item = item;
	}
}


void Storage::SetCount( const ItemDef* itemDef, int count )
{
	int index = GetIndex( itemDef );
	rounds[index] = count*itemDef->Rounds();
}


int Storage::GetCount( const ItemDef* itemDef) const
{
	int index = GetIndex( itemDef );
	int r = rounds[index];
	return (r+itemDef->Rounds()-1)/itemDef->Rounds();
}


void Storage::Save( TiXmlElement* parent )
{
	TiXmlElement* storageElement = new TiXmlElement( "Storage" );
	parent->LinkEndChild( storageElement );

	for( int i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( rounds[i] > 0 ) {
			TiXmlElement* roundElement = new TiXmlElement( "Rounds" );
			roundElement->SetAttribute( "i", i );
			roundElement->SetAttribute( "n", rounds[i] );
			storageElement->LinkEndChild( roundElement );
		}
	}
}


void Storage::Load( const TiXmlElement* element )
{
	memset( rounds, 0, sizeof(int)*EL_MAX_ITEM_DEFS );
	const TiXmlElement* storageElement = element->FirstChildElement( "Storage" );
	GLASSERT( storageElement );
	if ( storageElement ) {
		for( const TiXmlElement* roundElement = storageElement->FirstChildElement( "Rounds" );
			 roundElement;
			 roundElement = roundElement->NextSiblingElement( "Rounds" ) )
		{
			int i=0, n=0;
			roundElement->QueryIntAttribute( "i", &i );
			roundElement->QueryIntAttribute( "n", &n );
			GLASSERT( i>=0 && i<EL_MAX_ITEM_DEFS );
			if ( i>=0 && i<EL_MAX_ITEM_DEFS ) {
				rounds[i] = n;
			}
		}
	}
}


// Return the "best" item for on-screen rendering.
const ModelResource* Storage::VisualRep( ItemDef* const* itemDefArr, bool* zRotate ) const
{
	float bestScore = 0;
	const ItemDef* best = 0;
	*zRotate = false;

	for( unsigned i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( rounds[i] > 0 ) {

			if ( itemDefArr[i]->IsWeapon() ) {
				DamageDesc d;
				itemDefArr[i]->IsWeapon()->DamageBase( 0, &d );

				float score = (float)rounds[i] * d.Total();
				
				if ( score > bestScore ) {
					bestScore = score;
					best = itemDefArr[i];
				}
			}
		}
	}
	if ( best ) {
		*zRotate = true;
		return ModelResourceManager::Instance()->GetModelResource( best->resource->header.name );
	}
	return ModelResourceManager::Instance()->GetModelResource( "smallcrate" );
}
