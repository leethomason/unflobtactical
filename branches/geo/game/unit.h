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

	Unit() : status( STATUS_NOT_INIT ), model( 0 ) {}
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
	void DoDamage( const DamageDesc& damage, TacMap* map );
	void Kill( TacMap* map );		// normally called by DoDamage
	void UseTU( float val )		{ tu = grinliz::Max( 0.0f, tu-val ); }
	void Leave();
	void Heal()					{ if ( status != STATUS_NOT_INIT ) { tu = stats.TotalTU(); hp = stats.TotalHP(); status = STATUS_ALIVE; }}

	void NewTurn();

	int Team() const			{ return team; }
	int AlienType()	const		{ return type; }
	int Gender() const			{ return GetValue( GENDER ); }
	int AI() const				{ return ai; }

	void SetAI( int value )		{ ai = value; }

	const char* FirstName() const;
	const char* LastName() const;
	const char* Rank() const;

	void SetMapPos( int x, int z );
	void SetMapPos( const grinliz::Vector2I& pos ) { SetMapPos( pos.x, pos.y ); }
	void SetPos( const grinliz::Vector3F& pos, float rotation );
	void SetYRotation( float rotation );
	void CalcPos( grinliz::Vector3F* ) const;

	// Compute the bounding box of this unit's sight. (VERY loose.)
	void CalcVisBounds( grinliz::Rectangle2I* b ) const;

	// Compute the map pos: x,y (always int) and rotation (always multiple of 45)
	void CalcMapPos( grinliz::Vector2I*, float* rotation ) const;
	// Convenience to CalcMapPos
	grinliz::Vector2I Pos() const {
		grinliz::Vector2I p;
		CalcMapPos( &p, 0 );
		return p;
	}

	Item* GetWeapon();
	const Item* GetWeapon() const;
	const WeaponItemDef* GetWeaponDef() const;

	Inventory* GetInventory();
	const Inventory* GetInventory() const;
	void UpdateInventory();
	//bool RestoreInventory( const Inventory& memory, Storage* storage );	// based on 'memory' restore items after battle. Returns 'true' if okay, 'false' if out of stuff

	Model* GetModel()						{ return model; }
	const Model* GetModel() const			{ return model; }
	Model* GetWeaponModel()					{ return weapon; }
	const Model* GetWeaponModel() const		{ return weapon; }

	// Returns true if the mode can be used: mode is supported, enough time units,
	// enough rounds, etc.
	bool CanFire( WeaponMode mode ) const;
	float FireTimeUnits( WeaponMode mode ) const;
	void AllFireTimeUnits( float *snapTU, float* autoTU, float* altTU );

	// returns true if this fire mode is supported
	bool FireStatistics(	WeaponMode mode,
							const BulletTarget& target,
							float* chanceToHit, float* chanceAnyHit, 
							float* tu, float* damagePerTU ) const;

	// get the accuracy of the current mode
	Accuracy CalcAccuracy( WeaponMode mode ) const;

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

	void Save( FILE* fp, int depth ) const;

	// Loads the model. Follow with InitModel() if models needed.
	void Load( const TiXmlElement* doc, const ItemDefArr& arr );
	void InitModel(  SpaceTree* tree, TacMap* tacmap );
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

private:
	// WARNING: change this then change GetValue()
	enum {	
		GENDER,			// 1, 0-1
		APPEARANCE,		// 4, 0-15
		LAST_NAME,		// 5, 0-31
		FIRST_NAME,		// 6, 0-63
		NUM_TRAITS
	};
	U32 GetValue( int which ) const;

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

	//Game*		game;
	SpaceTree*	tree;
	Model*		model;
	Model*		weapon;
	bool		visibilityCurrent;	// if set, the visibility is current. Can be set by CalcAllVisibility()
	grinliz::Random	random;
	grinliz::Vector3F	initPos;	// position used to create the model.
	float				initRot;

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


#endif // UFOATTACK_UNIT_INCLUDED
