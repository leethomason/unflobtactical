#include "model.h"
#include "surface.h"
#include "platformgl.h"
#include "enginelimits.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

void ModelLoader::Load( FILE* fp, ModelResource* res )
{
	U32 nTotalVertices;
	U32 nTotalIndices;
	
	fread( res->name, 16, 1, fp );

	fread( &res->nGroups, 4, 1, fp );
	fread( &nTotalVertices, 4, 1, fp );
	fread( &nTotalIndices, 4, 1, fp );
	fread( res->bounds, 4, 6, fp );

	res->nGroups = grinliz::SwapBE32( res->nGroups );
	nTotalVertices = grinliz::SwapBE32( nTotalVertices );
	nTotalIndices = grinliz::SwapBE32( nTotalIndices );

	for( int i=0; i<2; i++ ) {
		res->bounds[i].x = grinliz::SwapBE32( res->bounds[i].x );
		res->bounds[i].y = grinliz::SwapBE32( res->bounds[i].y );
		res->bounds[i].z = grinliz::SwapBE32( res->bounds[i].z );
	}

	GLASSERT( nTotalVertices <= EL_MAX_VERTEX_IN_GROUP );
	GLASSERT( nTotalIndices <= EL_MAX_INDEX_IN_GROUP );

	GLOUTPUT(( "Load Model: %s\n", res->name ));
	for( U32 i=0; i<res->nGroups; ++i )
	{
		char textureName[16];
		fread( textureName, 16, 1, fp );

		const Texture* t = 0;
		if ( textureName[0] ) {
			// Remove extension.
			std::string base, name, extension;
			StrSplitFilename( std::string( textureName ), &base, &name, &extension );

			for( int j=0; j<nTextures; ++j ) {
				if ( strcmp( name.c_str(), texture[j].name ) == 0 ) {
					t = &texture[j];
					break;
				}
			}
		}
		res->texture[i] = t;

		fread( &res->nVertex[i], 4, 1, fp );
		fread( &res->nIndex[i], 4, 1, fp );
		res->nVertex[i] = grinliz::SwapBE32( res->nVertex[i] );
		res->nIndex[i] = grinliz::SwapBE32( res->nIndex[i] );

		GLOUTPUT(( "  '%s' vertices=%d tris=%d\n", textureName, res->nVertex[i], res->nIndex[i]/3 ));
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
	#ifdef DEBUG
	
	GLOUTPUT(( "Model: %s\n", res->name ));
	for( U32 i=0; i<nTotalVertices; ++i ) {
		const Vertex& v = *(((Vertex*)vertex) + i);;
		GLOUTPUT(( "Vertex %d: pos=(%.2f, %.2f, %.2f) normal=(%.1f, %.1f, %.1f) tex=(%.1f, %.1f)\n", 
					i,
					v.pos.x, v.pos.y, v.pos.z,
					v.normal.x, v.normal.y, v.normal.z,
					v.tex.x, v.tex.y ));
	}
	
	#endif
	#endif

	fread( index, sizeof(U16), nTotalIndices, fp );
	grinliz::SwapBufferBE16( index, nTotalIndices );

#ifdef DEBUG
	
	for( U32 i=0; i<nTotalIndices; i+=3 ) {
		GLOUTPUT(( "%d %d %d\n", index[i+0], index[i+1], index[i+2] ));
	}
	
#endif
	

	// Load to VBO
	int vOffset = 0;
	int iOffset = 0;
	for( U32 i=0; i<res->nGroups; ++i )
	{
		// Index buffer
		glGenBuffers( 1, (GLuint*) &res->indexID[i] );
		glGenBuffers( 1, (GLuint*) &res->vertexID[i] );
		CHECK_GL_ERROR
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, res->indexID[i] );
		glBindBuffer( GL_ARRAY_BUFFER, res->vertexID[i] );
		CHECK_GL_ERROR
		U32 indexSize = sizeof(U16)*res->nIndex[i];
		U32 dataSize  = sizeof(VertexX)*res->nVertex[i];
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, index+iOffset, GL_STATIC_DRAW );
		glBufferData( GL_ARRAY_BUFFER, dataSize, vertex+vOffset, GL_STATIC_DRAW );
		CHECK_GL_ERROR

		iOffset += res->nIndex[i];
		vOffset += res->nVertex[i];
	}
	// unbind
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}


void Model::Draw( bool useTexture )
{
	glPushMatrix();
	glTranslatef( pos.x, pos.y, pos.z );
	glRotatef( rot, 0.f, 1.f, 0.f );

	for( U32 i=0; i<resource->nGroups; ++i ) 
	{
		glBindBuffer( GL_ARRAY_BUFFER, resource->vertexID[i] );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, resource->indexID[i] );

		if ( useTexture ) {
			if ( resource->texture[i] ) {
				glBindTexture( GL_TEXTURE_2D, resource->texture[i]->glID );
				GLASSERT( glIsEnabled( GL_TEXTURE_2D ) );
			}
			else {
				glBindTexture( GL_TEXTURE_2D, 0 );
			}
		}

		#if TARGET_OS_IPHONE		
		glVertexPointer(   3, GL_FIXED, sizeof(Vertex), (const GLvoid*)Vertex::POS_OFFSET);			// last param is offset, not ptr
		glNormalPointer(      GL_FIXED, sizeof(Vertex), (const GLvoid*)Vertex::NORMAL_OFFSET);		
		glTexCoordPointer( 2, GL_FIXED, sizeof(Vertex), (const GLvoid*)Vertex::TEXTURE_OFFSET);  
		#else
		glVertexPointer(   3, GL_FLOAT, sizeof(Vertex), (const GLvoid*)Vertex::POS_OFFSET);			// last param is offset, not ptr
		glNormalPointer(      GL_FLOAT, sizeof(Vertex), (const GLvoid*)Vertex::NORMAL_OFFSET);		
		glTexCoordPointer( 2, GL_FLOAT, sizeof(Vertex), (const GLvoid*)Vertex::TEXTURE_OFFSET);  
		#endif
		CHECK_GL_ERROR;

		// mode, count, type, indices
		glDrawElements( GL_TRIANGLES, 
						resource->nIndex[i],
						GL_UNSIGNED_SHORT, 
						0 );
		CHECK_GL_ERROR;
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glPopMatrix();
}

