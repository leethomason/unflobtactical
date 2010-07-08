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

#ifndef UFO_ATTACK_STATS_INCLUDED
#define UFO_ATTACK_STATS_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "gamelimits.h"
class TiXmlElement;

namespace grinliz {
class Random;
};


/*
	STR, DEX, PSY:	fixed at character creation.
	
	RANK:			computed by experience		[ 0, NUM_RANKS-1 ]
	MEDALS:			awarded for field action

	HP		 = f( STR )							[ 1, TRAIT_MAX ]
	TU		 = f( DEX, STR )					[ MIN_TU, MAX_TU ]

	Accuracy = f( DEX )
	Reaction = f( DEX, PSY )
*/
class Stats
{
public:
	Stats() : totalHP(1), totalTU((float)MIN_TU), _STR(1), _DEX(1), _PSY(1), rank( 0 ) {}

	void SetSTR( int value )			{ _STR = value; CalcBaselines(); }
	void SetDEX( int value )			{ _DEX = value; CalcBaselines(); }
	void SetPSY( int value )			{ _PSY = value; CalcBaselines(); }
	void SetRank( int value )			{ rank = value; CalcBaselines(); }

	static int GenStat( grinliz::Random* rand, int min, int max );

	// Base traits:
	int STR() const			{ return _STR; }
	int DEX() const			{ return _DEX; }
	int PSY() const			{ return _PSY; }

	int Rank() const		{ return rank; }

	// Computed:
	int TotalHP() const			{ return totalHP; }

	float TotalTU() const	{ return totalTU; }		// one TU is one move
	float Accuracy() const	{ return accuracy; }	// radius at 1 unit out
	float Reaction() const	{ return reaction; }	// 0.0-1.0. The chance of reaction fire

	void Save( TiXmlElement* doc ) const;
	void Load( const TiXmlElement* doc );

private:
	void CalcBaselines();

	// primary:
	int _STR, _DEX, _PSY;
	int rank;

	// derived:
	int totalHP;
	float totalTU;
	float accuracy;
	float reaction;
};


// It is crazy difficult to keep accuracy correct through the system, when just a float. Use
// a class to enforce type. Can only be computed in one place - the WeaponItemDef.
class Accuracy {
public:
	friend class WeaponItemDef;
	Accuracy() : radiusAtOne( 0 ) {}

	float RadiusAtOne() const { return radiusAtOne; }
private:
	Accuracy( float radAtOne ) : radiusAtOne( radAtOne ) {}
	float radiusAtOne;
};


class BulletSpread
{
public:
	BulletSpread()		{}
	~BulletSpread()		{}

	void Generate( U32 seed, grinliz::Vector2F* result );
	void Generate( U32 seed, const Accuracy& accuracy, float distance, const grinliz::Vector3F& dir, const grinliz::Vector3F& target, grinliz::Vector3F* targetPrime );
	float ComputePercent( const Accuracy& accuracy, float distance, float width=0.6f, float height=1.8f );
private:
	void Generate( float uniformX, float uniformY, grinliz::Vector2F* result );
	float Deviation( float x )	{ return (x>=0) ? x*x : -x*x; }		// x [-1,1], returns [-1,1], but skewed to the center. so Dev(0.5) = 0.25
};


#endif // UFO_ATTACK_STATS_INCLUDED
