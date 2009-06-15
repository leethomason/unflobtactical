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
		hp -= hitDamage;
		if ( hp < 0 )
			hp = 0;
	}
	void ZeroHP() { 
		hp = 0;
	}

	int HP() const			{ return hp; }
	int TotalHP() const		{ return totalHP; }


private:
	int hp;
	int totalHP;
};


#endif // UFO_ATTACK_STATS_INCLUDED