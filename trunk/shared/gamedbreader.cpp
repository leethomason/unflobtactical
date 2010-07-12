/*
 Copyright (c) 2010 Lee Thomason

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#include <stdlib.h>
#include <string.h>

#include "gamedbreader.h"
#ifdef ANDROID_NDK
#	include <zlib.h>		// Built in zlib support.
#else
#	include "../zlib/zlib.h"
#endif

#pragma warning ( disable : 4996 )
using namespace gamedb;

struct CompStringID
{
	const char* key;
	const void* mem;
	CompStringID( const char* k, const void* r ) : key( k ), mem( r )	{}

	int operator()( U32 offset ) const {
		const char* str = (const char*)mem + offset;
		GLASSERT( str );
		return strcmp( str, key );
	}
};


struct CompChild
{
	int key;
	const void* mem;
	CompChild( int k, const void* r ) : key( k ), mem( r )	{}

	int operator()( U32 a ) const {
		const ItemStruct* istruct = (const ItemStruct*)((const char*)mem + a);
		return istruct->nameID - key;
	}
};


struct CompAttribute
{
	int key;
	CompAttribute( int k ) : key( k )	{}

	int operator()( const AttribStruct& attrib ) const {
		return (attrib.keyType&0x0fff) - key;
	}
};


template< typename T, typename C > 
class BinarySearch
{
public:
	int Search( const T* arr, int N, const C& compareFunc ) 
	{
		unsigned low = 0;
		unsigned high = N;

		while (low < high) {
			unsigned mid = low + (high - low) / 2;

			int compare = compareFunc( arr[mid] );
			if ( compare < 0 )
				low = mid + 1; 
			else
				high = mid; 
		}
		if ( ( low < (unsigned)N ) && ( compareFunc( arr[low] ) == 0 ) )
			return low;
		return -1;
	}
};



/* static */ Reader* Reader::readerRoot = 0;

/* static */ const Reader* Reader::GetContext( const Item* item )
{
	const void* m = item;

	for( Reader* r=readerRoot; r; r=r->next ) {
		if ( m >= r->mem && m < r->endMem ) {
			return r;
		}
	}
	GLASSERT( 0 );
	return 0;
}


Reader::Reader()
{
	mem = 0;
	memSize = 0;
	fp = 0;

	// Add to linked list.
	next = readerRoot;
	readerRoot = this;
	buffer = 0;
	bufferSize = 0;
	access = 0;
	accessSize = 0;
}


Reader::~Reader()
{
	// unlink from the global list.
	Reader* r = readerRoot;
	Reader* prev = 0;
	while( r && r != this ) {
		prev = r;
		r = r->next;
	}
	GLASSERT( r );

	if ( prev ) {
		prev->next = r->next;
	}
	else {
		readerRoot = r->next;
	}

	if ( mem )
		free( mem );
	if ( buffer )
		free( buffer );
	if ( access )
		free( access );
	if ( fp ) {
		fclose( fp );
	}
}


//bool Reader::Init( const void* mem, int size )
bool Reader::Init( const char* filename )
{
	fp = fopen( filename, "rb" );
	if ( !fp )
		return false;

	// Read in the data structers. Leave the "data" compressed and in the file.
	HeaderStruct header;
	fread( &header, sizeof(header), 1, fp );
	fseek( fp, 0, SEEK_SET );

	memSize = header.offsetToData;
	mem = malloc( memSize );
	endMem = (const char*)mem + memSize;

	fread( mem, memSize, 1, fp );

	root = (const Item*)( (U8*)mem + header.offsetToItems );

#if 0		// Dump string pool

	{
		int totalLen = 0;
		for( unsigned i=0; i<header.nString; ++i ) {
			const char* str = GetString( i );

			int len = strlen( str );
			if ( totalLen != 0 && totalLen + len > 60 ) {
				printf( "\n" );
				totalLen = 0;
			}
			totalLen += len;
			printf( "%s ", str );
		}
		printf( "\n" );
	}
#endif

	return true;
}


void Reader::RecWalk( const Item* item, int depth )
{
	for( int i=0; i<depth; ++i )
		printf( "  " );
	printf( "%s", item->Name() );
	
	printf( " [" );
	int n = item->NumAttributes();
	for( int i=0; i<n; ++i ) {
		const char* name = item->AttributeName( i );
		printf( " %s=", name );
		switch( item->AttributeType( i ) ) {
			case ATTRIBUTE_DATA:
				printf( "data(#%d %d)", item->GetDataID( name ), item->GetDataSize( name ) );
				break;
			case ATTRIBUTE_INT:
				printf( "%d", item->GetInt( name ) );
				break;
			case ATTRIBUTE_FLOAT:
				printf( "%f", item->GetFloat( name ) );
				break;
			case ATTRIBUTE_STRING:
				printf( "'%s'", item->GetString( name ) );
				break;
			case ATTRIBUTE_BOOL:
				printf( "%s", item->GetBool( name ) ? "TRUE" : "FALSE" );
				break;
			default:
				GLASSERT( 0 );
		}
	}
	printf( "]\n" );

	for( int i=0; i<item->NumChildren(); ++i ) {
		const Item* child = item->Child( i );
		RecWalk( child, depth + 1 );
	}
}


