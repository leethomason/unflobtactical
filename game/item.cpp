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
//#include "game.h"
#include "../engine/model.h"
#include "../engine/serialize.h"
#include "gamelimits.h"
#include "../engine/particleeffect.h"
#include "../engine/particle.h"
#include "../tinyxml/tinyxml.h"
#include "../grinliz/glstringutil.h"
#include "stats.h"

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
	int select = Index( mode );

	const ClipItemDef* cid = weapon[select].clipItemDef;

	int first = (weapon[select].flags & WEAPON_EXPLOSIVE) ? TRAIL : BEAM;
	int second = (weapon[select].flags & WEAPON_EXPLOSIVE) ? EXPLOSION : SPLASH;

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

		if ( IsAlien() ) {
			trail->SetQuad( ParticleSystem::CIRCLE, 1 );
			c = cid->color;
		}

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

	if ( weapon[select].flags & WEAPON_INCINDIARY ) {
		if ( d->incind < 0.5f ) {
			d->incind = 0.5f;
			d->Normalize();
		}
	}

	d->Scale( weapon[select].damage );
}


float WeaponItemDef::TimeUnits( WeaponMode mode ) const
{
	float s = 0.0f;
	switch ( mode ) {
		case kSnapFireMode:		
			s = TU_SNAP_SHOT;	
			break;
		case kAutoFireMode:		
			if ( weapon[0].flags & WEAPON_AUTO )
				s = TU_AUTO_SHOT;	
			else
				s = TU_AIMED_SHOT;
			break;
		case kAltFireMode:	
			s = TU_AIMED_SHOT;	
			break;
		default:
			GLASSERT( 0 );
	}
	
	s *= speed;

	// Secondary weapon is slower:
	if ( mode == kAltFireMode )
		s *= SECONDARY_SHOT_SPEED_MULT;

	return s;
}


Accuracy WeaponItemDef::CalcAccuracy( float accuracyArea, WeaponMode mode ) const
{
	float acc = weapon[Index(mode)].accuracy;

	switch ( mode ) {
	case kSnapFireMode:
		acc *= ACC_SNAP_SHOT_MULTIPLIER;
		break;
	case kAutoFireMode:
		if ( weapon[0].flags & WEAPON_AUTO )
			acc *= ACC_AUTO_SHOT_MULTIPLIER;
		else 
			acc *= ACC_AIMED_SHOT_MULTIPLIER;
	case kAltFireMode:
		acc *= ACC_AIMED_SHOT_MULTIPLIER;			// secondary fire is aimed? FIXME: consider this.
		break;
	}
	// a = PR2, R = sqrt(a/P)
	float radius = sqrtf( acc * accuracyArea / 3.14f );
	return Accuracy( radius );
}


