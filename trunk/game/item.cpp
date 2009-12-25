#include "item.h"
#include "material.h"
#include "game.h"
#include "../engine/serialize.h"
#include "gamelimits.h"
#include "../engine/particleeffect.h"
#include "../engine/particle.h"

using namespace grinliz;

void WeaponItemDef::RenderWeapon(	int select,
									ParticleSystem* system,
									const Vector3F& p0, 
									const Vector3F& p1,
									bool useImpact,
									U32 currentTime,
									U32* duration ) const
{
	enum { BEAM, TRAIL, SWING };
	enum { SPLASH, EXPLOSION, NONE };
	int first = BEAM;
	int second = (weapon[select].flags & WEAPON_EXPLOSIVE) ? EXPLOSION : SPLASH;

	// effects: 
	//		bolt then particle
	//		beam

	const float SPEED = 0.02f;	// ray gun settings
	const float WIDTH = 0.3f;
	const float KBOLT = 6.0f;
	const float EBOLT = 1.5f;

	Color4F color = { 1, 1, 1, 1 };
	float speed = SPEED;
	float width = WIDTH;
	float length = 1.0f;

	switch( weapon[select].clipType ) {
		case 0:
			GLASSERT( weapon[select].flags & WEAPON_MELEE );
			first = SWING;
			second = NONE;
			break;

		case ITEM_CLIP_SHELL:
		case ITEM_CLIP_AUTO:
			color.Set( 0.8f, 1.0f, 0.8f, 0.8f );
			speed = SPEED * 2.0f;
			width = WIDTH * 0.5f;
			length = KBOLT;
			break;
		
		case ITEM_CLIP_CELL:
			color.Set( 0.2f, 1.0f, 0.2f, 0.8f );
			speed = SPEED;
			width = WIDTH;
			length = EBOLT;
			break;

		case ITEM_CLIP_FLAME:
			color.Set( 1.0f, 0, 0, 0.8f );
			speed = SPEED * 0.7f;
			width = WIDTH;
			length = EBOLT;
			break;

		case ITEM_CLIP_ROCKET:
		case ITEM_CLIP_GRENADE:
			color.Set( 1, 0, 0, 1 );
			first = TRAIL;
			break;

		default:
			GLASSERT( 0 );	// not set yet
	}

	if ( first == BEAM ) {
		BoltEffect* bolt = (BoltEffect*) system->EffectFactory( "BoltEffect" );
		if ( !bolt ) { 
			bolt = new BoltEffect( system );
		}
		
		bolt->Clear();
		bolt->SetColor( color );
		bolt->SetSpeed( speed );
		bolt->SetLength( length );
		bolt->SetWidth( width );
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
		trail->SetSpeed( speed*0.4f );
		trail->Init( p0, p1, currentTime );
		*duration = trail->CalcDuration();
		system->AddEffect( trail );
	}
	else if ( first == SWING ) {
		GLASSERT( 0 );
	}
	else {
		GLASSERT( 0 );
	}

	if ( useImpact && second != NONE ) {

		ImpactEffect* impact = (ImpactEffect*) system->EffectFactory( "ImpactEffect" );
		if ( !impact ) {
			impact = new ImpactEffect( system );
		}

		Vector3F n = p0 - p1;
		n.Normalize();
		impact->Clear();
		impact->Init( p1, currentTime + *duration );
		impact->SetColor( color );
		impact->SetNormal( n );
		if ( second == SPLASH ) {
			impact->SetRadius( 1.5f );
		}
		else {
			// explosion
			impact->SetRadius( 3.5f );
			impact->SetConfig( ParticleSystem::PARTICLE_SPHERE );
		}
		system->AddEffect( impact );
	}
}


bool WeaponItemDef::CompatibleClip( const ItemDef* id, int* which ) const
{
	if ( id->IsClip() ) {
		if ( weapon[0].clipType == id->type ) {
			*which = 0;
			return true;
		}
		else if ( weapon[1].clipType == id->type ) {
			*which = 1;
			return true;
		}
	}
	return false;
}


