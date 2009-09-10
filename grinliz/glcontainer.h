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


#ifndef GRINLIZ_CONTAINER_INCLUDED
#define GRINLIZ_CONTAINER_INCLUDED

#include "gldebug.h"
#include "gltypes.h"

namespace grinliz
{
/*
template< typename T, typename Equal >
class PtrHashTable
{
public:
	PtrHashTable( int _size = 256 ) : size( 0 ), population( 0 ), table( 0 )
	{
		Resize( _size );
	}

	~PtrHashTable() { delete [] table; }

	void Clear()
	{
		memset( table, 0, size*sizeof(T*) );
		population = 0;
	}

	void Put( T* t ) 
	{
		if ( population > size / 2 ) {
			GLOUTPUT(( "PtrHashTable resize %d to %d\n", size, size*4 ));
			Resize( size*4 );
		}

		U32 hash = t->HashCode();
		unsigned index = hash % size;
		GLASSERT( index < size );

		// Linear probe
		while( table[index] ) {
			++index;
			if ( index == size )
				index = 0;
		}
		table[index] = t;
		++population;
	}

	T* Contains( const T& key ) 
	{
		U32 hash = key.HashCode();
		unsigned index = hash % size;
		while( table[index] ) 
		{
			Equal equal;
			if ( equal( *(table[index]), key ) )
				return table[index];

			++index;
			if ( index == size )
				index = 0;
		}
		return 0;
	}

	void Resize( unsigned newSize ) 
	{
		if ( newSize < population / 4 )
			newSize = population*4;

		T** oldTable = table;
		unsigned oldSize = size;

		size = newSize;
		table = glnew T*[size];
		Clear();

		for( unsigned i=0; i<oldSize; ++i ) 
		{
			if ( oldTable[i] )
				Put( oldTable[i] );
		}
		delete [] oldTable;
	}

private:
	unsigned size;
	unsigned population;
	T** table;
};


class MapIdToVoidPtr
{
public:
	MapIdToVoidPtr(  U32 _size = 256 );
	~MapIdToVoidPtr();

	void Clear();
	void Insert( U32 i, const void* t );
	const void* Get( U32 i );
	bool Remove( U32 i );

	U32 NumSlot()						{ return size; }
	bool GetSlot( U32 offset, const void** v, U32* i )	
	{	
		GLASSERT( offset < size );
		if ( table[offset].i >= 0 ) {
			*v = table[offset].ptr;
			*i = table[offset].i;
			return true;
		}
		return false;
	}

private:
	U32 HashCode( U32 hash )
	{
		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);
		return (U32) hash;
	}

	void Resize( U32 newSize );

	// 'ptr' can have any value.
	// 'i'  >= 0, index
	//      -1 empty
	//		-2 deleted
	struct Node {
		int i;
		const void* ptr;
	};
	enum {
		NODE_EMPTY = -1,
		NODE_DELETED = -2,
	};

	U32 size;
	U32 population;
	Node* table;
};

template< typename T >
class MapId
{
public:
	MapId(  U32 _size = 256 ) : map( _size ) {
		GLASSERT( sizeof( T ) <= sizeof( void* ) );
		idPool = 0;
	}
	~MapId()	{}

	void Clear()							{ map.Clear(); }
	void Insert( U32 i, const T t )			{ map.Insert( i, t ); }
	const T Get( U32 i )					{ return (const T)map.Get( i ); }
	bool Remove( U32 i )					{ return map.Remove( i ); }

	U32 NumSlot()										{ return map.NumSlot(); }
	bool GetSlot( U32 offset, T* v, U32* i )		
	{ 
		const void* vPtr = 0;
		bool result = map.GetSlot( offset, &vPtr, i ); 
		*v = (T)(vPtr);
		return result;
	}

	U32 NextID()							{ return idPool++; }
private:
	MapIdToVoidPtr map;
	U32 idPool;
};
*/
}	// namespace grinliz
#endif
