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

#ifndef UFOATTACK_ITEM_INCLUDED
#define UFOATTACK_ITEM_INCLUDED

#include <string.h>
#include "../grinliz/glvector.h"
#include "../grinliz/glcolor.h"
#include "../engine/ufoutil.h"
#include "../engine/enginelimits.h"
#include "../engine/vertex.h"
#include "../shared/glmap.h"
#include "gamelimits.h"
#include "stats.h"
#include "../grinliz/glstringutil.h"

class ModelResource;
class TiXmlElement;
class Engine;
class ParticleSystem;
struct MapDamageDesc;


enum {
	WEAPON_AUTO			= 0x01,
	WEAPON_EXPLOSIVE	= 0x02,
	WEAPON_INCENDIARY	= 0x04,	// adds incendiary damage regardless of the ammo type
};


class WeaponItemDef;
class ClipItemDef;
class ArmorItemDef;


struct DamageDesc
{
	float kinetic;
	float energy;
	float incend;

	float Total() const						{ return kinetic + energy + incend; }
	void Clear()							{ kinetic = energy = incend = 0.0f; }
	void Set( float k, float e, float i )	{ kinetic = k; energy = e; incend = i; }
	void Scale( float x )					{
												kinetic *= x;
												energy *= x;
												incend *= x;
											}
	void Normalize()						{
												float lenInv = 1.0f / ( kinetic + energy + incend );	// not a 2D length.
												kinetic *= lenInv;
												energy *= lenInv;
												incend *= lenInv;
											}

	float Dot( const DamageDesc& other ) const	{ return other.kinetic*kinetic + other.energy*energy + other.incend*incend; }
	void MapDamage( MapDamageDesc* );
};


class ItemDef
{
public:
	ItemDef() {};
	virtual ~ItemDef() {};

	const char*		name;	// the regular string name
	const char*		desc;	// extended description
	int				deco;
	int				price;
	bool			isAlien;
	int				maxCount;	// maximum allowed count
	const ModelResource*	resource;		// can be null, in which case render with crate.
	grinliz::CStr<6>		displayName;	// the short, formatted name for button display

	void InitBase( const char* name, const char* desc, int deco, int price, bool isAlien, const ModelResource* resource );
	virtual const WeaponItemDef* IsWeapon() const { return 0; }
	virtual const ClipItemDef* IsClip() const  { return 0; }
	virtual const ArmorItemDef* IsArmor() const { return 0; }
	bool IsAlien() const { return isAlien; }
	
	// Most items are 1 thing: a gun, armor, etc. But clips are formed of collections.
	// It may take 10 rounds to form a clip.
	virtual int DefaultRounds() const	{ return 1; }

	int TechLevel() const {
		if ( strstr( name, "-3" ) ) return 3;
		else if ( strstr( name, "-2" ) ) return 2;
		else if ( strstr( name, "-1" ) ) return 1;
		return 0;
	}
	// optimization trickiness:
	int index;

	int BuyPrice( float costMult ) const {
		if ( price >= 0 )
			return grinliz::LRintf( (float)price * costMult );
		else
			return -price;
	}

	int SellPrice() const {
		// Fixes an exploit where you can hire a soldier, sell his stuff,
		// and then fire the soldier to make money.
		if ( grinliz::StrEqual( name, "Soldr" ) ) return grinliz::LRintf( abs( price ) * 0.15f );
		return abs( price );
	}

	void PrintDesc( char*, int nChar ) const;
};


// So ItemDefs can be passed around without the entire Game.
class ItemDefArr
{
public:
	ItemDefArr();
	~ItemDefArr();

	void Add( ItemDef* );
	const ItemDef* Query( const char* name ) const;
	const ItemDef* Query( int id ) const;
	int Size() const								{ return nItemDef; }
	const ItemDef* GetIndex( int i ) const			{ GLASSERT( i >= 0 && i < EL_MAX_ITEM_DEFS ); return arr[i]; }

private:
	int							nItemDef;
	ItemDef*					arr[EL_MAX_ITEM_DEFS];
	CStringMap< ItemDef* >		map;
};

/*
enum WeaponMode {
	kSnapFireMode,
	kAutoFireMode,
	kAltFireMode
};
*/

class WeaponItemDef : public ItemDef
{
public:

	virtual const WeaponItemDef* IsWeapon() const { return this; }

	enum { MAX_MODE=5 };

	struct Weapon {
		const char* desc;
		const char* clipItemDefName;
		int flags;			// WEAPON_AUTO, etc.
		float damage;		// damage done by weapon, 1.0 is normal
		float accuracy;		// 1.0 is average
		float tu;
		const char* sound;
	};

	const char*	fireDesc[3];	// name of the 3 fire modes
	const Weapon*		weapon[MAX_MODE];
	const ClipItemDef*	clipItemDef[MAX_MODE];

//	int Index( WeaponMode mode ) const								{ return ( mode == kAltFireMode ) ? 1 : 0; }

	bool HasWeapon( int mode) const									{ return weapon[mode] != 0; }
	const ClipItemDef* GetClipItemDef( int mode ) const				{ return clipItemDef[mode]; }
	int RoundsNeeded( int mode ) const								{ return ( weapon[mode]->flags & WEAPON_AUTO ) ? 3 : 1; }
	bool IsExplosive( int mode ) const								{ return ( weapon[mode]->flags & WEAPON_EXPLOSIVE ) != 0; }

	void RenderWeapon(	int mode,
						ParticleSystem*,
						const grinliz::Vector3F& p0, 
						const grinliz::Vector3F& p1,
						bool impact,
						U32 currentTime,
						U32* doneTime ) const;

	bool CompatibleClip( const ItemDef* itemDef, int* which ) const;
	
