/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "modelbuilder.h"
#include "../engine/vertex.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glstringutil.h"

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
		StrNCpy( group[nGroup].textureName, textureName, 16 );
		current = &group[nGroup];
		++nGroup;
	}
}


void ModelBuilder::AddTri( const Vertex& v0, const Vertex& v1, const Vertex& v2 )
{
	GLASSERT( current );
	GLASSERT( current->nIndex < EL_MAX_INDEX_IN_GROUP-3 );

	const Vertex v[3] = { v0, v1, v2 };
	Vertex vX[3];

	// Transform the vertex into the current coordinate space.
	for( int i=0; i<3; ++i ) {
		Vertex vert = v[i];
		MultMatrix4( matrix, v[i].pos, &vert.pos );
		//Trouble if there is ever rotation, because the normals shouldn't be translated.
		GLASSERT( matrix.IsTranslationOnly() );
		vert.normal.Normalize();

		vX[i] = vert;
	}

	for( int i=0; i<3; ++i ) 
	{
		// Good idea that doesn't work: try to limit the scan back.
		// Fortuneatly not a real time algorithm...
		// int start = grinliz::Max( (int)0, current->nVertex - SCAN_BACK );
		int start = 0;
		bool added = false;

		for( int j=start; j<current->nVertex; ++j ) 
		{
			const Vertex& vc = current->vertex[j];

			const float EPS_POS = 0.01f;
			const float EPS_TEX = 0.001f;

			if ( Equal( vc.pos, vX[i].pos, EPS_POS ) && Equal( vc.tex, vX[i].tex, EPS_TEX ) )
			{
				Vector3F normal0 = { vc.normal.x, vc.normal.y, vc.normal.z };
				Vector3F normal1 = vX[i].normal;
				GLASSERT( Equal( normal0.Length(), 1.0f, 0.00001f ));
				GLASSERT( Equal( normal1.Length(), 1.0f, 0.00001f ));
				
				// we always match if smooth.
				float dot = 1.0f;
				dot = DotProduct( normal0, normal1 );
				if ( smooth ) {
					if ( dot > 0.2f )
						dot = 1.0f;
				}

				if ( dot > 0.95f ) {
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
			current->normalSum[ current->nVertex ] = vX[i].normal;
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

			group[i].vertex[j].normal.x = group[i].normalSum[j].x;
			group[i].vertex[j].normal.y = group[i].normalSum[j].y;
			group[i].vertex[j].normal.z = group[i].normalSum[j].z;
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

	bounds.min = bounds.max = group[0].vertex[0].pos;
	for( i=0; i<nGroup; ++i ) {
		for( int j=0; j<group[i].nVertex; ++j ) {
			bounds.min.x = Min( bounds.min.x, group[i].vertex[j].pos.x );
			bounds.min.y = Min( bounds.min.y, group[i].vertex[j].pos.y );
			bounds.min.z = Min( bounds.min.z, group[i].vertex[j].pos.z );

			bounds.max.x = Max( bounds.max.x, group[i].vertex[j].pos.x );
			bounds.max.y = Max( bounds.max.y, group[i].vertex[j].pos.y );
			bounds.max.z = Max( bounds.max.z, group[i].vertex[j].pos.z );
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