const char* Reader::GetString( int id ) const
{
	const HeaderStruct* header = (const HeaderStruct*)mem;
	GLASSERT( id >= 0 && id < (int)header->nString );

	const U32* stringOffset = (const U32*)((const char*)mem + sizeof(HeaderStruct));
	const char* str = (const char*)mem + stringOffset[id];

	GLASSERT( str > mem && str < endMem );
	return str;
}


int Reader::GetStringID( const char* str ) const
{
	// Trickier and more common case. Do a binary seach through the string table.
	const HeaderStruct* header = (const HeaderStruct*)mem;

	CompStringID comp( str, mem );
	const U32* stringOffset = (const U32*)((const char*)mem + sizeof(HeaderStruct));
	BinarySearch<U32, CompStringID> bsearch;
	return bsearch.Search( stringOffset, (int)header->nString, comp );
}


int Reader::GetDataSize( int dataID ) const
{
	const HeaderStruct* header = (const HeaderStruct*)mem;
	GLASSERT( header->offsetToDataDesc % 4 == 0 );

	const DataDescStruct* dataDesc = (const DataDescStruct*)((const U8*)mem + header->offsetToDataDesc);
	GLASSERT( dataID >= 0 && dataID < (int)header->nData );

	return dataDesc[dataID].size;
}


void Reader::GetData( int dataID, void* target, int memSize ) const
{
	const HeaderStruct* header = (const HeaderStruct*)mem;
	GLASSERT( header->offsetToDataDesc % 4 == 0 );

	const DataDescStruct* dataDescPtr = (const DataDescStruct*)((const U8*)mem + header->offsetToDataDesc);
	GLASSERT( dataID >= 0 && dataID < (int)header->nData );
	const DataDescStruct& dataDesc = dataDescPtr[dataID];
	
	fseek( fp, dataDesc.offset, SEEK_SET );

	if ( dataDesc.compressedSize == dataDesc.size ) {
		// no compression.
		GLASSERT( dataDesc.size == memSize );
		fread( target, memSize, 1, fp );
	}
	else {
		if ( bufferSize < (int)dataDesc.compressedSize ) {
			bufferSize = (int)dataDesc.compressedSize*5/4;
			if ( bufferSize < 1000 )
				bufferSize = 1000;
			if ( buffer )
				buffer = realloc( buffer, bufferSize );
			else 
				buffer = malloc( bufferSize );
		}
		fread( buffer, dataDesc.compressedSize, 1, fp );

		int result = uncompress(	(Bytef*)target, 
									(uLongf*)&dataDesc.size, 
									(const Bytef*)buffer,
									dataDesc.compressedSize );
		GLASSERT( result == Z_OK );
		GLASSERT( dataDesc.size == memSize );
	}
}


const void* Reader::AccessData( const Item* item, const char* name, int* p_size ) const
{
	int size = 0;
	if ( p_size ) *p_size = 0;

	if ( item->AttributeType( name ) != ATTRIBUTE_DATA ) {
		return 0;
	}
	size = item->GetDataSize( name );
	// Make sure we have enough space, including appended null terminator.
	if ( (size+1) > accessSize ) {
		accessSize = (size+1) * 5 / 4;
		if ( access )
			access = realloc( access, accessSize );
		else
			access = malloc( accessSize );
	}
	item->GetData( name, access, size );
	*((char*)access + size) = 0;	// null terminate for text assets.

	if ( p_size )
		*p_size = size;
	return access;
}


const char* Item::Name() const
{
	const Reader* context = Reader::GetContext( this );
	const ItemStruct* istruct = (const ItemStruct*)this;

	return context->GetString( istruct->nameID );	
}


const Item* Item::Child( int i ) const
{
	const Reader* context = Reader::GetContext( this );
	const U8* mem = (const U8*)context->BaseMem();

	GLASSERT( i >= 0 && i < (int)((const ItemStruct*)this)->nChildren );

	const U32* childOffset = (const U32*)((U8*)this + sizeof( ItemStruct ));
	return (const Item*)( mem + childOffset[i]);
}


const Item* Item::Child( const char* name ) const
{


	const ItemStruct* istruct = (const ItemStruct*)this;
	const U32* childOffset = (const U32*)((U8*)this + sizeof( ItemStruct ));
	const Reader* context = Reader::GetContext( this );
	const U8* mem = (const U8*)context->BaseMem();
	int nameID = context->GetStringID( name );

	if ( nameID >= 0 ) {
		CompChild comp( nameID, mem );
		BinarySearch<U32, CompChild> search;
		int result = search.Search( childOffset, istruct->nChildren, comp );
		if ( result >= 0 )
			return Child( result );
	}
	return 0;

}



