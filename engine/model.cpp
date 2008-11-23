#include "model.h"
#include "surface.h"
#include "platformgl.h"
#include "enginelimits.h"
#include "../grinliz/glvector.h"

using namespace grinliz;

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
	size_t r = fread( vertex, sizeof(VertexX), nTotalVertices, fp );
	GLASSERT( r == nTotalVertices );
	grinliz::SwapBufferBE32( (U32*)vertex, nTotalVertices*8 );

	GLASSERT( sizeof(VertexX) == sizeof(Vertex) );
	GLASSERT( sizeof(VertexX) == sizeof(U32)*8 );

	#ifndef TARGET_OS_IPHONE
	// Convert to float if NOT the ipod. The ipod uses fixed - everything else is float.
	for( U32 i=0; i<nTotalVertices*8; ++i ) {
		S32 u = *(((S32*)vertex)+i);
		float f = FixedToFloat( u );
		*(((float*)vertex)+i) = f;

	}
	/*
	#ifdef DEBUG
	for( U32 i=0; i<nTotalVertices; ++i ) {
		const Vertex& v = *(((Vertex*)vertex) + i);;
		GLOUTPUT(( "Vertex %d: pos=(%.2f, %.2f, %.2f) normal=(%.1f, %.1f, %.1f) tex=(%.1f, %.1f)\n", 
					i,
					v.pos.x, v.pos.y, v.pos.z,
					v.normal.x, v.normal.y, v.normal.z,
					v.tex.x, v.tex.y ));
	}
	#endif
	 */
	#endif

	fread( index, sizeof(U16), nTotalIndices, fp );
	grinliz::SwapBufferBE16( index, nTotalIndices );

#ifdef DEBUG
	/*
	for( U32 i=0; i<nTotalIndices; i+=3 ) {
		GLOUTPUT(( "%d %d %d\n", index[i+0], index[i+1], index[i+2] ));
	}
	 */
#endif
	
	// Load to VBO
	glGenBuffers( 1, (GLuint*) &res->dataID );
	glGenBuffers( 1, (GLuint*) &res->indexID );
	CHECK_GL_ERROR

	glBindBuffer( GL_ARRAY_BUFFER, res->dataID );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, res->indexID );
	CHECK_GL_ERROR

	// Data (vertex) buffer
	U32 dataSize  = sizeof(VertexX)*nTotalVertices;
	U32 indexSize = sizeof(U16)*nTotalIndices;

	glBufferData( GL_ARRAY_BUFFER, dataSize, vertex, GL_STATIC_DRAW );
	CHECK_GL_ERROR

	// Index buffer
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, index, GL_STATIC_DRAW );

	// unbind
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}


void Model::Draw( bool useTexture )
{
	glPushMatrix();
	glTranslatef( pos.x, pos.y, pos.z );
	glRotatef( rot, 0.f, 1.f, 0.f );

	for( U32 i=0; i<resource->nGroups; ++i ) {
		glBindBuffer( GL_ARRAY_BUFFER, resource->dataID );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, resource->indexID );

		if ( useTexture ) {
			if ( resource->texture[i] ) {
				glBindTexture( GL_TEXTURE_2D, resource->texture[i]->glID );
			}
			else {
				glBindTexture( GL_TEXTURE_2D, 0 );
			}
		}

		#if TARGET_OS_IPHONE		
		glVertexPointer(   3, GL_FIXED, sizeof(Vertex), (const GLvoid*)Vertex::POS_OFFSET);			// last param is offset, not ptr
		glNormalPointer(      GL_FIXED, sizeof(Vertex), (const GLvoid*)Vertex::NORMAL_OFFSET);		
		glTexCoordPointer( 3, GL_FIXED, sizeof(Vertex), (const GLvoid*)Vertex::TEXTURE_OFFSET);  
		#else
		glVertexPointer(   3, GL_FLOAT, sizeof(Vertex), (const GLvoid*)Vertex::POS_OFFSET);			// last param is offset, not ptr
		glNormalPointer(      GL_FLOAT, sizeof(Vertex), (const GLvoid*)Vertex::NORMAL_OFFSET);		
		glTexCoordPointer( 3, GL_FLOAT, sizeof(Vertex), (const GLvoid*)Vertex::TEXTURE_OFFSET);  
		#endif
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
}

