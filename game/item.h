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
#include "../engine/ufoutil.h"
#include "../engine/enginelimits.h"
#include "../engine/vertex.h"
#include "gamelimits.h"
#include "stats.h"

class ModelResource;
class TiXmlElement;
class Engine;
class Game;
class ParticleSystem;


enum {
	WEAPON_AUTO			= 0x01,
	WEAPON_EXPLOSIVE	= 0x02,
	WEAPON_INCINDIARY	= 0x04,	// adds incindiary damage regardless of the ammo type
};


class WeaponItemDef;
class ClipItemDef;
class ArmorItemDef;


struct DamageDesc
{
	float kinetic;
	float energy;
	float incind;

	float Total() const						{ return kinetic + energy + incind; }
	void Clear()							{ kinetic = energy = incind = 0.0f; }
	void Set( float k, float e, float i )	{ kinetic = k; energy = e; incind = i; }
	void Scale( float x )					{
												kinetic *= x;
												energy *= x;
												incind *= x;
											}
	void Normalize()						{
												float lenInv = 1.0f / ( kinetic + energy + incind );	// not a 2D length.
												kinetic *= lenInv;
												energy *= lenInv;
												incind *= lenInv;
											}

	float Dot( const DamageDesc& other ) const	{ return other.kinetic*kinetic + other.energy*energy + other.incind*incind; }
};


class ItemDef
{
public:
	ItemDef() {};
	virtual ~ItemDef() {};

	const char*		name;
	const char*		desc;
	int				deco;
	const ModelResource*	resource;	// can be null, in which case render with crate.

	void InitBase( /*int type, */ const char* name, const char* desc, int deco, const ModelResource* resource, int index )
	{
		//GLASSERT( type >= ITEM_WEAPON && type <= ITEM_GENERAL );
		//this->type = type; 
		this->name = name; 
		this->desc = desc; 
		this->deco = deco; 
		this->resource = resource; 
		this->index = index;
	}

	virtual const WeaponItemDef* IsWeapon() const { return 0; }
	virtual const ClipItemDef* IsClip() const  { return 0; }
	virtual int DefaultRounds() const { return 1; }
	virtual const ArmorItemDef* IsArmor() const { return 0; }

	// optimization trickiness:
	int index;
};

enum WeaponMode {
	kSnapFireMode,
	kAutoFireMode,
	kAltFireMode
};


class WeaponItemDef : public ItemDef
{
public:

	virtual const WeaponItemDef* IsWeapon() const { return this; }

	struct Weapon {
		const ClipItemDef* clipItemDef;
		int flags;			// WEAPON_AUTO, etc.
		float damage;		// damage done by weapon, 1.0 is normal
		float accuracy;		// 1.0 is average
		const char* sound;
	};

	const char*	fireDesc[3];	// name of the 3 fire modes
	float		speed;			// 1.0 is normal speed (and weight)
	Weapon		weapon[2];		// primary and secondary

	int Index( WeaponMode mode ) const								{ return ( mode == kAltFireMode ) ? 1 : 0; }

	bool HasWeapon( WeaponMode mode) const							{ return weapon[Index(mode)].damage > 0; }
	const ClipItemDef* GetClipItemDef( WeaponMode mode ) const		{ return weapon[Index(mode)].clipItemDef; }
	int RoundsNeeded( WeaponMode mode ) const						{ return ( mode == kAutoFireMode && weapon[0].flags & WEAPON_AUTO ) ? 3 : 1; }
	bool IsExplosive( WeaponMode mode ) const						{ return ( weapon[Index(mode)].flags & WEAPON_EXPLOSIVE ) != 0; }

	bool IsAlien() const;

	void RenderWeapon(	WeaponMode mode,
						ParticleSystem*,
						const grinliz::Vector3F& p0, 
						const grinliz::Vector3F& p1,
						bool impact,
						U32 currentTime,
						U32* doneTime ) const;

	bool CompatibleClip( const ItemDef* itemDef, int* which ) const;
	
	// Basic damage for this weapon.
	void DamageBase( WeaponMode mode, DamageDesc* damageArray ) const;
	// Amount of time it takes to use this weapon. (Does not depend on the Unit.)
	float TimeUnits( WeaponMode mode ) const;
	
