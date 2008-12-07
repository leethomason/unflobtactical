#include "VertexOptimizer.h"
#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>

using namespace vertexoptimizer;

#ifdef _DEBUG
#	ifndef DEBUG
#		define DEBUG
#	endif
#endif

#ifdef DEBUG
#	if defined(_MSC_VER)
		#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
		#include <windows.h>
#		define ASSERT( x )		if ( !(x)) DebugBreak()
#	else
#		define ASSERT assert
#	endif
#endif

FIFOCache::FIFOCache( int size ) 
{ 
	this->size = size; 
	if ( size > MAX_SIZE ) 
		size = MAX_SIZE; 
	for( int i=0; i<size; ++i ) {
		cache[i] = -1;
	}
	pos = 0;
	size = 0;
}


LRUCache::LRUCache( VertexData* vd, const TriData* td )
{
	vertexData = vd;
	triData = td;
	size = 0;
	for( int i=0; i<SIZE_PADDED; ++i )
		cache[i] = -1;
}

void LRUCache::PushFront( int vertex )
{
	if ( vertex < 0 )
		return;

	CalcInUseVertex( vertex );
	if ( vertexData[vertex].usedByRemaining == 0 ) 
		return;

	VertexData* vData = &vertexData[vertex];
	ASSERT( vData->usedByRemaining > 0 );

	int start = SIZE_PADDED-1;
	if ( vData->positionInCache >= 0 ) {
		start = vData->positionInCache;
		ASSERT( cache[ vData->positionInCache ] == vertex );
	}
	else {
		++size;
	}
	for( int j=start; j>0; --j ) {
		cache[j] = cache[j-1];
		vertexData[cache[j]].positionInCache = j;
	}
	cache[0] = vertex;
	vertexData[cache[0]].positionInCache = 0;

	// What fell off the back?
	for( int i=SIZE; i<SIZE_PADDED; ++i ) {
		if ( cache[i] >= 0 ) {
			vertexData[ cache[i] ].positionInCache = -1;
			cache[i] = -1;
			--size;
		}
	}
	/*
	for( int i=0; i<size; ++i ) {
		printf( "%d ", cache[i] );
	}
	printf( "\n" );
	*/
	Validate();
}


void LRUCache::Validate() 
{
#ifdef DEBUG
	for( int i=0; i<size; ++i ) {
		ASSERT( cache[i] >= 0 );
		ASSERT( vertexData[cache[i]].positionInCache == i );
		//ASSERT( vertexData[cache[i]].usedByRemaining > 0 );	// check in FlushUnused()
		//ASSERT( vertexData[cache[i]].score <= 3.0f );
	}
	for( int i=size; i<SIZE; ++i ) {
		ASSERT( cache[i] = -1 );
	}
#endif
}


void LRUCache::PushBack( int vertex ) 
{
	CalcInUseVertex( vertex );
	if ( vertexData[vertex].usedByRemaining == 0 ) 
		return;

	ASSERT( size < SIZE );
	cache[size] = vertex;
	vertexData[cache[size]].positionInCache = size;
	++size;
	Validate();
}


void LRUCache::Remove( int vertex )
{
	ASSERT( cache[ vertexData[vertex].positionInCache ] == vertex );
	ASSERT( vertexData[vertex].positionInCache >= 0 );

	for( int j=vertexData[vertex].positionInCache; j<SIZE-1; ++j ) {
		cache[j] = cache[j+1];
		vertexData[cache[j]].positionInCache = j;
	}
	vertexData[vertex].positionInCache = -1;
	--size;
	Validate();
}


void LRUCache::CalcInUseVertex( int vertex )
{
	int inUse = 0;
	for( int j=0; j<vertexData[vertex].usedByTotal; ++j ) {
		if ( !triData[ vertexData[vertex].triangles[j] ].inDrawList ) {
			inUse++;
		}
	}
	vertexData[vertex].usedByRemaining = inUse;
}


void LRUCache::CalcInUse()
{
	for( int i=0; i<size; ++i ) {
		CalcInUseVertex(cache[i]);
	}
}


void LRUCache::FlushUnused()
{
	int i=0;
	while( i < size ) {
		if ( vertexData[cache[i]].usedByRemaining == 0 ) {
			Remove( cache[i] );
		}
		else {
			++i;
		}
	}
	#ifdef DEBUG
	{
		for( int i=0; i<size; ++i ) {
			ASSERT( vertexData[cache[i]].usedByRemaining > 0 );
		}
		Validate();
	}
	#endif
}


int Set::Search( int value )
{
	int low = 0;
	int high = size - 1;

	while (low <= high) {
		int mid = (low + high) / 2;
		if (array[mid] > value) {
			high = mid - 1;
		}
		else if (array[mid] < value) {
			low = mid + 1;
		}
		else {
			return mid;
		}
	}
	return -1;
}


