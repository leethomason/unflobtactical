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


#ifndef GRINLIZ_MEMORY_POOL
#define GRINLIZ_MEMORY_POOL

#include "gldebug.h"

#ifdef DEBUG
#include <string.h>
#endif


namespace grinliz {

/*	A memory pool that will dynamically grow as memory is needed.
*/
class MemoryPool
{
  public:
	MemoryPool( const char* _name, unsigned objectSize, unsigned blockSize = 4096, bool warnOnGrowth=false );
	~MemoryPool();

	void* Alloc();
	void Free( void* mem );

	void FreePool();

	unsigned Blocks() const				{ return numBlocks; }
	unsigned NumObjects() const			{ return numObjects; }
	unsigned ObjectWatermark() const	{ return numObjectsWatermark; }

	unsigned MemoryInUse() const			{ return numObjects * objectSize; }
	unsigned MemoryInUseWatermark()	const 	{ return numObjectsWatermark * objectSize; }
	unsigned MemoryAllocated() const		{ return numBlocks * blockSize; }

	bool Empty() const						{ return numObjects == 0; }
	bool MemoryInPool( void* mem ) const;

  private:
	struct Block
	{
		Block* nextBlock;
	};
	struct Object
	{
		Object* next;
	};

	void NewBlock();

	void* MemStart( Block* block ) const
	{ 
		return block+1; 
	}
	void* MemEnd( Block* block ) const		
	{ 
		return (U8*)MemStart( block ) + objectsPerBlock*objectSize;
	}
	Object* GetObject( Block* block, unsigned i )	
	{	
		GLASSERT( i<objectsPerBlock );
		U8* mem = (U8*)MemStart( block ) + i*objectSize;
		return (Object*)mem;
	}


	unsigned objectSize;		// size of chunk in bytes
	unsigned blockSize;			// size of block in bytes
	unsigned objectsPerBlock;
	
	unsigned numBlocks;
	unsigned numObjects;
	unsigned numObjectsWatermark;
	
	Block* rootBlock;
	Object* head;

	const char* name;
	bool warn;
};

/*	A memory pool that has a fixed allocation.
	Essentially a cross between a linked list and an array.
*/
template < class T, int COUNT >
class FixedMemoryPool
{
  private:
	struct Chunk
	{
		Chunk* next;
	};

  public:
	FixedMemoryPool()
	{
		GLASSERT( sizeof( T ) >= sizeof( Chunk* ) );
		for( int i=0; i<COUNT-1; ++i )
		{
			( (Chunk*)(&memory[i]) )->next = (Chunk*)(&memory[i+1]);
		}
		( (Chunk*)(&memory[COUNT-1]) )->next = 0;
		root = ( (Chunk*)(&memory[0]) );
		inUse = 0;
	}

	~FixedMemoryPool()	{}

	T* Alloc()
	{
		T* ret = 0;
		if ( root )
		{
			ret = (T*) root;
			root = root->next;
			++inUse;
		}
		return ret;
	}

	void Free( T* _mem )
	{
		if ( _mem )
		{
			Chunk* mem = (Chunk*) _mem;
			#ifdef DEBUG
				memset( mem, 0xfe, sizeof( T ) );
			#endif
			mem->next = root;
			root = mem;
			--inUse;
		}
	}

	unsigned InUse()	{	return inUse; }
	unsigned Remain()	{	return COUNT - inUse; }
	bool     Contains( T* mem )	{ return mem >= memory && mem < &memory[COUNT]; }

  private:
	T memory[ COUNT ];
	unsigned inUse;
	Chunk* root;
};


};

#endif
