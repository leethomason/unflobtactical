#ifndef UFO_ATTACK_STATS_INCLUDED
#define UFO_ATTACK_STATS_INCLUDED


class Stats
{
public:
	Stats() : hp(0), totalHP( 0 ) {}

	void Init( int _hp ) {
		hp = totalHP = _hp;
	}
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
		GLASSERT( t >= tu );
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

};


#endif // UFO_ATTACK_STATS_INCLUDED