	// Accuracy of the weapon. 1.0 is normal, higher is worse.
	Accuracy CalcAccuracy( float unitAccuracy, WeaponMode mode ) const;

	// Statistics for this weapon. 
	bool FireStatistics( WeaponMode mode, 
						 float unitAccuracy, 
						 float distance, 
						 float* chanceToHit,				// chance a round hits
						 float* chanceAnyHit,				// chance any round (of 1 or 3) hits
						 float* totalDamage,			
						 float* damagePerTU ) const;		// damagePerTU = f( damage, TU, accuracyRadius, distance )

};


class ClipItemDef : public ItemDef
{
public:

	virtual const ClipItemDef* IsClip() const { return this; }
	virtual int DefaultRounds() const { return defaultRounds; }

	bool IsAlien() const { return alien; }

	bool alien;

	int defaultRounds;
	DamageDesc dd;

	char	abbreviation;
	Color4F color;
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
	Item( const ItemDef* itemDef, int rounds=1 );
	Item( Game*, const char* name, int rounds=1 );

	void operator=( const Item& rhs )	{ rounds = rhs.rounds; itemDef = rhs.itemDef; }

	// wipe this item
	void Clear()									{ rounds = 0; itemDef = 0; }

	const ItemDef* GetItemDef() const				{ return itemDef; }
	const WeaponItemDef* IsWeapon() const			{ return itemDef ? itemDef->IsWeapon() : 0; }
	const ClipItemDef* IsClip() const				{ return itemDef ? itemDef->IsClip() : 0; }
	const ArmorItemDef* IsArmor() const				{ return itemDef ? itemDef->IsArmor() : 0; }

	int Rounds() const								{ return rounds; }

	// --- handle weapons ----//
	// consume one rounds
	void UseRounds( int n=1 );

	const ClipItemDef* ClipType( int select ) const	{	GLASSERT( IsWeapon() );
														return IsWeapon()->weapon[select].clipItemDef;
													}

	const char* Name() const						{ return itemDef->name; }
	const char* Desc() const						{ return itemDef->desc; }
	int Deco() const								{ return itemDef->deco; }

	bool IsNothing() const							{ return itemDef == 0; }
	bool IsSomething() const						{ return itemDef != 0; }

	void Save( FILE* fp, int depth ) const;
	void Load( const TiXmlElement* doc, Engine* engine, Game* game );

private:
	int rounds;
	const ItemDef* itemDef;
};


class Storage
{
public:
	Storage( int _x, int _y, ItemDef* const* _itemDefArr ) : x( _x ), y( _y ), itemDefArr( _itemDefArr )	{	memset( rounds, 0, sizeof(int)*EL_MAX_ITEM_DEFS ); }
	~Storage();

	void Init( const int* roundArr )			{ memcpy( rounds, roundArr, sizeof(int)*EL_MAX_ITEM_DEFS ); }
	const int* Rounds() const					{ return rounds; }

	bool Empty() const;
	void AddItem( const Item& item );
	void RemoveItem( const ItemDef*, Item* item );
	bool Contains( const ItemDef* ) const;

	// Return true if either is true:
	// 1. This storage contains a clip (of either type) for 'weapon',
	// 2. This storage contains any weapon and rounds for that weapon
	// Returns the weapon def that is re-supplied.
	const WeaponItemDef* IsResupply( const WeaponItemDef* weapon ) const;
	
	void SetCount( const ItemDef*, int count );
	int GetCount( const ItemDef* ) const;

	void Save( FILE* fp, int depth );
	void Load( const TiXmlElement* mapNode );

	const ModelResource* VisualRep( bool* zRotate ) const;

	int X() const { return x; }
	int Y() const { return y; }

private:
	int GetIndex( const ItemDef* itemDef ) const {
		int index = itemDef->index;
		GLASSERT( index >=0 && index < EL_MAX_ITEM_DEFS );
		return index;
	}
	int x, y;
	ItemDef* const* itemDefArr;
	int rounds[EL_MAX_ITEM_DEFS];
};


#endif // UFOATTACK_ITEM_INCLUDED
