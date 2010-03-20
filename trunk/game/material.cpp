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

#include "material.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

/*
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
*/