bool WeaponItemDef::FireStatistics( WeaponMode mode,
								    float accuracyArea, 
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
		Accuracy acc = CalcAccuracy( accuracyArea, mode );
		//if( StrEqual( this->name, "PST" ) ) {
		//	GLOUTPUT(( "AccArea=%.2f rad=%.2f\n", accuracyArea, acc.RadiusAtOne() ));
		//}

		BulletSpread bulletSpread;
		*chanceToHit = bulletSpread.ComputePercent( acc, distance );

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


Item::Item( const ItemDefArr& itemDefArr, const char* name, int rounds )
{
	const ItemDef* itemDef = itemDefArr.Query( name );
	this->itemDef = itemDef;
	this->rounds = rounds;
}


void Item::UseRounds( int i ) 
{
	GLASSERT( i <= rounds );
	rounds -= i;
}


void Item::Save( FILE* fp, int depth ) const
{
	if ( itemDef && rounds > 0 ) {
		XMLUtil::OpenElement( fp, depth, "Item" );
		XMLUtil::Attribute( fp, "name", itemDef->name );
		if ( rounds != 1 ) {
			XMLUtil::Attribute( fp, "rounds", rounds );
		}
		XMLUtil::SealCloseElement( fp );
	}
}


void Item::Load( const TiXmlElement* ele, const ItemDefArr& itemDefArr )
{
	Clear();
	GLASSERT( StrEqual( ele->Value(), "Item" ) );
	const char* name = ele->Attribute( "name" );
	GLASSERT( name && *name );
	if ( !name || !*name )
		return;
	
	itemDef = itemDefArr.Query( name );
	GLASSERT( itemDef );
	rounds = itemDef->DefaultRounds();

	ele->QueryIntAttribute( "rounds", &rounds );
}




Storage::~Storage()
{
}


bool Storage::Empty() const
{
	for( int i=0; i<itemDefArr.Size(); ++i ) {
		if ( rounds[i] )
			return false;
	}
	return true;
}


void Storage::AddItem( const Item& item )
{
	int index = item.GetItemDef()->index;
	rounds[index] += item.Rounds();
}


void Storage::RemoveItem( const ItemDef* _itemDef, Item* _item )
{
	int index = _itemDef->index;
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


bool Storage::Contains( const ItemDef* def ) const
{
	if ( !def )
		return false;
	return GetCount( def ) > 0;

}



const WeaponItemDef* Storage::IsResupply( const WeaponItemDef* weapon ) const
{
	if ( weapon ) {
		const ClipItemDef* clip0 = weapon->GetClipItemDef( kSnapFireMode );
		const ClipItemDef* clip1 = weapon->GetClipItemDef( kAltFireMode );

		if ( GetCount( clip0 ) || GetCount( clip1 ) )
			return weapon;
	}

	for( int i=0; i<itemDefArr.Size(); ++i ) {
		const ItemDef* itemDef = itemDefArr.Query( i );

		if ( itemDef && itemDef->IsWeapon() && GetCount( itemDef ) ) {
			const ClipItemDef* clip0 = itemDef->IsWeapon()->GetClipItemDef( kSnapFireMode );
			const ClipItemDef* clip1 = itemDef->IsWeapon()->GetClipItemDef( kAltFireMode );
			if ( GetCount( clip0 ) || GetCount( clip1 ) )
				return itemDef->IsWeapon();
		}
	}
	return 0;
}


void Storage::SetCount( const ItemDef* itemDef, int count )
{
	int index = itemDef->index;
	rounds[index] = count*itemDef->DefaultRounds();
}


int Storage::GetCount( const ItemDef* itemDef) const
{
	if ( !itemDef )
		return 0;

	int index = itemDef->index;
	int r = rounds[index];
	return (r+itemDef->DefaultRounds()-1)/itemDef->DefaultRounds();
}


void Storage::Save( FILE* fp, int depth )
{
	XMLUtil::OpenElement( fp, depth, "Storage" );
	XMLUtil::SealElement( fp );

	for( int i=0; i<itemDefArr.Size(); ++i ) {
		if ( rounds[i] > 0 ) {
			XMLUtil::OpenElement( fp, depth+1, "Rounds" );
			XMLUtil::Attribute( fp, "name", itemDefArr.Query(i)->name );
			XMLUtil::Attribute( fp, "n", rounds[i] );
			XMLUtil::SealCloseElement( fp );
		}
	}

	XMLUtil::CloseElement( fp, depth, "Storage" );
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
			int i=-1, n=0;
			if ( roundElement->Attribute( "name" ) ) {
				const ItemDef* itemDef = itemDefArr.Query( roundElement->Attribute( "name" ) );
				if ( itemDef ) 
					i = itemDef->index;
			}
			else {
				roundElement->QueryIntAttribute( "i", &i );
			}
			roundElement->QueryIntAttribute( "n", &n );
			if ( i>=0 && i<EL_MAX_ITEM_DEFS ) {
				rounds[i] = n;
			}
		}
	}
}


// Return the "best" item for on-screen rendering.
const ModelResource* Storage::VisualRep( bool* zRotate ) const
{
	float bestScore = 0;
	const ItemDef* best = 0;
	*zRotate = false;

	for( int i=0; i<itemDefArr.Size(); ++i ) {
		if ( rounds[i] > 0 ) {
			const ItemDef* itemDef = itemDefArr.Query( i );
			if ( itemDef && itemDef->IsWeapon() ) {
				DamageDesc d;
				itemDef->IsWeapon()->DamageBase( kAutoFireMode, &d );

				float score = (float)rounds[i] * d.Total();
				
				if ( score > bestScore ) {
					bestScore = score;
					best = itemDef;
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


ItemDefArr::ItemDefArr()
{
	memset( arr, 0, sizeof(ItemDef*)*EL_MAX_ITEM_DEFS );
	nItemDef = 0;
}


ItemDefArr::~ItemDefArr()
{
	for( int i=0; i<nItemDef; ++i ) {
		delete arr[i];
	}
}


void ItemDefArr::Add( ItemDef* itemDef )
{
	GLASSERT( itemDef );
	GLASSERT( nItemDef < EL_MAX_ITEM_DEFS );
	
	arr[nItemDef] = itemDef;
	map.Add( itemDef->name, itemDef );

	itemDef->index = nItemDef;
	nItemDef++;
}


const ItemDef* ItemDefArr::Query( const char* name ) const
{
	ItemDef* result = 0;
	if ( map.Query( name, &result ) )
		return result;
	return 0;
}


const ItemDef* ItemDefArr::Query( int id ) const
{
	GLASSERT( id >= 0 && id < EL_MAX_ITEM_DEFS );
	return arr[id];
}
