/*
Copyright (c) 2000-2007 Lee Thomason (www.grinninglizard.com)
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

#include "glcontainer.h"

#ifdef GRINLIZ_DEBUG_MEM
// Trick to get more info from the debug tracking system. Doesn't work in headers, regrettably.
#define new new(__FILE__, __LINE__)
#endif

using namespace grinliz;


MapIdToVoidPtr::MapIdToVoidPtr( U32 _size ) : size( 0 ), population( 0 ), table( 0 )
{
	Resize( _size );
}

MapIdToVoidPtr::~MapIdToVoidPtr() 
{ 
	free( table ); 
}

void MapIdToVoidPtr::Clear()
{
	memset( table, 0xff, size*sizeof(Node) );
	population = 0;
}

void MapIdToVoidPtr::Insert( U32 i, const void* ptr ) 
{
	GLASSERT( (i & 0x80000000) == 0 );	// we need the high bit as a flag

	if ( population > size / 2 ) {
		GLOUTPUT(( "MapIdToVoidPtr resize %d to %d\n", size, size*4 ));
		Resize( size*4 );
	}

	U32 hash = HashCode(i);
	U32 index = hash % size;

	// Linear probe. We can use any EMPTY or DELETED node.
	while( table[index].i >= 0 ) {
		++index;
		if ( index == size )
			index = 0;
	}
	if ( table[index].i != (NODE_DELETED) )
		++population;

	table[index].ptr = ptr;
	table[index].i   = i;
}


const void* MapIdToVoidPtr::Get( U32 i )
{
	U32 hash = HashCode(i);
	U32 index = hash % size;

	// Linear probe
	while ( true )
	{
		if ( table[index].i == (int)i )
			return table[index].ptr;
		if ( table[index].i == NODE_EMPTY )
			return 0;

		++index;
		if ( index == size )
			index = 0;
	}
	return 0;
}


bool MapIdToVoidPtr::Remove( U32 i )
{
	U32 hash = HashCode(i);
	U32 index = hash % size;

	// Linear probe
	while ( true )
	{
		if ( table[index].i == (int)i ) {
			table[index].ptr = 0;			// will be re-used
			table[index].i = NODE_DELETED;	// can't truly delete in a linear probe
			// note the population does not drop
			return true;
		}
		if ( table[index].i == NODE_EMPTY ) {
			return false;
		}
		++index;
		if ( index == size ) {
			index = 0;
		}
	}
	return false;
}
	

void MapIdToVoidPtr::Resize( U32 newSize ) 
{
	if ( newSize < population / 4 )
		newSize = population*4;

	Node* old = table;
	unsigned oldSize = size;

	size = newSize;
	table = (Node*) malloc( sizeof(Node)*size );
	Clear();

	for( unsigned i=0; i<oldSize; ++i ) 
	{
		if ( old[i].i >= 0 ) {
			Insert( old[i].i, old[i].ptr );
		}
	}
	free( old );
}

