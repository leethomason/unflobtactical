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

#ifndef UFOATTACK_UNIT_INCLUDED
#define UFOATTACK_UNIT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glbitarray.h"
#include "../engine/enginelimits.h"

#include "stats.h"
#include "inventory.h"
#include "gamelimits.h"

class Model;
class ModelResource;
class Engine;
class Game;
class TacMap;
class SpaceTree;


class Unit
{
public:
	enum {
		STATUS_NOT_INIT,
		STATUS_ALIVE,

		STATUS_KIA,
		STATUS_UNCONSCIOUS,
		STATUS_MIA
	};

	enum {
		ALIEN_GREEN,
		ALIEN_PRIME,
		ALIEN_HORNET,
		ALIEN_JACKAL,
		ALIEN_VIPER,
		NUM_ALIEN_TYPES
	};

	enum {
		MALE,
		FEMALE
	};

	Unit() : status( STATUS_NOT_INIT ), version( 1 ) {}
	~Unit();
	
	void Free();

	bool InUse() const			{ return status != STATUS_NOT_INIT; }
	bool IsAlive() const		{ return status == STATUS_ALIVE; }

	bool IsKIA() const			{ return status == STATUS_KIA; }
	bool IsUnconscious() const	{ return status == STATUS_UNCONSCIOUS; }
	bool IsMIA() const			{ return status == STATUS_MIA; }

	int HP() const				{ return hp; }
	float TU() const			{ return tu; }

	// Do damage to this unit. Will create a Storage on the map, if the map is provided.
	void DoDamage( const DamageDesc& damage, TacMap* map, bool playSound );
	void Kill( TacMap* map, bool playSound );		// normally called by DoDamage
	void UseTU( float val )		{ tu = grinliz::Max( 0.0f, tu-val ); }
	void Leave();
	void Heal()					{ if ( status != STATUS_NOT_INIT ) { tu = stats.TotalTU(); hp = stats.TotalHP(); status = STATUS_ALIVE; }}

	void NewTurn();

	int Team() const			{ return team; }
	int AlienType()	const		{ return type; }
	int Gender() const			{ return GetValue( GENDER ); }
	int AI() const				{ return ai; }
	const char* AlienName() const;

	void SetAI( int value )		{ ai = value; }

	const char* FirstName() const;
	const char* LastName() const;
	const char* Rank() const;

	void SetMapPos( int x, int z );
	void SetMapPos( const grinliz::Vector2I& pos ) { SetMapPos( pos.x, pos.y ); }
	void SetPos( const grinliz::Vector3F& pos, float rotation );
	void SetYRotation( float rotation );

	grinliz::Vector3F Pos() const	{ return pos; }
	float Rotation() const			{ return rot; }

	// Compute the bounding box of this unit's sight. (VERY loose.)
	void CalcVisBounds( grinliz::Rectangle2I* b ) const;

	// Compute the map pos: x,y (always int) and rotation (always multiple of 45)
	void CalcMapPos( grinliz::Vector2I*, float* rotation ) const;
	// Convenience to CalcMapPos
	grinliz::Vector2I MapPos() const {
		grinliz::Vector2I p;
		CalcMapPos( &p, 0 );
		return p;
	}

	Item* GetWeapon();
	const Item* GetWeapon() const;
	const WeaponItemDef* GetWeaponDef() const;

	Inventory* GetInventory();
	const Inventory* GetInventory() const;

	// Returns true if the mode can be used: mode is supported, enough time units,
	// enough rounds, etc.
	bool CanFire( int mode ) const;
	float FireTimeUnits( int mode ) const;
	void AllFireTimeUnits( float *snapTU, float* autoTU, float* altTU ) const;

	// returns true if this fire mode is supported
	bool FireStatistics(	int mode,
							const BulletTarget& target,
							float* chanceToHit, float* chanceAnyHit, 
							float* tu, float* damagePerTU ) const;

	// get the accuracy of the current mode
	Accuracy CalcAccuracy( int mode ) const;

	// An AI query: is this unit ready to go? Or do we need to find a new weapon?
	bool HasGunAndAmmo( bool atReady ) const;
	
