
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
#include "gltypes.h"
#include "glmemorypool.h"

using namespace grinliz;

MemoryPool::MemoryPool( const char* _name, unsigned _objectSize, unsigned _blockSize, bool _warn )
{
	name = _name;
	warn = _warn;

	objectSize = _objectSize;
	if ( sizeof(void*) > objectSize )
		objectSize = sizeof(void*);

	objectSize = ( ( objectSize + 3 )  / 4 ) * 4;
	blockSize  = ( ( _blockSize + 3 ) / 4 ) * 4;

	// Reserve 4 bytes for "nextBlock" pointer.
	objectsPerBlock = ( blockSize - sizeof(Block) ) / objectSize;
	GLASSERT( objectsPerBlock > 1 );

	numBlocks = 0;
	numObjects = 0;
	numObjectsWatermark = 0;
	rootBlock = 0;
	head = 0;

	GLOUTPUT(( "Memory pool '%s' created.\n", _name ));
}


MemoryPool::~MemoryPool()
{
	#ifdef DEBUG
	GLOUTPUT(( "Memory pool '%s' destroyed. #blocks=%d usedKB=%d/%d\n", 
			   name, numBlocks, MemoryInUseWatermark()/1024, MemoryAllocated()/1024 ));
	GLASSERT( numObjects == 0 );
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

	if ( numBlocks > 0 && warn ) {
		GLOUTPUT(( "WARNING: memory pool '%s' growing.\n" ));
		GLASSERT( 0 );
	}
	++numBlocks;

	for( unsigned i=0; i<objectsPerBlock-1; ++i )
	{
		GetObject( block, i )->next = GetObject( block, i+1 );
	}
	GetObject( block, objectsPerBlock-1 )->next = 0;
	head = (Object*)MemStart( block );
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
	numBlocks = 0;
	numObjects = 0;
	rootBlock = 0;
	head = 0;
}


void* MemoryPool::Alloc()
{
	void* mem = 0;

	if ( !head ) {
		NewBlock();
		GLASSERT( head );
	}

	mem = head;
	head = head->next;

	++numObjects;
	if ( numObjects > numObjectsWatermark )
		numObjectsWatermark = numObjects;

	#ifdef DEBUG
	// Is this in a memory block?
	Block* block=0;
	for( block=rootBlock; block; block=block->nextBlock ) {
		if ( mem >= MemStart( block ) && mem < MemEnd( block ) )
			break;
	}
	GLASSERT( block );
	GLASSERT( mem >= MemStart( block ) && mem < MemEnd( block ) );
	memset( mem, 0xaa, objectSize );
	#endif

	return mem;
}


bool MemoryPool::MemoryInPool( void* mem )
{
	// Is this in a memory block?
	Block* block=0;
	for( block=rootBlock; block; block=block->nextBlock ) {
		if ( mem >= MemStart( block ) && mem < MemEnd( block ) )
			break;
	}

	if ( block ) {
		// Does it map to a proper address?
#ifdef DEBUG
		int offset = (U8*)mem - (U8*)MemStart( block );
		GLASSERT( offset % objectSize == 0 );
#endif
		return true;
	}
	return false;
}


void MemoryPool::Free( void* mem ) 
{
	if ( !mem )
		return;

	GLASSERT( numObjects > 0 );
	--numObjects;

	#ifdef DEBUG
	GLASSERT( MemoryInPool( mem ) );

	memset( mem, 0xbb, objectSize );
	#endif

	((Object*)mem)->next = head;
	head = (Object*)mem;
}



