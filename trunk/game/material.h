#ifndef MATERIAL_UFOATTACK_INCLUDED
#define MATERIAL_UFOATTACK_INCLUDED

// Shells
enum {
	SH_KINETIC		= 0x01,
	SH_EXPLOSIVE	= 0x02,
	SH_ENERGY		= 0x04,
	SH_AP			= 0x08,
	SH_INCIN		= 0x10,

	SH_BITS = 5,
};


// Materials
enum {
	MAT_GENERIC		= 0x01,
	MAT_STEEL		= 0x02,
	MAT_WOOD		= 0x04,

	MAT_BITS = 3,
};
		
struct MaterialDef
{
	static int CalcDamage( int damage, int shell, int material );
};


#endif // MATERIAL_UFOATTACK_INCLUDED
