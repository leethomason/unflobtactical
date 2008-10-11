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
	MemoryPool( const char* _name, unsigned objectSize, unsigned blockSize = 4096 );
	~MemoryPool();

	void* Alloc() {
		void* ret = 0;

		if ( !head ) {
			NewBlock();
			GLASSERT( head );
		}

		ret = head;
		head = head->nextChunk;

		++numChunks;
		#ifdef DEBUG
			memset( ret, 0xaa, chunkSize );
		#endif

		return ret;
	}

	void Free( void* mem ) {
		if ( !mem )
			return;

		Chunk* chunk = (Chunk*) mem;

		--numChunks;
		#ifdef DEBUG
			memset( mem, 0xbb, chunkSize );
		#endif

		chunk->nextChunk = head;
		head = chunk;
	}

	void FreePool();

	unsigned Blocks()	{ return numBlocks; }
	unsigned Chunks()	{ return numChunks; }
	unsigned MemoryUsed()	{ return numBlocks * blockSize; }

  private:
	struct Chunk
	{
		Chunk* nextChunk;
	};

	struct Block
	{
		Block* nextBlock;
		Chunk* chunk;		// treated as an array of chunks.
	};

	void NewBlock();

	unsigned chunkSize;			// size of chunk in bytes
	unsigned blockSize;			// size of block in bytes
	unsigned chunksPerBlock;
	
	unsigned numBlocks;
	unsigned numChunks;
	
	Block* rootBlock;
	Chunk* head;

	const char* name;
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


/* 	This is memory allocation for when you know exactly how much 
	memory is going to be needed. So it's a way to pre-allocate 
	while still using the new and delete operators.

	FIXME 16 byte align
*/
class LinearMemoryPool
{
  public:
	/// Construct, and pass in the amount of memory needed, in bytes.
	LinearMemoryPool( unsigned totalMemory );
	~LinearMemoryPool();

	/// Allocate a chunk of memory.
	void* Alloc( unsigned allocate );

	/** Note this does nothing.
	*/
	void Free( void* )		{}

	/// Return true of this is out of memory.
	bool OutOfMemory()			{ return current == end; }

  public:
	char*	base;
	char*	current;
	char*	end;
};

};

#endif
