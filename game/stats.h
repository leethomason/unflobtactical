#ifndef UFO_ATTACK_STATS_INCLUDED
#define UFO_ATTACK_STATS_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

namespace grinliz {
class Random;
};


/*
	STR, DEX, PSY:	fixed at character creation.
	
	LEVEL:			computed by experience
	MEDALS:			awarded for field action

	HP		 = f( STR )
	TU		 = f( DEX, STR )
	Accuracy = f( DEX )
*/
class Stats
{
public:
	Stats() : hp(0), totalHP( 0 ), tu( 10.0f ), totalTU( 10.0f ), _STR( 50 ), _DEX( 50 ), _PSY( 50 ), level( 0 ) {}

	void SetSTR( int value )			{ _STR = value; }
	void SetDEX( int value )			{ _DEX = value; }
	void SetPSY( int value )			{ _PSY = value; }
	void SetLevel( int value )			{ level = value; }
	void CalcBaselines();

	static int GenStat( grinliz::Random* rand, int min, int max );

	void DoDamage( int hitDamage ) {
		if ( hp < 0xffff ) {
			hp -= hitDamage;
			if ( hp < 0 )
				hp = 0;
		}
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

	/*
		20-80 human range for any stat.
		Level 0-5.
	*/
	enum { LEVEL_MAX = 5 };

	// Base traits:
	int STR() const			{ return _STR; }
	int DEX() const			{ return _DEX; }
	int PSY() const			{ return _PSY; }

	int Level() const		{ return level; }

	// Computed:
	int HP() const				{ return hp; }
	int TotalHP() const			{ return totalHP; }
	float HPFraction() const	{ return (float)hp / (float)totalHP; }

	float TU() const		{ return tu; }
	float TotalTU() const	{ return totalTU; }		// one TU is one move
	float Accuracy() const	{ return accuracy; }	// cone at 1 unit out
	float Reaction() const	{ return reaction; }	// 0.0-1.0. The chance of reaction fire

private:
	int hp, totalHP;
	float tu, totalTU;
	int _STR, _DEX, _PSY;
	int level;
	float accuracy;
	float reaction;
};


#endif // UFO_ATTACK_STATS_INCLUDED