void WeaponItemDef::DamageBase( int select, DamageDesc* d ) const
{
	switch( weapon[select].clipType ) {
		case 0:								// melee
		case ITEM_CLIP_SHELL:
		case ITEM_CLIP_AUTO:
			d->Set( 1, 0, 0 );
			break;

		case ITEM_CLIP_CELL:
			if ( weapon[select].flags & WEAPON_EXPLOSIVE ) {
				d->Set( 0.5f, 0.5f, 0 );
			}
			else {
				d->Set( 0, 1, 0 );
			}
			break;

		case ITEM_CLIP_FLAME:
			d->Set( 0, 0, 1 );
			break;

		case ITEM_CLIP_ROCKET:
			d->Set( 0.5f, 0, 0.5f );
			break;

		case ITEM_CLIP_GRENADE:
			d->Set( 0.8f, 0, 0.2f );
			break;

		default:
			GLASSERT( 0 );	// need to implement
	}
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


void WeaponItemDef::FireStatistics( int select, int type, 
								    float accuracy, float distance, 
								    float* chanceToHit, float* totalDamage, float* damagePerTU ) const
{
	*chanceToHit = 0.0f;
	*damagePerTU = 0.0f;
	*totalDamage = 0.0f;
	float tu = TimeUnits( select, type );
	DamageDesc dd;

	if ( tu > 0.0f ) {
		float targetRad = distance * accuracy * AccuracyBase( select, type );
		*chanceToHit = STANDARD_TARGET_AREA / targetRad;
		if ( *chanceToHit > 0.98f )
			*chanceToHit = 0.98f;

		DamageBase( select, &dd );
		*totalDamage = dd.Total();

		*damagePerTU = (*chanceToHit) * (*totalDamage) / tu;
		if ( type == AUTO_SHOT )
			*damagePerTU *= 3.0f;
	}
}

void ItemPart::Init( const ItemDef* itemDef, int rounds )
{
	this->itemDef = itemDef;
	if ( rounds < 0 ) {
		const ClipItemDef* cid = itemDef->IsClip();
		if ( cid ) 
			this->rounds = cid->rounds;
		else
			this->rounds = 1;
	}
	else {
		this->rounds = rounds;
	}
}


Item::Item( const ItemDef* itemDef, int rounds )
{
	part[0].Init( itemDef, rounds );
	part[1].Clear();
	part[2].Clear();
}

Item::Item( Game* game, const char* name, int rounds )
{
	part[0].Init( game->GetItemDef( name ), rounds );
	part[1].Clear();
	part[2].Clear();
}


bool Item::Insert( const Item& withThis )
{
	Item item( withThis );
	bool consumed;
	return Combine( &item, &consumed );
}


void Item::RemovePart( int i, Item* item )
{
	GLASSERT( i>0 && i<3 );
	item->part[0] = part[i];
	item->part[1].Clear();
	item->part[2].Clear();
	part[i].Clear();
}

void Item::Clear()
{
	for( int i=0; i<3; ++i )
		part[i].Clear();
}


bool Item::Combine( Item* with, bool* consumed )
{
	int which = 0;
	const WeaponItemDef* wid = part[0].IsWeapon();
	bool result = false;

	if (	wid 
		 && wid->CompatibleClip( with->part[0].itemDef, &which ) ) 
	{
		// weapon0,1 maps to part 1,2
		which++;

		// The 'with' is the clip, and is defined by with.part[0].
		// this->part[0] is a weapon
		// this->part[1] is the primary fire
		// this->part[2] is the secondary fire
		//
		const ClipItemDef* clipDef = with->part[0].itemDef->IsClip();
		GLASSERT( clipDef );

		// The clip *type* is compatible (ITEM_CLIP_CANON), but maybe not
		// the specific ammo (MC-AC). Check that.
		if (    part[which].None()								// ammo not yet set
			 || part[which].itemDef == with->part[0].itemDef )	// same itemDef means the same exact type.
		{
			// then we have a match.

			if ( part[which].None() ) {
				// need to create the part to add to
				part[which].Init( with->part[0].itemDef, 0 );
			}

			int roundsThatFit = clipDef->rounds - part[which].rounds;
			GLASSERT( roundsThatFit >= 0 );

			if ( with->part[0].rounds > roundsThatFit ) {
				part[which].rounds += roundsThatFit;
				with->part[0].rounds -= roundsThatFit;
				*consumed = false;
				result = true;
			}
			else { 
				part[which].rounds += with->part[0].rounds;
				with->part[0].rounds = 0;
				*consumed = true;
				result = true;
			}	
		}
	}
	return result;
}


int Item::RoundsAvailable( int i ) const
{
	GLASSERT( i==1 || i==2 )
	GLASSERT( this->IsWeapon() );

	if ( i == 1 )
		return this->Rounds(1);
	else if ( i == 2 && this->IsWeapon()->weapon[1].clipType == ITEM_CLIP_CELL )
		// then use the cell in slot 1
		return this->Rounds(1);
	else 
		// use the rounds in slot 2
		return this->Rounds(2);
}


int Item::RoundsRequired( int i ) const 
{
	GLASSERT( i==1 || i==2 )
	GLASSERT( this->IsWeapon() );

	const WeaponItemDef* wid = this->IsWeapon();

	if ( wid->weapon[i-1].clipType == ITEM_CLIP_CELL )
		return wid->weapon[i-1].power;
	return 1;
}


void Item::UseRound( int i ) 
{
	GLASSERT( i==1 || i==2 )
	GLASSERT( this->IsWeapon() );

	const WeaponItemDef* wid = this->IsWeapon();

	if ( wid->weapon[i-1].clipType == ITEM_CLIP_CELL ) {
		int power = wid->weapon[i-1].power;
		GLASSERT( part[1].IsClip() );
		GLASSERT( power <= part[1].rounds );
		part[1].rounds -= power;
	}
	else {
		GLASSERT( part[i].IsClip() );
		GLASSERT( part[i].rounds > 0 );
		part[i].rounds--;
	}
}


void Item::Save( UFOStream* s ) const
{
	s->WriteU8( 1 );	// version
	for( int i=0; i<NUM_PARTS; ++i ) {
		if ( part[i].itemDef ) {
			s->WriteU8( 1 );
			s->WriteStr( part[i].itemDef->name );
			s->WriteU8( part[i].rounds );
		}
		else {
			s->WriteU8( 0 );
		}
	}
}


void Item::Load( UFOStream* s, Engine* engine, Game* game )
{
	Clear();

	int version = s->ReadU8();
	GLASSERT( version == 1 );

	for( int i=0; i<NUM_PARTS; ++i ) {
		int filled = s->ReadU8();
		if ( filled ) {
			const char* name = s->ReadStr();
			const ItemDef* itemDef = game->GetItemDef( name );
			GLASSERT( itemDef );
			part[i].itemDef = itemDef;
			part[i].rounds = s->ReadU8();
		}
	}
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
	for( int i=0; i<3; ++i ) {
		if ( item.GetItemDef( i ) ) {
			int index = GetIndex( item.GetItemDef(i) );
			rounds[index] += item.Rounds(i);
		}
	}
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


// Return the "best" item for on-screen rendering.
/*const ItemDef* Storage::SomeItem() const
{
	int bestScore = -1;
	const ItemDef* best = 0;

	for( unsigned i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( rounds[i] > 0 ) {
			int score = rounds[i];
			
			if ( itemDefs[i]->IsWeapon() )
				score *= 20;

			if ( score > bestScore ) {
				bestScore = score;
				best = itemDefs[i];
			}
		}
	}
	return best;
}
*/

/*
void Storage::Save( UFOStream* s ) const
{
	s->WriteU32Arary( itemDefs.Size(), (const U32*) rounds.Mem() );
}

void Storage::Load( UFOStream* s )
{
	s->ReadU32Arary( itemDefs.Size(), (U32*) rounds.Mem() );
}
*/