void Set::Add( int value )
{
	if ( Search( value ) < 0 ) {
		ASSERT( size < MAX_SIZE );

		int i;
		for( i=0; i<size; ++i ) {
			if ( array[i] > value ) 
				break;
		}
		// Move up
		for( int j=size; j>i; j-- ) {
			array[j] = array[j-1];
		}
		array[i] = value;
		++size;
	}
#ifdef DEBUG
	for( int i=1; i<size; ++i ) {
		ASSERT( array[i] > array[i-1] );
	}
#endif
}

	
VertexOptimizer::VertexOptimizer()
{
	indexBuffer = 0;
	targetIndexBuffer =0;
	nIndex = 0;
	vertexBuffer = 0;
	nVertex = 0;
	vertexData = 0;
	triData = 0;
	nTriData = 0;
	trianglePool = 0;
	nVertexData = 0;
	triSet = new Set();
}


VertexOptimizer::~VertexOptimizer()
{
	delete [] vertexData;
	delete [] triData;
	delete [] trianglePool; 
	delete [] targetIndexBuffer;
	delete triSet;
}


bool VertexOptimizer::SetBuffers( uint16_t* index, int32_t nIndex, int32_t* vertex, int32_t vertexStride, int32_t nVertex )
{
	this->nIndex = 0;
	this->nVertex = 0;

	if ( nIndex > LRUCache::SIZE && nVertex > LRUCache::SIZE ) {
		ASSERT( index );
		ASSERT( nIndex % 3 == 0 );
		ASSERT( vertex );
		ASSERT( vertexStride%4 == 0 );

		this->indexBuffer = index;
		this->nIndex = nIndex;
		this->vertexBuffer = vertex;
		this->vertexStride32 = vertexStride>>2;
		this->nVertex = nVertex;
		return true;
	}
	return false;
}


float VertexOptimizer::Vertex_ACMR( int vertexCacheSize )
{
	FIFOCache cache( vertexCacheSize );

	int cacheMiss = 0;
	int nTri = nIndex / 3;
	ASSERT( nIndex % 3 == 0 );

	for( int i=0; i<nTri; ++i ) {
		//printf( "Tri [%3d]: ", i );
		int hit = 0;
		int miss = 0;

		for( int j=0; j<3; ++j ) {
			int vertex = indexBuffer[i*3+j];
			//printf( "%3d ", vertex ); 

			if ( !cache.Find( vertex ) ) {
				cache.Add( vertex );
				++miss;
			}
			else {
				++hit;
			}
		}
		//printf( " hit=%d miss=%d\n", hit, miss );
		cacheMiss += miss;
	}
	float amcr = (float)cacheMiss/(float)(nTri);
	return amcr;
}


void VertexOptimizer::ScoreVertex( VertexData* vd )
{
	const float CACHE_DECAY_POWER = 1.5f;
	const float LAST_TRI_SCORE = 0.75f;
	const float VALENCE_BOOST_SCALE = 2.0f;
	const float VALENCE_BOOST_POWER = 0.5f;

	if ( vd->usedByRemaining == 0 ) {
		vd->score = -1.0f;
		return;
	}

	float score = 0.0f;
	if ( vd->positionInCache < 0 ) {
		score = 0.0f;
	}
	else if ( vd->positionInCache < 3 ) {
		score = LAST_TRI_SCORE;
	}
	else {
		ASSERT( vd->positionInCache < LRUCache::SIZE );
		const float scale = 1.0f / ( LRUCache::SIZE-3 );
		score = 1.0f - (float)(vd->positionInCache - 3)*scale;
		score = powf( score, CACHE_DECAY_POWER );
		ASSERT( score <= 1.0f );
	}

	float valenceBoost = powf( (float)vd->usedByRemaining, -VALENCE_BOOST_POWER );
	float valenceScore = VALENCE_BOOST_SCALE * valenceBoost;
	//ASSERT( valenceScore <= 2.0f );

	score += valenceScore;
	//ASSERT( score <= 3.0f );

	vd->score = score;
}


