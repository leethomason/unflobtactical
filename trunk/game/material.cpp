#include "material.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"


const float gDamage[SH_BITS*MAT_BITS] = 
{
//	Generic	Steel	Wood
	0.0f,	0.0f,	0.0f,		// kinetic
	0.0f,	0.2f,	0.4f		// energy
};

int MaterialDef::CalcDamage( int damage, int shell, int material )
{
	if ( damage < 1 )
		return 0;

	//GLASSERT( (shell & SH_MASK) <= shell );		// okay, but make sure using high bits is okay!
	//GLASSERT( (material & MAT_MASK) <= material );		// okay, but make sure using high bits is okay!

	// The damage bonus:
	// SUMMED for each shell type
	// AVERAGED for the material.

	float shellSum[MAT_BITS] = { 0.0f };

	for( int m=0; m<MAT_BITS; ++m ) {
		for( int s=0; s<SH_BITS; ++s ) {
			if ( (1<<s) & shell ) {
				shellSum[m] += gDamage[s*MAT_BITS+m];
			}
		}
	}
	int count=0;
	float sum = 0.0f;
	float ave = 0.0f;

	for( int i=0; i<MAT_BITS; ++i ) {
		if ( shellSum[i] > 0 ) {
			++count;
			sum += shellSum[i];
		}
	}
	if ( count > 0 ) {
		ave = sum / (float)count;
	}
	ave += 1.0f;

	int hp = (int)((float)damage * ave);
	if ( hp < 1 )
		hp = 1;
	return hp;
}
