#ifndef UFO_ATTACK_STATS_INCLUDED
#define UFO_ATTACK_STATS_INCLUDED

#include "../grinliz/glrandom.h"

class Stats
{
public:
	Stats() : hp(0), totalHP( 0 ) {}

	void InitStats( int _hp, float _tu ) {
		hp = totalHP = _hp;
		tu = totalTU = _tu;
	}
	void SetSTR( int value )			{ _STR = value; }
	void SetDEX( int value )			{ _DEX = value; }
	void SetPSY( int value )			{ _PSY = value; }
	int GenStat( grinliz::Random* rand, int min, int max );

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

	int HP() const			{ return hp; }
	int TotalHP() const		{ return totalHP; }
	float TU() const		{ return tu; }
	float TotalTU() const	{ return totalTU; }

private:
	int hp, totalHP;
	float tu, totalTU;
	int _STR, _DEX, _PSY;
};


#endif // UFO_ATTACK_STATS_INCLUDED