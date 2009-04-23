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
#include <string.h>
	
using namespace grinliz;


void Random::SetSeed( const char* str )
{
	int len = strlen( str );
	U32 seed = SuperFastHash( str, len );
	SetSeed( seed );
}


// Very clever post by: George Marsaglia
// http://www.bobwheeler.com/statistics/Password/MarsagliaPost.txt
// And also the paper:
// http://tbf.coe.wayne.edu/jmasm/vol2_no1.pdf
// and post:
// http://oldmill.uchicago.edu/~wilder/Code/random/Papers/Marsaglia_2003.html
// Simple, elegant, effective!

U32 Random::Rand()
{
	U64 t;
	U64 a=698769069;

	x = 69069*x + 12345;
	y ^= (y<<13);  
	y ^= (y>>17); 
	y ^= (y<<5);

	t = a*(U64)z + (U64)c; 
	c = (U32)(t>>32);
	z = (U32)t;

	return x+y+z;
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

// Another great work:
// http://www.azillionmonkeys.com/qed/hash.html
// by Paul Hsieh
//
U32 Random::SuperFastHash (const void* _data, U32 len) 
{
#ifdef GRINLIZ_LITTLE_ENDIAN
	#define get16bits(d) (*((const U16 *) (d)))
#endif

#if	!defined (get16bits)
	#define get16bits(d) ((((U32)(((const U8 *)(d))[1])) << 8)\
						   +(U32)(((const U8 *)(d))[0]) )
#endif

	const U8* data = (const U8*)_data;
	U32 hash = len, tmp;
	S32 rem;

    if (len <= 0 || data == NULL) 
		return 0;

    rem = len & 3;
    len >>= 2;

    // Main loop 
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    // Handle end cases
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    // Force "avalanching" of final 127 bits
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
