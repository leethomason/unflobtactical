/*
Copyright (c) 2000-2003 Lee Thomason (www.grinninglizard.com)

Grinning Lizard Utilities. Note that software that uses the 
utility package (including Lilith3D and Kyra) have more restrictive
licences which applies to code outside of the utility package.


This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#include "glrandom.h"
#include <math.h>
using namespace grinliz;


// Very clever post by: George Marsaglia
// http://www.bobwheeler.com/statistics/Password/MarsagliaPost.txt
// The basic MWC is extremely attractive for its simplicity. But in
// its lower 16 bits, its period is shorted (since it is independent
// of the upper bits.) KISS is still simple and compelling.
U32 Random::Rand()
{
	#define znew  ((z=36969*(z&65535)+(z>>16))<<16)
	#define wnew  ((w=18000*(w&65535)+(w>>16))&65535)
	#define MWC   (znew+wnew)
	#define SHR3  (jsr=(jsr=(jsr=jsr^(jsr<<17))^(jsr>>13))^(jsr<<5))
	#define CONG  (jcong=69069*jcong+1234567)
	#define KISS  ((MWC^CONG)+SHR3)
	return KISS;
}


void Random::NormalVector( float* v, int dim )
{
	GLASSERT( dim > 0 );

	float len2 = 0.0f;
	for( int i=0; i<dim; ++i ) {
		v[i] = -1.0f + 2.0f*Uniform();
		len2 += v[i]*v[i];
	}
	// exceedingly unlikely error case:
	if ( len2 == 0.0f ) {
		v[0] = 1.0;
		len2 = 1.0;
	}
	float lenInv = 1.0f / sqrtf( len2 );
	for( int i=0; i<dim; ++i ) {
		v[i] *= lenInv;
	}
}
