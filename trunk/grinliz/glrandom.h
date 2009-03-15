/*
Copyright (c) 2000-2003 Lee Thomason (www.grinninglizard.com)
Grinning Lizard Utilities.

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


#ifndef GRINLIZ_RANDOM_INCLUDED
#define GRINLIZ_RANDOM_INCLUDED

#include "gldebug.h"
#include "gltypes.h"

namespace grinliz {

/**	Random number generators (including the rand() in C) are often frustratingly
	non-random in their lower bits. This is a fast random number that is random
	in its lower bits.
*/

class Random
{
  public:
	/// Constructor, with optional seed value.
	Random( U32 seed = 0 )		{ SetSeed( seed ); }

	/** The current seed can be set at any time to
		guarentee a certain sequence of random numbers.
	*/
	void SetSeed( U32 seed )	{	w = 521288629 + seed;
									z = 362436069 + seed;
									jsr = 123456789;
									jcong = 380116160;
								}									

	/// Returns a 32 bit random number.
	U32 Rand();						

	/** Returns a random number greater than or equal to 0, and less 
		that 'upperBound'.
	*/	
	U32 Rand( U32 upperBound )		{ return Rand() % upperBound; }

	/** "Roll dice." Has the same bell curve distribution as N dice. Dice start with the 
		value '1', so RandD2( 2, 6 ) returns a value from 2-12
	*/
	U32 Dice( U32 nDice, U32 sides ) {	
		U32 total = 0;
		for( U32 i=0; i<nDice; ++i ) { total += Rand(sides)+1; }
		return total;
	}

	/// Return a random number from 0 to upper: [0.0,1.0].
	float Uniform()	{ 
		const float INV = 1.0f / 65535.0f;	
		return (float)( Rand() & 65535 ) * INV;
	}

	/// Return 0 or 1
	int Bit()
	{
		return Rand()>>31;
	}

	/// Return a random boolean.
	bool Boolean()					
	{ 
		return Bit() ? true : false;
	}

	/// Return +1 or -1
	int Sign()					
	{ 
		return -1 + 2*Bit();
	}

	/// Return a random unit normal vector. 'dimension' is usual 2 (2D) or 3 (3D)
	void NormalVector( float* v, int dimension );

private:
	U32 z, w, jsr, jcong;
};

};	// namespace grinliz

#endif
