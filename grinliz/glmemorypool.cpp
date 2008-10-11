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

#include <stdlib.h>
#include <memory.h>
#include "gldebug.h"
#include "glmemorypool.h"

using namespace grinliz;

MemoryPool::MemoryPool( const char* _name, unsigned _objectSize, unsigned _blockSize )
{
	name = _name;

	chunkSize = _objectSize;
	if ( chunkSize < sizeof( Chunk ) )
		chunkSize = sizeof( Chunk );
	chunkSize = ( ( chunkSize + 3 )  / 4 ) * 4;
	blockSize = ( ( _blockSize + 3 ) / 4 ) * 4;

	// Reserve 4 bytes for "nextBlock" pointer.
	chunksPerBlock = ( blockSize - sizeof(Block*) ) / chunkSize;
	GLASSERT( chunksPerBlock > 1 );

	numBlocks = 0;
	numChunks = 0;

	rootBlock = 0;
	head = 0;
	GLOUTPUT(( "Memory pool '%s' created.\n", _name ));
}


MemoryPool::~MemoryPool()
{
	#ifdef DEBUG
	GLOUTPUT(( "Memory pool '%s' destroyed. #blocks=%d usedKB=%d\n", name, numBlocks, MemoryUsed()/1024 ));
	GLASSERT( numChunks == 0 );
	#endif
	FreePool();
}

void MemoryPool::NewBlock()
{
	GLASSERT( head == 0 );

	Block* block = (Block*) malloc( blockSize );
	GLASSERT( block );

	block->nextBlock = rootBlock;
	rootBlock = block;

	++numBlocks;

	Chunk** pp = &block->chunk;
	int increment = chunkSize / 4;
	unsigned i;
	for( i=0; i<chunksPerBlock-1; ++i )
	{
		*( pp+i*increment ) = (Chunk*) ( pp+(i+1)*increment );
	}
	*( pp+i*increment ) = 0;
	head = block->chunk;
}

void MemoryPool::FreePool()
{
	Block* block = rootBlock;
	while ( block )
	{
		Block* temp = block->nextBlock;
		free( block );
		block = temp;
	}
	numBlocks = numChunks = 0;
	rootBlock = 0;
	head = 0;
}


LinearMemoryPool::LinearMemoryPool( unsigned totalMemory )
{
	base = (char* ) malloc( totalMemory );
	GLASSERT( base );

	current = base;
	end = base + totalMemory;

	#ifdef DEBUG
		memset( base, 0xaa, totalMemory );
	#endif
}


LinearMemoryPool::~LinearMemoryPool()
{
	free( base );
}


void* LinearMemoryPool::Alloc( unsigned allocate )
{
	if ( current < end )
	{
		char* ret = current;
		current += allocate;

		// Out of memory check.
		GLASSERT( current <= end );
		if ( current <= end )
			return ret;
		else
			return 0;
	}
	else
	{
		// Out of memory!
		GLASSERT( 0 );
		return 0;
	}
}
