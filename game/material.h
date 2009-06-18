#ifndef MATERIAL_UFOATTACK_INCLUDED
#define MATERIAL_UFOATTACK_INCLUDED


struct MaterialDef
{
	// SHELLS
	enum {
		SH_KINETIC		= 0x01,
		SH_ENERGY		= 0x02,
		SH_AP			= 0x04,
		SH_INCIN		= 0x08,

		SH_MASK = SH_KINETIC | SH_ENERGY,
		SH_BITS = 4,
	};

	// MATERIALS
	enum {
		MAT_GENERIC		= 0x01,
		MAT_STEEL		= 0x02,
		MAT_WOOD		= 0x04,

		MAT_MASK = MAT_GENERIC | MAT_STEEL | MAT_WOOD,
		MAT_BITS = 3,
	};

	static int CalcDamage( int damage, int shell, int material );
};


#endif // MATERIAL_UFOATTACK_INCLUDED
