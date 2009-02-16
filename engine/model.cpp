#include "model.h"
#include "surface.h"
#include "platformgl.h"
#include "enginelimits.h"
#include "loosequadtree.h"
#include "renderQueue.h"

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
		res->bounds[i].x.x = grinliz::SwapBE32( res->bounds[i].x.x );
		res->bounds[i].y.x = grinliz::SwapBE32( res->bounds[i].y.x );
		res->bounds[i].z.x = grinliz::SwapBE32( res->bounds[i].z.x );
	}

	// Compute the bounding sphere. The model is always rotated around the
	// Y axis, so be sure to put the sphere there, else it won't be invariant
	// to rotation. Then check the 8 corners for the furthest.
	res->boundSphere.origin.x = 0;
	res->boundSphere.origin.y = ( res->bounds[0].y + res->bounds[1].y ) / 2;
	res->boundSphere.origin.z = 0;

	Fixed longestR2( 0 );

	for( int k=0; k<2; ++k ) {
		for( int j=0; j<2; ++j ) {
			for( int i=0; i<2; ++i ) {
				Fixed dx = res->bounds[i].x - res->boundSphere.origin.x;
				Fixed dy = res->bounds[j].y - res->boundSphere.origin.y;
				Fixed dz = res->bounds[k].z - res->boundSphere.origin.z;
				Fixed d2 = dx*dx + dy*dy + dz*dz;

				if ( d2 > longestR2 ) {
					longestR2 = d2;
				}
			}
		}
	}
	res->boundSphere.radius = longestR2.Sqrt();

	// And the bounding circle.
	longestR2 = 0;

	for( int k=0; k<2; ++k ) {
		for( int i=0; i<2; ++i ) {
			Fixed dx = res->bounds[i].x - res->boundSphere.origin.x;
			Fixed dz = res->bounds[k].z - res->boundSphere.origin.z;
			Fixed d2 = dx*dx + dz*dz;

			if ( d2 > longestR2 ) {
				longestR2 = d2;
			}
		}
	}
	res->boundRadius2D = longestR2.Sqrt();

	// compute the hit testing AABB
	GLASSERT( res->bounds[1].x >= res->bounds[0].x );
	GLASSERT( res->bounds[1].z >= res->bounds[0].z );

	Fixed ave = (( res->bounds[1].x - res->bounds[0].x ) + ( res->bounds[1].z - res->bounds[0].z )) >> 1;
	res->hitBounds.min.Set( res->boundSphere.origin.x - ave/2, res->bounds[0].y, res->boundSphere.origin.z - ave/2 );
	res->hitBounds.max.Set( res->boundSphere.origin.x + ave/2, res->bounds[1].y, res->boundSphere.origin.z + ave/2 );

	//GLASSERT( nTotalVertices <= EL_MAX_VERTEX_IN_GROUP );
	//GLASSERT( nTotalIndices <= EL_MAX_INDEX_IN_GROUP );

	GLOUTPUT(( "Load Model: %s\n", res->name ));
	GLOUTPUT(( "  sphere (%.2f,%.2f,%.2f) rad=%.2f  hit(%.2f,%.2f,%.2f)-(%.2f,%.2f,%.2f)\n",
				(float)res->boundSphere.origin.x,
				(float)res->boundSphere.origin.y,
				(float)res->boundSphere.origin.z,
				(float)res->boundSphere.radius,
				(float)res->hitBounds.min.x,
				(float)res->hitBounds.min.y,
				(float)res->hitBounds.min.z,
				(float)res->hitBounds.max.x,
				(float)res->hitBounds.max.y,
				(float)res->hitBounds.max.z ));

	for( U32 i=0; i<res->nGroups; ++i )
	{
		char textureNameBuf[16];
		fread( textureNameBuf, 16, 1, fp );

		const Texture* t = 0;
		const char* textureName = textureNameBuf;
		if ( !textureName[0] ) {
			textureName = "white";
		}
		
		// Remove extension.
		std::string base, name, extension;
		StrSplitFilename( std::string( textureName ), &base, &name, &extension );

		for( int j=0; j<nTextures; ++j ) {
			if ( strcmp( name.c_str(), texture[j].name ) == 0 ) {
				t = &texture[j];
				break;
			}
		}
		GLASSERT( t );                       
		res->atom[i].texture = t;

		fread( &res->atom[i].nVertex, 4, 1, fp );
		fread( &res->atom[i].nIndex, 4, 1, fp );
		res->atom[i].nVertex = grinliz::SwapBE32( res->atom[i].nVertex );
		res->atom[i].nIndex = grinliz::SwapBE32( res->atom[i].nIndex );

		GLOUTPUT(( "  '%s' vertices=%d tris=%d\n", textureName, res->atom[i].nVertex, res->atom[i].nIndex/3 ));
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
		float f = Fixed::FixedToFloat( u );
		*(((float*)vertex)+i) = f;

	}
	/*
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
	int vOffset = 0;
	int iOffset = 0;
	for( U32 i=0; i<res->nGroups; ++i )
	{
		// Index buffer
		glGenBuffers( 1, (GLuint*) &res->atom[i].indexID );
		glGenBuffers( 1, (GLuint*) &res->atom[i].vertexID );
		CHECK_GL_ERROR
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, res->atom[i].indexID );
		glBindBuffer( GL_ARRAY_BUFFER, res->atom[i].vertexID );
		CHECK_GL_ERROR
		U32 indexSize = sizeof(U16)*res->atom[i].nIndex;
		U32 dataSize  = sizeof(VertexX)*res->atom[i].nVertex;
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, index+iOffset, GL_STATIC_DRAW );
		glBufferData( GL_ARRAY_BUFFER, dataSize, vertex+vOffset, GL_STATIC_DRAW );
		CHECK_GL_ERROR

		iOffset += res->atom[i].nIndex;
		vOffset += res->atom[i].nVertex;
	}
	// unbind
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}


void Model::Init( ModelResource* resource, SpaceTree* tree )
{
	this->resource = resource; 
	this->tree = tree;
	pos.Set( Fixed(0), Fixed(0), Fixed(0) ); 
	rot = Fixed(0);
	textureOffsetX = Fixed( 0 );

	if ( tree ) {
		tree->Update( this );
	}
	isDraggable = false;
	hiddenFromTree = false;
}


void Model::SetPos( const Vector3X& pos )
{ 
	this->pos = pos;	
	tree->Update( this ); 
}


void Model::SetSkin( int armor, int skin, int hair )
{
	textureOffsetX = Fixed( 0 );

	armor = Clamp( armor, 0, 3 );
	skin = Clamp( skin, 0, 3 );
	hair = Clamp( hair, 0, 3 );

	textureOffsetX += Fixed( armor ) / Fixed( 4 );
	textureOffsetX += Fixed( skin ) / Fixed( 16 );
	textureOffsetX += Fixed( hair ) / Fixed( 64 );
}


void Model::CalcBoundSphere( SphereX* spherex )
{
	*spherex = resource->boundSphere;
	spherex->origin += pos;
}


void Model::CalcBoundCircle( CircleX* circlex )
{
	circlex->origin.x = pos.x;
	circlex->origin.y = pos.z;
	circlex->radius = resource->boundRadius2D;
}


void Model::CalcHitAABB( Rectangle3X* aabb )
{
	// This is already an approximation - ignore rotation.
	aabb->min = pos + resource->hitBounds.min;
	aabb->max = pos + resource->hitBounds.max;
}


void Model::Queue( RenderQueue* queue, bool useTexture )
{
	for( U32 i=0; i<resource->nGroups; ++i ) {

		int flags = 0;

		// FIXME: If alpha test is in use, we need the texture for the shadow phase.
		if ( resource->atom[i].texture->alphaTest & RenderQueue::ALPHA_TEST ) {
			flags |= RenderQueue::ALPHA_TEST;
			//useTexture = true;
		}

		queue->Add( flags, 
					(useTexture) ? resource->atom[i].texture->glID : 0,
					this, 
					&resource->atom[i] );
	}
}


void Model::Draw( bool useTexture )
{
	glPushMatrix();
	#if TARGET_OS_IPHONE		
	glTranslatex( pos.x.x, pos.y.x, pos.z.x );
	glRotatex( rot.x, 0.f, 1.f, 0.f );
	#else
	{
		Vector3F posf;
		float rotf = rot;
		ConvertVector3( pos, &posf );

		glTranslatef( pos.x, pos.y, pos.z );
		glRotatef( rotf, 0.f, 1.f, 0.f );
	}
	#endif

	if ( textureOffsetX > 0 ) {
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		#if TARGET_OS_IPHONE
		glTranslatex( textureOffsetX.x, 0, 0 );
		#else
		glTranslatef( (float) textureOffsetX, 0.0f, 0.0f );
		#endif
	}

	for( U32 i=0; i<resource->nGroups; ++i ) 
	{
		glBindBuffer( GL_ARRAY_BUFFER, resource->atom[i].vertexID );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, resource->atom[i].indexID );

		if ( useTexture ) {
			GLASSERT( resource->atom[i].texture );
			glBindTexture( GL_TEXTURE_2D, resource->atom[i].texture->glID );
			GLASSERT( glIsEnabled( GL_TEXTURE_2D ) );
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
						resource->atom[i].nIndex,
						GL_UNSIGNED_SHORT, 
						0 );
		CHECK_GL_ERROR;
	}

	if ( textureOffsetX > 0 ) {
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glPopMatrix();
}


void ModelAtom::Bind() const
{
	glBindBuffer( GL_ARRAY_BUFFER, vertexID );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexID );

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
}


void ModelAtom::Draw() const
{
	// mode, count, type, indices
	glDrawElements( GL_TRIANGLES, 
					nIndex,
					GL_UNSIGNED_SHORT, 
					0 );
	CHECK_GL_ERROR;
}


void Model::PushMatrix() const
{
	glPushMatrix();
	#if TARGET_OS_IPHONE		
	glTranslatex( pos.x.x, pos.y.x, pos.z.x );
	if ( rot != 0 ) {
		glRotatex( rot.x, 0.f, 1.f, 0.f );
	}
	#else
	{
		Vector3F posf;
		float rotf = rot;
		ConvertVector3( pos, &posf );

		glTranslatef( pos.x, pos.y, pos.z );
		if ( rot != 0 ) {
			glRotatef( rotf, 0.f, 1.f, 0.f );
		}
	}
	#endif

	if ( textureOffsetX > 0 ) {
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		#if TARGET_OS_IPHONE
		glTranslatex( textureOffsetX.x, 0, 0 );
		#else
		glTranslatef( (float) textureOffsetX, 0.0f, 0.0f );
		#endif
	}
}


void Model::PopMatrix() const
{
	if ( textureOffsetX > 0 ) {
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
}