int Item::NumAttributes() const
{
	const ItemStruct* istruct = (const ItemStruct*)this;
	return (int)istruct->nAttrib;
}


const AttribStruct* Item::AttributePtr( int i ) const
{
	GLASSERT( i>=0 && i<NumAttributes() );
	const ItemStruct* istruct = (const ItemStruct*)this;
	const AttribStruct* attrib = (const AttribStruct*)((U8*)this + sizeof( ItemStruct ) + istruct->nChildren*sizeof(U32) );

	return attrib + i;
}


const char* Item::AttributeName( int i ) const
{
	const AttribStruct* attrib = AttributePtr( i );
	// note only uses lower 24 bits
	U32 key = (attrib->keyType) & 0x0fff;

	const Reader* context = Reader::GetContext( this );
	return context->GetString( key );
}


int Item::AttributeType( int i ) const
{
	const AttribStruct* attrib = AttributePtr( i );
	U32 type = (attrib->keyType) >> 24;
	return (int)type;
}


int Item::AttributeType( const char* name ) const
{
	int i = AttributeIndex( name );
	if ( i >= 0 ) {
		return AttributeType( i );
	}
	return ATTRIBUTE_INVALID;
}



int Item::AttributeIndex( const char* name ) const
{

	const ItemStruct* istruct = (const ItemStruct*)this;

	const Reader* context = Reader::GetContext( this );
	int id = context->GetStringID( name );
	int n = NumAttributes();
	int result = -1;

	if ( id >= 0 && n ) {
		const AttribStruct* attrib = AttributePtr( 0 );
		CompAttribute comp( id );
		BinarySearch<AttribStruct, CompAttribute> search;
		result = search.Search( attrib, n, comp );
	}
	return result;
}


int	Item::GetDataSize( int i ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_DATA );
	if ( AttributeType( i ) == ATTRIBUTE_DATA ) {	// also range checks
		const AttribStruct* attrib = AttributePtr( i );
		const Reader* context = Reader::GetContext( this );
		return context->GetDataSize( attrib->dataID );
	}
	return -1;
}


int	Item::GetDataSize( const char* name ) const
{
	int i = AttributeIndex( name );
	if ( i >= 0 ) {
		return GetDataSize( i );
	}
	return -1;
}


void Item::GetData( int i, void* mem, int memSize ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_DATA );
	GLASSERT( GetDataSize( i ) == memSize );

	const Reader* context = Reader::GetContext( this );

	const AttribStruct* attrib = AttributePtr( i );
	context->GetData( attrib->dataID, mem, memSize );
}


void Item::GetData( const char* name, void* mem, int memSize ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0 );
	return GetData( i, mem, memSize );
}


int	Item::GetDataID( int i ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_DATA );
	const AttribStruct* attrib = AttributePtr( i );
	return attrib->dataID;
}


int	Item::GetDataID( const char* name ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0 );
	return GetDataID( i );
}


int Item::GetInt( int i ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_INT );
	if ( AttributeType( i ) == ATTRIBUTE_INT ) {
		const AttribStruct* attrib = AttributePtr( i );
		return attrib->intVal;
	}
	return 0;
}


int Item::GetInt( const char* name ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0);
	if ( i >= 0 ) {
		return GetInt( i );
	}
	return 0;
}


float Item::GetFloat( int i ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_FLOAT );
	if ( AttributeType( i ) == ATTRIBUTE_FLOAT ) {
		const AttribStruct* attrib = AttributePtr( i );
		return attrib->floatVal;
	}
	return 0.0f;
}


float Item::GetFloat( const char* name ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0);
	if ( i >= 0 ) {
		return GetFloat( i );
	}
	return 0.0f;
}


const char*	Item::GetString( int i ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_STRING );
	if ( AttributeType( i ) == ATTRIBUTE_STRING ) {
		const AttribStruct* attrib = AttributePtr( i );
		const Reader* context = Reader::GetContext( this );
		return context->GetString( attrib->stringID );
	}
	return 0;
}


const char* Item::GetString( const char* name ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0);
	if ( i >= 0 ) {
		return GetString( i );
	}
	return 0;
}


bool Item::GetBool( int i ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_BOOL );
	if ( AttributeType( i ) == ATTRIBUTE_BOOL ) {
		const AttribStruct* attrib = AttributePtr( i );
		return attrib->intVal ? true : false;
	}
	return false;
}


bool Item::GetBool( const char* name ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0);
	if ( i >= 0 ) {
		return GetBool( i );
	}
	return false;
}