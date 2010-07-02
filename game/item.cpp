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


void WeaponItemDef::RenderWeapon(	WeaponMode mode,
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
	int select = Index( mode );

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


void WeaponItemDef::DamageBase( WeaponMode mode, DamageDesc* d ) const
{
	int select = Index( mode );
	GLASSERT( weapon[select].clipItemDef );
	*d = weapon[select].clipItemDef->dd;
	d->Scale( weapon[select].damage );
}


float WeaponItemDef::TimeUnits( WeaponMode mode ) const
{
	float s = 0.0f;
	switch ( mode ) {
		case kModeSnap:		
			s = TU_SNAP_SHOT;	
			break;
		case kModeAuto:		
			if ( weapon[0].flags & WEAPON_AUTO )
				s = TU_AUTO_SHOT;	
			else
				s = TU_AIMED_SHOT;
			break;
		case kModeAlt:	
			s = TU_AIMED_SHOT;	
			break;
		default:
			GLASSERT( 0 );
	}
	
	s *= speed;

	// Secondary weapon is slower:
	if ( mode == kModeAlt )
		s *= SECONDARY_SHOT_SPEED_MULT;

	return s;
}


float WeaponItemDef::AccuracyBase( WeaponMode mode ) const
{
	float acc = weapon[Index(mode)].accuracy;

	switch ( mode ) {
	case kModeSnap:
		acc *= ACC_SNAP_SHOT_MULTIPLIER;
		break;
	case kModeAuto:
		if ( weapon[0].flags & WEAPON_AUTO )
			acc *= ACC_AUTO_SHOT_MULTIPLIER;
		else 
			acc *= ACC_AIMED_SHOT_MULTIPLIER;
	case kModeAlt:
		acc *= ACC_AIMED_SHOT_MULTIPLIER;			// secondary fire is aimed? FIXME: consider this.
		break;
	}
	return acc;
}


bool WeaponItemDef::FireStatistics( WeaponMode mode,
								    float accuracyRadius, 
									float distance, 
									float* chanceToHit, float* anyChanceToHit,
									float* totalDamage, float* damagePerTU ) const
{
	*chanceToHit = 0.0f;
	*damagePerTU = 0.0f;
	*totalDamage = 0.0f;
	float tu = TimeUnits( mode );
	DamageDesc dd;

	if ( tu > 0.0f ) {
		float radius = distance * accuracyRadius * AccuracyBase( mode );
		float area   = PI * radius * radius;

		/*
		// This is an approximation of a correct rectangle-circle intersection.
		float targetArea = 0.0f;
		if ( radius*2.0f <= STANDARD_TARGET_W ) {
			targetArea = area;	// 100% chance...
		}
		else if ( radius*2.0f <= STANDARD_TARGET_H ) {
			targetArea = STANDARD_TARGET_W * (radius * 2.0f);	// approximation...and a poor one
		}
		else {
			targetArea = STANDARD_TARGET_W * STANDARD_TARGET_H;
		}
		*/
		float targetArea = STANDARD_TARGET_W * STANDARD_TARGET_H;

		*chanceToHit = targetArea / area;
		if ( *chanceToHit > 1.0f )
			*chanceToHit = 1.0f;

		*anyChanceToHit = *chanceToHit;
		int nRounds = RoundsNeeded( mode );

		if ( nRounds == 3 ) {
			float chanceMiss = (1.0f - (*chanceToHit));
			*anyChanceToHit = 1.0f - chanceMiss*chanceMiss*chanceMiss;
		}

		DamageBase( mode, &dd );
		*totalDamage = dd.Total();

		*damagePerTU = (*chanceToHit) * (*totalDamage) / tu;

		*damagePerTU *= (float)nRounds;
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
	GLASSERT( name && *name );
	if ( !name || !*name )
		return;
	
	itemDef = game->GetItemDef( name );
	rounds = itemDef->DefaultRounds();

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
	int r = grinliz::Min( rounds[index], _itemDef->DefaultRounds() );

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
	rounds[index] = count*itemDef->DefaultRounds();
}


int Storage::GetCount( const ItemDef* itemDef) const
{
	int index = GetIndex( itemDef );
	int r = rounds[index];
	return (r+itemDef->DefaultRounds()-1)/itemDef->DefaultRounds();
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
				itemDefArr[i]->IsWeapon()->DamageBase( kModeAuto, &d );

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
		return ModelResourceManager::Instance()->GetModelResource( best->resource->header.name.c_str() );
	}
	return ModelResourceManager::Instance()->GetModelResource( "smallcrate" );
}
