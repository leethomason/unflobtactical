#include "glmap.h"
#include <stdlib.h>
#include "../grinliz/glutil.h"


CMapBase::CMapBase( bool strKey, int size )
{
	GLASSERT( sizeof( void*) == sizeof( unsigned char* ) );	// crazy just in case
	nBuckets = grinliz::CeilPowerOf2( size );
	nAdds = 0;
	nItems = 0;
	buckets = new Bucket[nBuckets];
	memset( buckets, 0, sizeof( Bucket )*nBuckets );
	recursing = false;
	stringKey = strKey;
}


CMapBase::~CMapBase()
{
	delete [] buckets;
}


void CMapBase::Clear() 
{
	nAdds = 0;
	nItems = 0;
	memset( buckets, 0, sizeof( Bucket )*nBuckets );
}


unsigned CMapBase::HashStr( const char* p )
{
	unsigned hash = 2166136261UL;
	for( ; *p; ++p ) {
		hash ^= *p;
		hash *= 16777619;
	}
	return hash;
}


unsigned CMapBase::HashVal( const void* _p )
{
	unsigned hash = 2166136261UL;
	const char* p = (const char*)_p;
	
	for( int i=0; i<sizeof(void*); ++i, ++p ) {
		hash ^= *p;
		hash *= 16777619;
	}
	return hash;
}


void CMapBase::Add( const char* key, void* value )
{
	if ( nAdds > (nBuckets/2) ) {
		GLASSERT( !recursing );
		recursing = true;
		
		Bucket* tempBuckets = buckets;
		int tempNBuckets = nBuckets;
		
		nBuckets = grinliz::CeilPowerOf2( nItems*3 );	// grow a bit. "normal" case nItems is 1/2 nBuckets when this happens. nItems*2 would be the same size, nItems*3 is 50% growth, up a power of 2.
		nAdds = 0;
		nItems = 0;
		buckets = new Bucket[nBuckets];
		memset( buckets, 0, sizeof(Bucket)*nBuckets );
		
		for( int i=0; i<tempNBuckets; ++i ) {
			if ( tempBuckets[i].key > (const char*)(1) ) {
				Add( tempBuckets[i].key, tempBuckets[i].value );
			}
		}
		
		delete [] tempBuckets;
		recursing = false;
	}
	
	unsigned h = Hash( key );
	h = h & (nBuckets-1);

	// Can use buckets[i].key == 0, because that's the end of a run.
	// ==1 regrettably tells us nothing, because the dupe could be further down.
	while ( true ) {
		if ( buckets[h].key == 0 ) {
			buckets[h].key = key;
			buckets[h].value = value;
			++nItems;
			++nAdds;
			break;
		}
		if ( buckets[h].key > (const char*)(1)  && Equal( buckets[h].key, key ) ) {	
			// dupe. Overwrite with new.
			buckets[h].value = value;
			break;
		}
		++h;
		if ( h == nBuckets )
			h = 0;
	}
}


void CMapBase::Remove( const char* key )
{
	unsigned h = Hash( key );
	h = h & (nBuckets-1);

	while ( buckets[h].key ) {
		if ( Equal( buckets[h].key, key )) {
			buckets[h].key = (const char*)(1);
			--nItems;
			break;
		}
		++h;
		if ( h == nBuckets )
			h = 0;
	}
}


void* CMapBase::Get( const char* key )
{
	unsigned h = Hash( key );
	h = h & (nBuckets-1);

	while ( buckets[h].key ) {
		if ( Equal( buckets[h].key, key )) {
			return buckets[h].value;
		}
	}
	GLASSERT( 0 );
	return 0;
}


bool CMapBase::Query( const char* key, void** value )
{
	unsigned h = Hash( key );
	h = h & (nBuckets-1);

	while ( buckets[h].key ) {
		if ( Equal( buckets[h].key, key )) {
			*value = buckets[h].value;
			return true;
		}
	}
	return false;
}
