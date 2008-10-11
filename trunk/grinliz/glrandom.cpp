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
#include "glutil.h"

using namespace grinliz;

Random::Random(	U32 _seed )
{
	SetSeed( _seed );
}


U16 Random::Randomize()
{
	CalcSeed();
	U32 slot = ( seed & 0xffff ) / SLOTWIDTH;
	GLASSERT( slot < TABLESIZE );

	U16 ret = seedTable[slot];
	seedTable[slot] = U16( seed );
	return ret;
}


void Random::SetSeed( U32 _seed ) 
{
	int i;
	seed = _seed;
	
	// Initial values:
	for ( i=0; i<TABLESIZE; i++ )
	{
		CalcSeed();
		seedTable[i] = (U16)seed;	// intentionally truncate
	}

	// Shuffle:
	for ( i=0; i<TABLESIZE; i++ )
	{
		CalcSeed();
		Swap( &seedTable[i], &seedTable[ ( seed & 0xffff ) / SLOTWIDTH ] );
	}
}
