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
	Random( U32 _seed = 0 );

	/** The current seed can be set at any time. This does 
		put the random number generator in a consistent state,
		even though the random numbers are actually partially
		cached.
	*/
	void SetSeed( U32 _seed );

	/// Returns a 16bit random number.
	U16 Rand()						{ return Randomize();	}
	/** Returns a random number greater than or equal to 0, and less 
		that 'upperBound'.
	*/	
	U16 Rand( U16 upperBound )		{ return Randomize() % upperBound; }

	/** "Roll two dice." Return a result from [0,upperBound).
		This has the same bell curve distribution as 2 dice.
	*/
	U16 RandD2( U16 upperBound )	{ U16 d1 = upperBound / 2 + 1;
									  U16 d2 = upperBound - d1 + 1;
									  U16 r  = Rand( d1 ) + Rand( d2 );
									  GLASSERT( r < upperBound );
									  return r;
									}

	/** "Roll three dice." Return a result from [0,upperBound).
		This has the same bell curve distribution as 3 dice.
	*/
	U16 RandD3( U16 upperBound )	{ U16 d1 = upperBound / 3 + 2;
									  U16 d2 = upperBound / 3 + 2;
									  U16 d3 = upperBound - d1 - d2 + 2;
									  U16 r = Rand( d1 ) + Rand( d2 ) + Rand( d3 );
									  GLASSERT( r < upperBound );
									  return r;
									}

	/// Return a random number from 0 to upper: [0,upper].
	double DRand( double upper )	{ return upper * double( Randomize() ) / 65535.0; }

	/// Return a random number from 0 to upper: [0,upper].
	float FRand( float upper )		{ return upper * float( Randomize() ) / 65535.0f; }

	/// Return a random boolean.
	bool Boolean()					{ return Rand( 100 ) >= 50; }

  private:
	U32 seed;
	inline void CalcSeed()		{ seed = seed * 39421 + 1; }
	U16 Randomize();

	enum {
		TABLESIZE = 16,
		SLOTWIDTH = 0x10000 / TABLESIZE
	};

	U16 seedTable[ TABLESIZE ];
};

};	// namespace grinliz

#endif