	// Basic damage for this weapon.
	void DamageBase( int mode, DamageDesc* damageArray ) const;
	// Amount of time it takes to use this weapon. (Does not depend on the Unit.)
	float TimeUnits( int mode ) const;
	
	// Accuracy of the weapon. 1.0 is normal, higher is worse.
	Accuracy CalcAccuracy( float unitAccuracy, int mode ) const;

	static void AddAccData( float predicted, bool hit );
	static void CurrentAccData( float* predicted, float* actual );

	// Statistics for this weapon. 
	bool FireStatistics( int mode, 
						 float unitAccuracy, 
						 const BulletTarget& target,
						 float* chanceToHit,				// chance a round hits
						 float* chanceAnyHit,				// chance any round (of 1 or 3) hits
						 float* totalDamage,			
						 float* damagePerTU ) const;		// damagePerTU = f( damage, TU, accuracyRadius, distance )

private:
	static float accPredicted;
	static int accHit;
	static int accTotal;
};


class ClipItemDef : public ItemDef
{
public:

	virtual const ClipItemDef* IsClip() const { return this; }
	
	virtual int DefaultRounds() const	{ return defaultRounds; }

	bool alien;

	int defaultRounds;
	DamageDesc dd;

	char	abbreviation;
	grinliz::Color4F color;
	float speed;
	float width;
	float length;
};


class ArmorItemDef : public ItemDef
{
public:
	virtual const ArmorItemDef* IsArmor() const { return this; }
};


/* POD.
*/
class Item
{
public:
	Item()								{ rounds = 0; itemDef = 0; }
	Item( const Item& rhs )				{ rounds = rhs.rounds; itemDef = rhs.itemDef; }
	Item( const ItemDef* itemDef, int rounds=0 );
	Item( const ItemDefArr&, const char* name, int rounds=0 );

	void operator=( const Item& rhs )	{ rounds = rhs.rounds; itemDef = rhs.itemDef; }

	// wipe this item
	void Clear()									{ rounds = 0; itemDef = 0; }

	const ItemDef* GetItemDef() const				{ return itemDef; }
	const WeaponItemDef* IsWeapon() const			{ return itemDef ? itemDef->IsWeapon() : 0; }
	const ClipItemDef* IsClip() const				{ return itemDef ? itemDef->IsClip() : 0; }
	const ArmorItemDef* IsArmor() const				{ return itemDef ? itemDef->IsArmor() : 0; }


	// --- handle clips (and only clips) ----//
	void UseRounds( int n=1 );
	int Rounds() const								{ return rounds; }

	const ClipItemDef* ClipType( int select ) const	{	GLASSERT( IsWeapon() );
														return IsWeapon()->clipItemDef[select];
													}

	const char* Name() const						{ return itemDef->name; }
	const char* Desc() const						{ return itemDef->desc; }
	const char* DisplayName() const					{ return itemDef->displayName.c_str(); }
	int Deco() const								{ return itemDef->deco; }

	bool IsNothing() const							{ return itemDef == 0; }
	bool IsSomething() const						{ return itemDef != 0; }

	void Save( FILE* fp, int depth ) const;
	void Load( const TiXmlElement* doc, const ItemDefArr& itemDefArr );

private:
	int rounds;
	const ItemDef* itemDef;
};


// A complete list of all possible items and how many there are.
class Storage
{
public:
	Storage( int _x, int _y, const ItemDefArr& _itemDefArr ) : x( _x ), y( _y ), itemDefArr( _itemDefArr )	{	memset( rounds, 0, sizeof(int)*EL_MAX_ITEM_DEFS ); }
	Storage( const Storage& rhs ) : x( rhs.x ), y( rhs.y ), itemDefArr( rhs.itemDefArr )					{	memcpy( rounds, rhs.rounds, sizeof(int)*EL_MAX_ITEM_DEFS ); }
	void operator=( const Storage& rhs )	{
		this->x = rhs.x;
		this->y = rhs.y;
		//this->itemDefArr = rhs.itemDefArr;	
		GLASSERT( &this->itemDefArr == &rhs.itemDefArr );
		memcpy( rounds, rhs.rounds, sizeof(int)*EL_MAX_ITEM_DEFS );
	}

	~Storage();

	void Clear();
	bool Empty() const;

	const ItemDef* GetItemDef( int i ) const {
		if ( i < itemDefArr.Size() )
			return itemDefArr.GetIndex( i );
		return 0;
	}

	void AddItem( const ItemDef*, int n=1 );
	void AddItem( const char* itemName, int n=1 );
	void AddItem( const Item& item );				// doesn't change the item passed in (it is constant) but the caller must handle the item is consumed
	void AddStorage( const Storage& storage );
	void SetFullRounds();							// refill clips

	bool RemoveItem( const ItemDef*, Item* item );	// returns true if successful
	void ClearItem( const char* name );
	bool Contains( const ItemDef* ) const;

	// Return true if either is true:
	// 1. This storage contains a clip (of either type) for 'weapon',
	// 2. This storage contains any weapon and rounds for that weapon
	// Returns the weapon def that is re-supplied.
	const WeaponItemDef* IsResupply( const WeaponItemDef* weapon ) const;
	
	int GetCount( const ItemDef* ) const;	// the number of items, corrected for the rounds
	int GetCount( int index ) const;

	void Save( FILE* fp, int depth );
	void Load( const TiXmlElement* mapNode );

	const ModelResource* VisualRep( bool* zRotate ) const;

	int X() const { return x; }
	int Y() const { return y; }

	const ItemDefArr& GetItemDefArr() const { return itemDefArr; }

private:
	int x, y;
	const ItemDefArr& itemDefArr;
	int rounds[EL_MAX_ITEM_DEFS];
};


#endif // UFOATTACK_ITEM_INCLUDED
