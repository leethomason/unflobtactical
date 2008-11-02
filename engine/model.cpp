#include "model.h"
#include "surface.h"
#include "platformgl.h"
#include "enginelimits.h"

void ModelLoader::Load( FILE* fp, ModelResource* res )
{
	U32 nTotalVertices;
	U32 nTotalIndices;
	
	fread( res->name, 16, 1, fp );

	fread( &res->nGroups, 4, 1, fp );
	fread( &nTotalVertices, 4, 1, fp );
	fread( &nTotalIndices, 4, 1, fp );

	res->nGroups = grinliz::SwapBE32( res->nGroups );
	nTotalVertices = grinliz::SwapBE32( nTotalVertices );
	nTotalIndices = grinliz::SwapBE32( nTotalIndices );

	GLASSERT( nTotalVertices <= EL_MAX_VERTEX_IN_GROUP );
	GLASSERT( nTotalIndices <= EL_MAX_INDEX_IN_GROUP );

	for( U32 i=0; i<res->nGroups; ++i )
	{
		char textureName[16];
		fread( textureName, 16, 1, fp );

		const Texture* t = 0;
		if ( textureName[0] ) {
			for( int j=0; j<nTextures; ++j ) {
				if ( strcmp( textureName, texture[j].name ) ) {
					t = &texture[j];
					break;
				}
			}
		}
		res->texture[i] = t;

		U32 startVertex, nVertex;
		fread( &startVertex, 4, 1, fp );
		fread( &nVertex, 4, 1, fp );
		startVertex = grinliz::SwapBE32( startVertex );
		nVertex = grinliz::SwapBE32( nVertex );

		fread( &res->startIndex[i], 4, 1, fp );
		fread( &res->nIndex[i], 4, 1, fp );
		res->startIndex[i] = grinliz::SwapBE32( res->startIndex[i] );
		res->nIndex[i] = grinliz::SwapBE32( res->nIndex[i] );
	}
	fread( vertex, sizeof(Vertex), nTotalVertices, fp );
	fread( index, sizeof(U16), nTotalIndices, fp );
	grinliz::SwapBufferBE16( index, nTotalIndices );

	
	// Load to VBO
	glGenBuffers( 1, (GLuint*) &res->dataID );
	glGenBuffers( 1, (GLuint*) &res->indexID );
	CHECK_GL_ERROR

	glBindBuffer( GL_ARRAY_BUFFER, res->dataID );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, res->indexID );
	CHECK_GL_ERROR

	// Data (vertex) buffer
	U32 dataSize = sizeof(Vertex)*nTotalVertices;
	U32 indexSize = sizeof(U16)*nTotalIndices;

	glBufferData( GL_ARRAY_BUFFER, dataSize, vertex, GL_STATIC_DRAW );
	CHECK_GL_ERROR

	// Index buffer
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, index, GL_STATIC_DRAW );

	// unbind
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}


void Model::Draw()
{
	//glEnable( GL_LIGHTING );
	//glEnable( GL_LIGHT0 );
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glBindBuffer(GL_ARRAY_BUFFER, resource->dataID );			// for vertex coordinates
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resource->indexID );	// for indices
	CHECK_GL_ERROR;

	glPushMatrix();
	glTranslatef( pos.x, pos.y, pos.z );

	for( U32 i=0; i<resource->nGroups; ++i ) {
		glVertexPointer(3, GL_FLOAT, 0, (const GLvoid*)Vertex::POS_OFFSET);			// last param is offset, not ptr
		glNormalPointer( GL_FLOAT, 0, (const GLvoid*)Vertex::NORMAL_OFFSET);		
		glTexCoordPointer( 3, GL_FLOAT, 0, (const GLvoid*)Vertex::TEXTURE_OFFSET);  
		CHECK_GL_ERROR;

		// mode, count, type, indices
		glDrawElements( GL_TRIANGLES, 
						resource->nIndex[i],
						GL_UNSIGNED_SHORT, 
						(const GLvoid*)(resource->startIndex[i]*sizeof(U16) ) );
		CHECK_GL_ERROR;
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glPopMatrix();

	//glDisable( GL_LIGHTING );
}

