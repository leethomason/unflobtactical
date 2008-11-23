#include "modelbuilder.h"
#include "../engine/vertex.h"


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

	for( int i=0; i<3; ++i ) {
		Vertex vert = v[i];
		MultMatrix4( matrix, v[i].pos, &vert.pos );
		//Trouble if there is ever rotation.
		//MultMatrix4( matrix, v[i].normal, &vert.normal );
		vert.normal.Normalize();

		vX[i].From( vert );
	}

	for( int i=0; i<3; ++i ) 
	{
		int start = grinliz::Max( (int)0, current->nVertex - SCAN_BACK );
		bool added = false;

		for( int j=start; j<current->nVertex; ++j ) 
		{
			if ( current->vertex[j].Equal( vX[i] ) ) {
				added = true;
				current->index[ current->nIndex++ ] = j;
				break;
			}
		}
		if ( !added ) {
			GLASSERT( current->nVertex < EL_MAX_VERTEX_IN_GROUP );
			current->index[ current->nIndex++ ] = current->nVertex;
			current->vertex[ current->nVertex++ ] = vX[i];
		}
	}
	// Always add 3 indices.
	GLASSERT( current->nIndex % 3 == 0 );
}
