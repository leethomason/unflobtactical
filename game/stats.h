#ifndef UFO_ATTACK_STATS_INCLUDED
#define UFO_ATTACK_STATS_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "gamelimits.h"
class TiXmlElement;

namespace grinliz {
class Random;
};


/*
	STR, DEX, PSY:	fixed at character creation.
	
	RANK:			computed by experience		[ 0, NUM_RANKS-1 ]
	MEDALS:			awarded for field action
	ARMOR:			set on unit					[ 0, TRAIT_MAX ], where 10-60 is typical

	HP		 = f( STR, ARMOR )					[ 1, TRAIT_MAX ]
	TU		 = f( DEX, STR )					[ MIN_TU, MAX_TU ]

	Accuracy = f( DEX )
	Reaction = f( DEX, PSY )
*/
class Stats
{
public:
	Stats() : hp(1), totalHP(1), tu((float)MIN_TU), totalTU((float)MIN_TU), _STR(1), _DEX(1), _PSY(1), rank( 0 ), armor( 0 ) {}

	void SetSTR( int value )			{ _STR = value; CalcBaselines(); }
	void SetDEX( int value )			{ _DEX = value; CalcBaselines(); }
	void SetPSY( int value )			{ _PSY = value; CalcBaselines(); }
	void SetRank( int value )			{ rank = value; CalcBaselines(); }
	void SetArmor( int value )			{ armor = value; CalcBaselines(); }

	static int GenStat( grinliz::Random* rand, int min, int max );

	void DoDamage( int hitDamage ) {
		hp -= hitDamage;
		if ( hp < 0 )
			hp = 0;
	}
	void ZeroHP() { 
		hp = 0;
	}

	void UseTU( float t ) {
		GLASSERT( t <= tu );
		tu -= t;
	}
	void RestoreTU() {
		tu = totalTU;
	}

	// Base traits:
	int STR() const			{ return _STR; }
	int DEX() const			{ return _DEX; }
	int PSY() const			{ return _PSY; }

	int Rank() const		{ return rank; }

	// Computed:
	int HP() const				{ return hp; }
	int TotalHP() const			{ return totalHP; }
	float HPFraction() const	{ return (float)hp / (float)totalHP; }
	int Armor() const			{ return armor; }

	float TU() const		{ return tu; }
	float TotalTU() const	{ return totalTU; }		// one TU is one move
	float Accuracy() const	{ return accuracy; }	// cone at 1 unit out
	float Reaction() const	{ return reaction; }	// 0.0-1.0. The chance of reaction fire

	void Save( TiXmlElement* doc ) const;
	void Load( const TiXmlElement* doc );

private:
	void CalcBaselines();

	// primary:
	int _STR, _DEX, _PSY;
	int rank;
	int armor;

	// derived:
	int hp, totalHP;
	float tu, totalTU;
	float accuracy;
	float reaction;
};


#endif // UFO_ATTACK_STATS_INCLUDED
