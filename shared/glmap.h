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
	CMapBase( bool stringKey, int initialSize );
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
	int NumBuckets() const	{ return nBuckets; }
	int NumItems() const	{ return nItems; }

private:
	unsigned Hash( const char* p ) {
		return stringKey ? HashStr(p) : (unsigned)(p);
	}
	bool Equal( const char* s0, const char* s1 ) {
		return stringKey ? CStrEqual( s0, s1 ) : (s0==s1);
	}

	bool CStrEqual( const char* s0, const char* s1 ) {
		bool same = ( s0 == s1 ) || (strcmp( s0, s1 ) == 0);
		GLASSERT( !same || strcmp( s0, s1 )==0 );	// if this fires, we have 2 pointers to 2 different strings, and the entire memory model is blown
		return same;
	}
	bool stringKey;
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
	CStringMap( int initialSize=64 ) : cmap( true, initialSize )	{ GLASSERT( sizeof( V ) <= sizeof( void* ) ); }
	~CStringMap()										{}

	void Clear()										{ cmap.Clear(); }

	void Add( const char* key, V value )				{ cmap.Add( key, (void*)value ); }
	void Remove( const char* key )						{ cmap.Remove( key ); }
	
	V Get( const char* key )							{ return (V)cmap.Get( key ); }
	bool Query( const char* key, V* value )				{ return cmap.Query( key, (void**)value ); }
	bool Empty() const									{ return cmap.NumItems() == 0; }

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

template <class KEY, class V>
class CMap
{
public:
	CMap( int initialSize=64 ) : cmap( false, initialSize )	{}
	~CMap()												{}

	void Clear()										{ cmap.Clear(); }

	void Add( KEY key, V value )						{ cmap.Add( (const char*)key, (void*)value ); }
	void Remove( KEY key )								{ cmap.Remove( (const char*)key ); }
	
	V Get( KEY key )									{ return (V)cmap.Get( (const char*)key ); }
	bool Query( KEY key, V* value )						{ return cmap.Query( (const char*)key, (void**)value ); }

private:
	CMapBase cmap;
};

#endif // CMAP_INCLUDED