	enum {
		NO_WEAPON,
		NO_TIME,
		SNAP_SHOT,
		AUTO_SHOT,
		SECONDARY_SHOT
	};
	int CalcWeaponTURemaining( float subtract ) const;

	// Calculate the angle from 'this' to the 'dst'. The coordinates are
	// in map coordinates, and the angle is the absolute y angle. (Not the delta.
	// The function name could be better.) If 'quantize' is true, the angle
	// will be rounded to the nearest 45 degrees.
	float AngleBetween( const grinliz::Vector2I& dst, bool quantize ) const;

	const Stats& GetStats() const	{ return stats; }
	static void GenStats( int team, int type, int seed, Stats* stats );
	void DoMissionEnd() {
		nMissions++;
		if ( hp < stats.TotalHP() ) allMissionOvals++;
		allMissionKills += kills;
		stats.SetRank( XPToRank( XP() ));
		
		kills = 0;
	}

	void CreditKill()				{ kills++; }
	int  KillsCredited() const		{ return kills; }
	void CreditGunner()				{ gunner++; stats.SetRank( XPToRank( XP() )); }

	int Missions() const			{ return nMissions; }
	int AllMissionKills() const		{ return allMissionKills; }
	int AllMissionOvals() const		{ return allMissionOvals; }

	int XP() const					{ return nMissions + allMissionKills + allMissionOvals + gunner; }
	static int XPToRank( int xp );
	const U32 Body() const			{ return body; }

	void Save( FILE* fp, int depth ) const;

	// Loads the model. Follow with InitModel() if models needed.
	void Load( const TiXmlElement* doc, const ItemDefArr& arr );
	void InitLoc(  TacMap* tacmap );
	void Create(	int team,
					int alienType,
					int rank,
					int seed );

	static int Count( const Unit* unit, int n, int status ) {
		int count = 0;
		if ( status >= 0 )
			for( int i=0; i<n; ++i ) { if ( unit[i].status == status ) ++count; }
		else
			for( int i=0; i<n; ++i ) { if ( unit[i].status != STATUS_NOT_INIT ) ++count; }

		return count;
	}

	static const Unit* Find( const Unit* unit, int n, U32 body ) {
		for( int i=0; i<n; ++i ) {
			if ( unit[i].Body() == body )
				return unit+i;
		}
		return 0;
	}

	// WARNING: change this then change GetValue()
	enum {	
		GENDER,			// 1, 0-1
		APPEARANCE,		// 4, 0-15
		LAST_NAME,		// 5, 0-31
		FIRST_NAME,		// 6, 0-63
		APPEARANCE_EXT,	// 4, 0-15
		LAST_NAME_EXT,	// 1, 0-1, version >= 1	
		NUM_TRAITS
	};
	U32 GetValue( int which ) const;

private:

	// Note that the 'stats' should be set before init.
	// Load calls init automatically
	void Init(	int team,
				int status,
				int alienType,
				int seed );

	void CreateModel();
	void UpdateModel();		// make the model current with the unit status - armor, etc.
	void UpdateWeapon();	// set the gun position

	int status;		// alive, dead, etc.
	int ai;			// normal or guard
	int team;		// terran, alien, civ
	int type;		// type of alien
	U32 body;		// describes everything! a random #
	int version;	

	bool				visibilityCurrent;	// if set, the visibility is current. Can be set by CalcAllVisibility()
	grinliz::Random		random;
	grinliz::Vector3F	pos;		// y always 0
	float				rot;

	Inventory	inventory;
	Stats		stats;
	float		tu;
	int			hp;

	int			kills;	
	int			nMissions;
	int			allMissionKills;
	int			allMissionOvals;
	int			gunner;
};


class UnitRenderer
{
public:
	UnitRenderer();
	~UnitRenderer();

	void Update( SpaceTree* tree, const Unit* unit );

	const Model* GetModel() const		{ return model; }
	const Model* GetWeapon() const		{ return weapon; }

	void SetSelectable( bool selectable );

private:
	SpaceTree*  tree;
	Model*		model;
	Model*		weapon;
};


#endif // UFOATTACK_UNIT_INCLUDED
