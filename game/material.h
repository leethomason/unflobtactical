#ifndef MATERIAL_UFOATTACK_INCLUDED
#define MATERIAL_UFOATTACK_INCLUDED


struct MaterialDef
{
	// SHELLS
	enum {
		SH_KINETIC		= 0x01,
		SH_ENERGY		= 0x02,
	};

	// MATERIALS
	enum {
		MAT_GENERIC		= 0x01,
		MAT_STEEL		= 0x02,
		MAT_WOOD		= 0x04,
	};

	int CalcDamage( int damage, int shell, int material );
};


#endif // MATERIAL_UFOATTACK_INCLUDED
