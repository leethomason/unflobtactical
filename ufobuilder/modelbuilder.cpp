#include "modelbuilder.h"
#include "../engine/vertex.h"
#include "../grinliz/glgeometry.h"


using namespace grinliz;

void ModelBuilder::SetTexture( const char* textureName )
{
	GLASSERT( strlen( textureName ) < 16 );
	current = 0;

	for( int i=0; i<nGroup; ++i ) {
		if ( strcmp( textureName, group[i].textureName ) == 0 ) {
			current = &group[i];
			break;
		}
	}
	if ( !current ) {
		GLASSERT( nGroup < EL_MAX_MODEL_GROUPS );
		strcpy( group[nGroup].textureName, textureName );
		current = &group[nGroup];
		++nGroup;
	}
}


void ModelBuilder::AddTri( const Vertex& v0, const Vertex& v1, const Vertex& v2 )
{
	GLASSERT( current );
	GLASSERT( current->nIndex < EL_MAX_INDEX_IN_GROUP-3 );

	const Vertex v[3] = { v0, v1, v2 };
	VertexX vX[3];

	// Transform the vertex into the current coordinate space.
	for( int i=0; i<3; ++i ) {
		Vertex vert = v[i];
		MultMatrix4( matrix, v[i].pos, &vert.pos );
		//Trouble if there is ever rotation, because the normals shouldn't be translated.
		GLASSERT( matrix.IsTranslationOnly() );
		vert.normal.Normalize();

		vX[i].From( vert );
	}

	const float CLOSE = 0.7f;

	for( int i=0; i<3; ++i ) 
	{
		// Good idea that doesn't work: try to limit the scan back.
		// Fortuneatly not a real time algorithm...
		// int start = grinliz::Max( (int)0, current->nVertex - SCAN_BACK );
		int start = 0;
		bool added = false;

		for( int j=start; j<current->nVertex; ++j ) 
		{
			const VertexX& vc = current->vertex[j];

			if (    vc.pos == vX[i].pos
				 && vc.tex == vX[i].tex )
			{
				// We might have a match. Look at the normals. Are they
				// similar enough?
				Vector3F normal0 = { FixedToFloat( vc.normal.x ), FixedToFloat( vc.normal.y ), FixedToFloat( vc.normal.z ) };
				Vector3F normal1 = {	FixedToFloat( vX[i].normal.x ), 
										FixedToFloat( vX[i].normal.y ), 
										FixedToFloat( vX[i].normal.z ) };

				float dot = DotProduct( normal0, normal1 );

				if ( dot > CLOSE ) {
					added = true;
					current->index[ current->nIndex ] = j;
					current->normalSum[j] = current->normalSum[j] + normal1;
					current->nIndex++;
					break;
				}
			}
		}
		if ( !added ) {
			GLASSERT( current->nVertex < EL_MAX_VERTEX_IN_GROUP );
			current->normalSum[ current->nVertex ].Set( FixedToFloat( vX[i].normal.x ), 
														FixedToFloat( vX[i].normal.y ), 
														FixedToFloat( vX[i].normal.z ) );
			current->index[ current->nIndex++ ] = current->nVertex;
			current->vertex[ current->nVertex++ ] = vX[i];
		}
	}
	// Always add 3 indices.
	GLASSERT( current->nIndex % 3 == 0 );
}


void ModelBuilder::Flush()
{
	int i;

	// We've been keeping the normals as a sum. In this code, compute the final normal and assign it.
	for( int i=0; i<nGroup; ++i ) {
		for( int j=0; j<group[i].nVertex; ++j ) {
			group[i].normalSum[j].Normalize();
			group[i].vertex[j].normal.Set(	FloatToFixed( group[i].normalSum[j].x ),
											FloatToFixed( group[i].normalSum[j].y ),
											FloatToFixed( group[i].normalSum[j].z ) );

		}
	}

	// Get rid of empty groups.
	i=0; 
	while ( i<nGroup ) {
		if ( group[i].nVertex == 0 ) {
			for( int k=i; k<nGroup-1; ++k ) {
				group[k] = group[k+1];
			}
			--nGroup;
		}
		else {
			++i;
		}
	}

	bounds[0] = bounds[1] = group[0].vertex[0].pos;
	for( i=0; i<nGroup; ++i ) {
		for( int j=0; j<group[i].nVertex; ++j ) {
			bounds[0].x = Min( bounds[0].x, group[i].vertex[j].pos.x );
			bounds[0].y = Min( bounds[0].y, group[i].vertex[j].pos.y );
			bounds[0].z = Min( bounds[0].z, group[i].vertex[j].pos.z );

			bounds[1].x = Max( bounds[1].x, group[i].vertex[j].pos.x );
			bounds[1].y = Max( bounds[1].y, group[i].vertex[j].pos.y );
			bounds[1].z = Max( bounds[1].z, group[i].vertex[j].pos.z );
		}
	}



	// Either none of this code works, or it happens as a consequence of how I implemented
	// vertex filtering. But I'm frustrating with getting it all to work, so I'm commenting
	// it all out for now.
	/*
	vertexoptimizer::VertexOptimizer vOpt;
	for( i=0; i<nGroup; ++i ) {
		if ( vOpt.SetBuffers( group[i].index, group[i].nIndex, &group[i].vertex[0].pos.x, sizeof(VertexX), group[i].nVertex ) ) {
			group[i].vertex_InACMR = vOpt.Vertex_ACMR();
			vOpt.Optimize();
			group[i].vertex_OutACMR = vOpt.Vertex_ACMR();;
		}
	}
	*/
	/*
	for( i=0; i<nGroup; ++i ) {
		printf( "group %d mACMR=%.2f->", i, MemoryACMR( group[i].index, group[i].nIndex ) );
		ReOrderVertices( &group[i] );
		printf( "%.2f\n", MemoryACMR( group[i].index, group[i].nIndex ) );

	}
	*/
}


float ModelBuilder::MemoryACMR( const U16* index, int nIndex )
{
	const int CACHE_SIZE = 16;
	const int MASK = (~(CACHE_SIZE-1));
	int cacheBase = 0;
	int cacheMiss = 0;

	for( int i=0; i<nIndex; ++i ) {
		if ( (index[i] & MASK) == cacheBase ) {
			// cache hit
		}
		else {
			++cacheMiss;
			cacheBase = index[i] & MASK;
		}
 	}
	return (float)cacheMiss / (float)nIndex;
}


void ModelBuilder::ReOrderVertices( VertexGroup* group ) 
{
	/*
	for( int i=0; i<group->nIndex; ++i ) {
		indexMap[i] = -1;
	}

	// Vertices in the vertex buffer, but don't change the order of the triangles.
	int nextIndex = 0;
	for( int i=0; i<group->nIndex; ++i ) {
		int vertexID = group->index[i];

		if ( indexMap[vertexID] == -1 ) {
			indexMap[vertexID] = nextIndex++;
			targetVertex[indexMap[vertexID]] = group->vertex[vertexID];
		}
		targetIndex[i] = indexMap[vertexID];
	}
	memcpy( group->index, targetIndex, sizeof(U16)*group->nIndex );
	memcpy( group->vertex, targetVertex, sizeof(VertexX)*group->nVertex );
	*/
}