void VertexOptimizer::Optimize()
{
	// Working array - allocated on demand with a little overhead.
	if ( nVertexData < nVertex ) {
		delete [] vertexData;
		nVertexData = nVertex; // * 5 / 4;
		vertexData = new VertexData[nVertexData];
	}
	ASSERT( nIndex % 3 == 0 );
	int nTri = nIndex / 3;

	if ( nTriData < nTri ) {
		delete [] triData;
		triData = new TriData[nTri];
	}
	delete [] targetIndexBuffer;
	targetIndexBuffer = new uint16_t[nIndex];
	delete [] trianglePool;
	trianglePool = new int[nIndex];

	// Initialize the vertices.
	for( int i=0; i<nVertex; ++i ) {
		vertexData[i].Clear();
	}
	// Initialize the triangles
	for( int i=0; i<nTri; ++i ) {
		triData[i].Clear();
	}

	// Count tris used by each vertex
	for( int i=0; i<nTri; ++i ) {
		for( int j=0; j<3; ++j ) {
			int vertex = indexBuffer[i*3+j];
			vertexData[ vertex ].usedByTotal++;
		}
	}

	// Distribute memory:
	int* triMem = trianglePool;
	for( int i=0; i<nVertex; ++i ) {
		ASSERT( triMem < &trianglePool[nIndex] );
		vertexData[i].triangles = triMem;
		triMem += vertexData[i].usedByTotal;
	}

	// Assign tris:
	for( int tri=0; tri<nTri; ++tri ) {
		for( int j=0; j<3; ++j ) {
			int vertex = indexBuffer[tri*3+j];
			vertexData[vertex].triangles[ vertexData[vertex].usedByRemaining++ ] = tri;
		}
	}
#ifdef DEBUG
	for( int i=0; i<nVertex; ++i ) {
		ASSERT( vertexData[i].usedByRemaining == vertexData[i].usedByTotal );
	}
#endif

	ASSERT( nIndex  > LRUCache::SIZE );
	ASSERT( nVertex > LRUCache::SIZE );

	// Initialize the LRUCache
	LRUCache lruCache( vertexData, triData );

	for(	int seedVertex = 0; 
			lruCache.size < LRUCache::SIZE && seedVertex < nVertex;
			++seedVertex ) 
	{
		lruCache.PushFront( seedVertex );
	}
	int nextVertex = LRUCache::SIZE;
	int nextTargetIndex = 0;

	while ( !lruCache.Empty() ) 
	{
		// The cache is a cache of vertices. While the TriSet is the complete
		// set of triangles. This means that there will be vertices used by
		// the triangles that are NOT in the cache. They always have a score of 0.0,
		// which is a little inaccurate but keeps things sane.
		
		for( int i=0; i<lruCache.size; ++i ) {
			ScoreVertex( &vertexData[ lruCache.cache[i] ] );
		}

		// Go through all the triangles. Put in a set so we don't look at them multiple times.
		triSet->Clear();

		for( int i=0; i<lruCache.size; ++i ) 
		{
			ASSERT( lruCache.cache[i] >= 0 );
			int vertex = lruCache.cache[i];
			ASSERT( vertexData[vertex].usedByRemaining > 0 );	// should be filtered out

			// OPT: if the list is sorted with usedByRemaining in the front, then
			// that could be used.
			for( int j=0; j<vertexData[vertex].usedByTotal; ++j ) {
				int tri = vertexData[vertex].triangles[j];
				ASSERT( tri < nTri );
				if ( !triData[tri].inDrawList ) {
					triSet->Add( tri );
				}
			}
		}
		ASSERT( triSet->size > 0 );

		int bestTri = -1;
		float bestScore = -2000.0f;
		for( int i=0; i<triSet->size; ++i ) 
		{
			int tri = triSet->array[i];

			float score = 0;
			for( int j=0; j<3; ++j ) {
				if ( vertexData[ indexBuffer[tri*3+0] ].positionInCache >= 0 ) {
					score += vertexData[ indexBuffer[tri*3+j] ].score;
				}
			}
			//ASSERT( score <= 9.0f );
			if ( score > bestScore ) {
				bestScore = score;
				bestTri = tri;
			}
		}

		// We have our new triangle! Add it, and then clean up the vertices.
		ASSERT( nextTargetIndex < nIndex );
		uint16_t* bestTriIndex = targetIndexBuffer + nextTargetIndex;
		GetTriIndices( bestTri, targetIndexBuffer + nextTargetIndex );
		triData[ bestTri ].inDrawList = true;

		/*
		printf( "Tri [%3d]: %3d %3d %3d score=%5.1f LRUCache.size=%2d triSet.size=%2d\n",
				nextTargetIndex / 3,
				targetIndexBuffer[ nextTargetIndex+0 ],
				targetIndexBuffer[ nextTargetIndex+1 ],
				targetIndexBuffer[ nextTargetIndex+2 ],
				(double)bestScore,
				lruCache.size,
				triSet->size );
		*/

		nextTargetIndex += 3;
		ASSERT( nextTargetIndex <= nIndex );
			    
		// What are all the vertices associated with that triangle? A tricky problem.
		// We are only set up for a one way map. So just do a pass on the cache and 
		// detect the use counts.
		lruCache.CalcInUse();
		lruCache.FlushUnused();

		// Move the most recently used stuff to the front.
		for( int i=0; i<3; ++i ) {
			int index = bestTriIndex[i];
			lruCache.PushFront( indexBuffer[index] );
		}
		lruCache.Validate();

		// Fill in the back with additional vertices.
		while ( lruCache.size < lruCache.SIZE && nextVertex < nVertex ) 
		{
			// It's possible for a vertex not in the cache to be added
			// if the scores are low.
			if ( vertexData[nextVertex].positionInCache < 0 ) {
				lruCache.PushBack( nextVertex );
			}
			nextVertex++;
		}
		lruCache.Validate();
	}
	ASSERT( nextTargetIndex == nIndex );
	memcpy( indexBuffer, targetIndexBuffer, sizeof( uint16_t )*nIndex );
}


