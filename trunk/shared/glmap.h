/*
 *  CMap.h
 */
 
#ifndef CMAP_INCLUDED
#define CMAP_INCLUDED

#include "../grinliz/gldebug.h"
#include <memory.h>
#include <string.h>
 
class CMapBase
{
public:
	CMapBase( int initialSize=16 );
	~CMapBase();
	
	void Clear();
	void Add( const char* key, void* value );
	void Remove( const char* key );
	
	void* Get( const char* key );
	bool Query( const char* key, void** value );

	// FNV1a
	static unsigned HashStr( const char* p );

	struct Bucket
	{
		const char* key;
		void* value;
	};

	const Bucket* GetBuckets() const { return buckets; }
	int NumBuckets() const { return nBuckets; }

private:

	bool CStrEqual( const char* s0, const char* s1 ) {
		bool same = ( s0 == s1 ) || (strcmp( s0, s1 ) == 0);
		GLASSERT( !same || strcmp( s0, s1 )==0 );	// if this fires, we have 2 pointers to 2 different strings, and the entire memory model is blown
		return same;
	}
	Bucket* buckets;
	int nBuckets;		// power of 2 - avoid the divide
	int nAdds;			// number of adds - since delete doesn't clear things out, it is the adds that control when it should resive
	int nItems;
	bool recursing;
};


/*
	Hash between a const char* and a value. Important: the const char* is
	NOT copied, so it only works on static data.
*/
template <class V>
class CStringMap
{
public:
	CStringMap( int initialSize=16 ) : cmap( initialSize )	{ GLASSERT( sizeof( V ) <= sizeof( void* ) ); }
	~CStringMap()										{}

	void Clear()										{ cmap.Clear(); }

	void Add( const char* key, V value )				{ cmap.Add( key, (void*)value ); }
	void Remove( const char* key )						{ cmap.Remove( key ); }
	
	V Get( const char* key )							{ return (V)cmap.Get( key ); }
	bool Query( const char* key, V* value )				{ return cmap.Query( key, (void**)value ); }

	class Iterator {
	public:
		Iterator( const CStringMap<V>& _cmap ) : cmap( _cmap ) { i=-1; Next(); }
		void Next() {
			++i;
			const CMapBase::Bucket* b = cmap.cmap.GetBuckets();
			while ( i<cmap.cmap.NumBuckets() ) {
				if ( b[i].key > (const char*)(1) )
					break;
				++i;
			}
		}
		bool End()	{ return i == cmap.cmap.NumBuckets(); }
		V Value()	{ const CMapBase::Bucket* b = cmap.cmap.GetBuckets();
					  return (V) b[i].value; }

	private:
		const CStringMap<V>& cmap;
		int i;
	};

private:
	CMapBase cmap;
};

#endif // CMAP_INCLUDED


