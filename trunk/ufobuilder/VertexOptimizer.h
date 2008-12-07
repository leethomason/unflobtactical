#ifndef VERTEX_OPTIMIZER_INCLUDED
#define VERTEX_OPTIMIZER_INCLUDED

#if defined(_MSC_VER)
typedef signed __int8		int8_t;
typedef unsigned __int8		uint8_t;
typedef signed __int16		int16_t;
typedef unsigned __int16	uint16_t;
typedef signed __int32		int32_t;
typedef unsigned __int32	uint32_t;
#else
#include <stdint.h>
#endif

namespace vertexoptimizer
{
struct VertexData
{
	void Clear() {
		positionInCache = -1;
		score = 0.0f;
		usedByTotal = 0;
		usedByRemaining = 0;
		triangles = 0;
	}

	int positionInCache;	// -1 if not
	float score;
	int usedByTotal;
	int usedByRemaining;	// set and managed by the cache.
	int *triangles;			// which triangles use this vertex
};

struct TriData
{
	void Clear() {
		inDrawList = false;
	}

	bool inDrawList;
};


struct FIFOCache
{
	enum {
		MAX_SIZE = 32
	};
	int cache[ MAX_SIZE ];
	int size;
	int pos;

	FIFOCache( int size );
	void Add( int value )	{	cache[pos] = value; 
								++pos;
								if ( pos == size ) pos = 0;
							}
	bool Find( int value )	{	for( int i=0; i<size; ++i ) {
									if ( cache[i] == value )
										return true;
								}
								return false;
							}
};


struct LRUCache
{
	enum { 
		SIZE = 32,
		SIZE_PADDED = SIZE + 3
	};
	int cache[SIZE+3];
	int size;
	VertexData* vertexData;
	const TriData* triData;

	LRUCache( VertexData*, const TriData* );

	void PushFront( int a );	// will only add vertices "inUse"
	void PushBack( int a );		// will only add vertices "inUse"
	void Remove( int vertex );
	bool Empty() { return cache[0] == -1; }
	void CalcInUse();							// call when triData has changed
	void CalcInUseVertex( int i );
	void FlushUnused();							// get rid of unused vertices
	void Validate();
};


struct Set
{
	enum { MAX_SIZE = LRUCache::SIZE*6 };
	int array[MAX_SIZE];
	int size;

	Set() : size( 0 ) {}
	void Clear() { size=0; }

	void Add( int value );
	int Search( int value );	// returns index or -1

};

struct VertexX
{
	int32_t x;
	int32_t y;
	int32_t z;
};

struct Vertex
{
	float x;
	float y;
	float z;
};

class VertexOptimizer
{
public:
	enum {
		MAX_VERTEX_CACHE_SIZE = 32
	};

	VertexOptimizer();
	~VertexOptimizer();

	bool SetBuffers( uint16_t* index, int32_t nIndex, int32_t* vertex, int32_t stride, int32_t nVertex );

	void Optimize();

	float Vertex_ACMR( int vertexCacheSize = 16 );
	float Memory_ACMR();

private:
	void Swap( int* a, int* b ) { int t = *a; *a = *b; *b = t; }
	void ScoreVertex( VertexData* );

	// As always (and as always confusing) a:
	// Triangle is 3 indices.
	// An index is an offset into the vertex array.

	const VertexX* GetVertexX( int a ) {
		VertexX* vertex = (VertexX*)(vertexBuffer + a*vertexStride32 );
		return vertex;
	}
	void VertexXToVertex( const VertexX& vX, Vertex* v ) {
		v->x = (float)vX.x / 65536.0f;
		v->y = (float)vX.y / 65536.0f;
		v->z = (float)vX.z / 65536.0f;
	}
	void GetTriIndices( uint16_t tri, uint16_t* target ) {
		*(target+0) = indexBuffer[tri*3+0];
		*(target+1) = indexBuffer[tri*3+1];
		*(target+2) = indexBuffer[tri*3+2];
	}

	uint16_t* indexBuffer;		// index = indexBuffer[...]
	int32_t nIndex;
	uint16_t* targetIndexBuffer;	
	int32_t* vertexBuffer;		// Fixed point value. 16.16.
	int32_t vertexStride32;		// Size of the stride in 4 byte chunks
	int32_t nVertex;

	VertexData* vertexData;		// metadata about the vertices
	int nVertexData;
	TriData* triData;
	int nTriData;
	int* trianglePool;			// memory pool for VertexData

	Set* triSet;
};

};	// vertexoptimizer

#endif //  VERTEX_OPTIMIZER_INCLUDED