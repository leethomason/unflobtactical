#include "battledata.h"

void BattleData::CopyUnit( int i, const Unit& copyFrom )
{
	GLASSERT( i >= 0 && i < MAX_UNITS );
	units[i].Free();
	units[i] = copyFrom;
}