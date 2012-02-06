#include "gpustatemanager.h"
#include "platformgl.h"
#include "texture.h"
#include "enginelimits.h"
#include "../gamui/gamui.h"	// for auto setting up gamui stream
#include "../grinliz/glperformance.h"

#if XENOENGINE_OPENGL == 2
#include "shadermanager.h"
#endif

using namespace grinliz;

int GPUShader::matrixDepth[3];

/*static*/ GPUVertexBuffer GPUVertexBuffer::Create( const Vertex* vertex, int nVertex )
{
	GPUVertexBuffer buffer;

	if ( GPUShader::SupportsVBOs() ) {
		U32 dataSize  = sizeof(Vertex)*nVertex;
		glGenBuffersX( 1, (GLuint*) &buffer.id );
		glBindBufferX( GL_ARRAY_BUFFER, buffer.id );
		// if vertex is null this will just allocate
		glBufferDataX( GL_ARRAY_BUFFER, dataSize, vertex, vertex ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW );
		glBindBufferX( GL_ARRAY_BUFFER, 0 );
		CHECK_GL_ERROR;
	}
	return buffer;
}


void GPUVertexBuffer::Upload( const Vertex* data, int count, int start )
{
	GLASSERT( GPUShader::SupportsVBOs() );
	glBindBufferX( GL_ARRAY_BUFFER, id );
	// target, offset, size, data
	glBufferSubDataX( GL_ARRAY_BUFFER, start*sizeof(Vertex), count*sizeof(Vertex), data );
	CHECK_GL_ERROR;
	glBindBufferX( GL_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;
}


void GPUVertexBuffer::Destroy() 
{
	if ( id ) {
		glDeleteBuffersX( 1, (GLuint*) &id );
		id = 0;
	}
}


/*static*/ GPUIndexBuffer GPUIndexBuffer::Create( const U16* index, int nIndex )
{
	GPUIndexBuffer buffer;

	if ( GPUShader::SupportsVBOs() ) {
		U32 dataSize  = sizeof(U16)*nIndex;
		glGenBuffersX( 1, (GLuint*) &buffer.id );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, buffer.id );
		glBufferDataX( GL_ELEMENT_ARRAY_BUFFER, dataSize, index, GL_STATIC_DRAW );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
		CHECK_GL_ERROR;
	}
	return buffer;
}

void GPUIndexBuffer::Upload( const uint16_t* data, int count, int start )
{
	GLASSERT( GPUShader::SupportsVBOs() );
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, id );
	// target, offset, size, data
	glBufferSubDataX( GL_ELEMENT_ARRAY_BUFFER, start*sizeof(uint16_t), count*sizeof(uint16_t), data );
	CHECK_GL_ERROR;
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;
}



void GPUIndexBuffer::Destroy() 
{
	if ( id ) {
		glDeleteBuffersX( 1, (GLuint*) &id );
		id = 0;
	}
}


MatrixStack::MatrixStack() : index( 0 )
{
	stack[0].SetIdentity();
}


MatrixStack::~MatrixStack()
{
	GLASSERT( index == 0 );
}


void MatrixStack::Push()
{
	GLASSERT( index < MAX_DEPTH-1 );
	if ( index < MAX_DEPTH-1 ) {
		stack[index+1] = stack[index];
		++index;
	}
}


void MatrixStack::Pop()
{
	GLASSERT( index > 0 );
	if ( index > 0 ) {
		index--;
	}
}


void MatrixStack::Multiply( const grinliz::Matrix4& m )
{
	GLASSERT( index > 0 );
	stack[index] = stack[index] * m;
}


/*static*/ GPUShader GPUShader::current;
/*static*/ int GPUShader::trianglesDrawn = 0;
/*static*/ int GPUShader::drawCalls = 0;
/*static*/ uint32_t GPUShader::uid = 0;
/*static*/ GPUShader::MatrixType GPUShader::matrixMode = MODELVIEW_MATRIX;
/*static*/ MatrixStack GPUShader::textureStack[2];
#if XENOENGINE_OPENGL == 2
/*static*/ MatrixStack GPUShader::mvStack;
/*static*/ MatrixStack GPUShader::projStack;
#endif
/*static*/ bool GPUShader::textureXFormInUse[2] = { false, false };
/*static*/ int GPUShader::vboSupport = 0;

/*static*/ bool GPUShader::SupportsVBOs()
{
	if ( vboSupport == 0 ) {
		const char* extensions = (const char*)glGetString( GL_EXTENSIONS );
		const char* vbo = strstr( extensions, "ARB_vertex_buffer_object" );
		vboSupport = (vbo) ? 1 : -1;
	}
	return (vboSupport > 0);
}


/*static */ void GPUShader::ResetState()
{
	GPUShader state;
	current = state;

	// Texture unit 1
	glActiveTexture( GL_TEXTURE1 );
	glClientActiveTexture( GL_TEXTURE1 );
	glDisable( GL_TEXTURE_2D );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	// Texture unit 0
	glActiveTexture( GL_TEXTURE0 );
	glClientActiveTexture( GL_TEXTURE0 );
	glDisable( GL_TEXTURE_2D );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	// Client state
	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	// Blend/Alpha
	glDisable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_ALPHA_TEST );
	glAlphaFunc( GL_GREATER, 0.5f );

	// Ligting
	glDisable( GL_LIGHTING );

	// Depth
	glDepthMask( GL_TRUE );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

	// Matrix
	glMatrixMode( GL_MODELVIEW );
	matrixMode = MODELVIEW_MATRIX;

	// Color
	glColor4f( 1, 1, 1, 1 );

	// General config:
	glCullFace( GL_BACK );
	glEnable( GL_CULL_FACE );

	CHECK_GL_ERROR;
}


void GPUShader::SetTextureXForm( int unit )
{
	if ( textureStack[unit].NumMatrix()>1 || textureXFormInUse[unit] ) 
	{
#if XENOENGINE_OPENGL == 1
		glMatrixMode( GL_TEXTURE );
		glLoadMatrixf( textureStack[unit].Top().x );
		glMatrixMode( matrixMode == MODELVIEW_MATRIX ? GL_MODELVIEW : GL_PROJECTION );
#endif
		textureXFormInUse[unit] = textureStack[unit].NumMatrix()>1;
	}
	CHECK_GL_ERROR;
}


//static 
void GPUShader::SetState( const GPUShader& ns )
{
	GRINLIZ_PERFTRACK
	CHECK_GL_ERROR;
	GLASSERT( ns.stream.stride > 0 );

#if XENOENGINE_OPENGL == 1
	// Texture1
	if ( ns.stream.HasTexture1() || current.stream.HasTexture1() ) {

		glActiveTexture( GL_TEXTURE1 );
		glClientActiveTexture( GL_TEXTURE1 );

		if ( ns.texture1 && !current.texture1 ) {
			GLASSERT( ns.texture0 );
			GLASSERT( ns.stream.nTexture1 );

			glEnable( GL_TEXTURE_2D );
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );

			// Craziness: code - by chance - is completely ES1.1 compliant (except for 
			// auto mip-maps? not sure) except for this call, which just needed
			// to be switched to [f] form.
			//glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
			glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		}
		else if ( !ns.stream.HasTexture1() && current.stream.HasTexture1() ) {
			glDisable( GL_TEXTURE_2D );
			glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		
		if (  ns.stream.HasTexture1() ) {
			GLASSERT( ns.texture1 );
			glTexCoordPointer(	ns.stream.nTexture1, 
								GL_FLOAT, 
								ns.stream.stride, 
								PTR( ns.streamPtr, ns.stream.texture1Offset ) );

			if ( ns.texture1 != current.texture1 ) {
				glBindTexture( GL_TEXTURE_2D, ns.texture1->GLID() );
			}
			SetTextureXForm( 1 );
		}
		glActiveTexture( GL_TEXTURE0 );
		glClientActiveTexture( GL_TEXTURE0 );
	}

	CHECK_GL_ERROR;

	// Texture0
	if ( ns.stream.HasTexture0() && !current.stream.HasTexture0() ) {
		GLASSERT( ns.texture0 );
		GLASSERT( ns.stream.nTexture0 );

		glEnable( GL_TEXTURE_2D );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	}
	else if ( !ns.stream.HasTexture0() && current.stream.HasTexture0() ) {
		glDisable( GL_TEXTURE_2D );
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}
	
	if (  ns.stream.HasTexture0() ) {
		GLASSERT( ns.texture0 );
		glTexCoordPointer(	ns.stream.nTexture0, 
							GL_FLOAT, 
							ns.stream.stride, 
							PTR( ns.streamPtr, ns.stream.texture0Offset ) );	

		if ( ns.texture0 != current.texture0 )
		{
			glBindTexture( GL_TEXTURE_2D, ns.texture0->GLID() );
		}
		SetTextureXForm( 0 );
	}
	CHECK_GL_ERROR;

	// Vertex
	if ( ns.stream.HasPos() && !current.stream.HasPos() ) {
		glEnableClientState( GL_VERTEX_ARRAY );
	}
	else if ( !ns.stream.HasPos() && current.stream.HasPos() ) {
		glDisableClientState( GL_VERTEX_ARRAY );
	}
	if ( ns.stream.HasPos() ) {
		glVertexPointer(	ns.stream.nPos, 
							GL_FLOAT, 
							ns.stream.stride, 
							PTR( ns.streamPtr, ns.stream.posOffset ) );
	}

	// Normal
	if ( ns.stream.HasNormal() && !current.stream.HasNormal() ) {
		glEnableClientState( GL_NORMAL_ARRAY );
	}
	else if ( !ns.stream.HasNormal() && current.stream.HasNormal() ) {
		glDisableClientState( GL_NORMAL_ARRAY );
	}
	if ( ns.stream.HasNormal() ) {
		GLASSERT( ns.stream.nNormal == 3 );
		glNormalPointer(	GL_FLOAT, 
							ns.stream.stride, 
							PTR( ns.streamPtr, ns.stream.normalOffset ) );
	}

	// Color
	if ( ns.stream.HasColor() && !current.stream.HasColor() ) {
		glEnableClientState( GL_COLOR_ARRAY );
	}
	else if ( !ns.stream.HasColor() && current.stream.HasColor() ) {
		glDisableClientState( GL_COLOR_ARRAY );
	}
	if ( ns.stream.HasColor() ) {
		glColorPointer(	ns.stream.nColor, 
						GL_FLOAT, 
						ns.stream.stride,
						PTR( ns.streamPtr, ns.stream.colorOffset ) );
	}
	CHECK_GL_ERROR;

	// Lighting
	if ( ns.lighting && !current.lighting ) {
		glEnable( GL_LIGHTING );
		glEnable ( GL_COLOR_MATERIAL );
		// The call below isn't supported on all the mobile chipsets:
		//glColorMaterial ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE ) 
		//GLOUTPUT(( "Lighting on.\n" ));
	}
	else if ( !ns.lighting && current.lighting ) {
		glDisable( GL_LIGHTING );
		//GLOUTPUT(( "Lighting off.\n" ));
	}

	// color
	if ( ns.color != current.color ) {
		glColor4f( ns.color.r, ns.color.g, ns.color.b, ns.color.a );
	}

#elif XENOENGINE_OPENGL == 2
	ShaderManager* shadman = ShaderManager::Instance();

	int flags = 0;
	flags |= ( ns.HasTexture0() ) ? ShaderManager::TEXTURE0 : 0;
	flags |= (textureStack[0].NumMatrix()>1 || textureXFormInUse[0]) ? ShaderManager::TEXTURE0_TRANSFORM : 0;
	flags |= (ns.HasTexture0() && (ns.texture0->Format() == Texture::ALPHA )) ? ShaderManager::TEXTURE0_ALPHA_ONLY : 0;
	flags |= (ns.stream.nTexture0 == 3 ) ? ShaderManager::TEXTURE0_3COMP : 0;

	flags |= ( ns.HasTexture1() ) ? ShaderManager::TEXTURE1 : 0;
	flags |= (textureStack[1].NumMatrix()>1 || textureXFormInUse[1]) ? ShaderManager::TEXTURE1_TRANSFORM : 0;
	flags |= (ns.HasTexture1() && (ns.texture1->Format() == Texture::ALPHA )) ? ShaderManager::TEXTURE1_ALPHA_ONLY : 0;
	flags |= (ns.stream.nTexture1 == 3 ) ? ShaderManager::TEXTURE1_3COMP : 0;

	flags |= ns.stream.HasColor() ? ShaderManager::COLORS : 0;
	flags |= ( ns.color.r != 1.f || ns.color.g != 1.f || ns.color.b != 1.f || ns.color.a != 1.f ) ? ShaderManager::COLOR_MULTIPLIER : 0;
	flags |= ns.HasLighting( 0, 0, 0 ) ? ShaderManager::LIGHTING_DIFFUSE : 0;

	shadman->ActivateShader( flags );
	shadman->ClearStream();
	Matrix4 mvp;
	const Matrix4& mv = ns.ConcatedMatrix( GPUShader::MODELVIEW_MATRIX );
	MultMatrix4( ns.ConcatedMatrix( GPUShader::PROJECTION_MATRIX ), mv, &mvp );

	// FIXME: the normal matrix can be used because the game doesn't support scaling.
	shadman->SetTransforms( mvp, mv );

	// Texture1
	glActiveTexture( GL_TEXTURE1 );

	if (  ns.stream.HasTexture1() ) {
		glBindTexture( GL_TEXTURE_2D, ns.texture1->GLID() );
		shadman->SetTexture( 1, ns.texture1 );
		shadman->SetStreamData( ShaderManager::A_TEXTURE1, ns.stream.nTexture1, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.texture1Offset ) );
		if ( flags & ShaderManager::TEXTURE1_TRANSFORM ) {
			shadman->SetTextureTransform( 1, textureStack[1].Top() );
		}
	}
	CHECK_GL_ERROR;

	// Texture0
	glActiveTexture( GL_TEXTURE0 );

	if (  ns.stream.HasTexture0() ) {
		glBindTexture( GL_TEXTURE_2D, ns.texture0->GLID() );
		shadman->SetTexture( 0, ns.texture0 );
		shadman->SetStreamData( ShaderManager::A_TEXTURE0, ns.stream.nTexture0, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.texture0Offset ) );
		if ( flags & ShaderManager::TEXTURE0_TRANSFORM ) {
			shadman->SetTextureTransform( 0, textureStack[0].Top() );
		}
	}
	CHECK_GL_ERROR;

	// vertex
	if ( ns.stream.HasPos() ) {
		shadman->SetStreamData( ShaderManager::A_POS, ns.stream.nPos, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.posOffset ) );	 
	}

	// color
	if ( ns.stream.HasColor() ) {
		GLASSERT( ns.stream.nColor == 4 );
		shadman->SetStreamData( ShaderManager::A_COLOR, 4, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.colorOffset ) );	 
	}

	// lighting 
	if ( flags & ShaderManager::LIGHTING_DIFFUSE ) {
		Vector4F dirWC, d, a;
		ns.HasLighting( &dirWC, &a, &d ); 

		Vector4F dirEye = mv * dirWC;
		shadman->SetDiffuse( dirEye, a, d );	
		shadman->SetStreamData( ShaderManager::A_NORMAL, 3, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.normalOffset ) );	 
	}

	// color multiplier
	if ( flags & ShaderManager::COLOR_MULTIPLIER ) {
		shadman->SetColorMultiplier( ns.color );
	}
#endif

/*
	// Alpha Test
	if ( ns.alphaTest && !current.alphaTest ) {
		glEnable( GL_ALPHA_TEST );
	}
	else if ( !ns.alphaTest && current.alphaTest ) {
		glDisable( GL_ALPHA_TEST );
	}
*/

	// Blend
	if ( ns.blend && !current.blend ) {
		glEnable( GL_BLEND );
	}
	else if ( !ns.blend && current.blend ) {
		glDisable( GL_BLEND );
	}

	// Depth Write
	if ( ns.depthWrite && !current.depthWrite ) {
		glDepthMask( GL_TRUE );
	}
	else if ( !ns.depthWrite && current.depthWrite ) {
		glDepthMask( GL_FALSE );
	}

	// Depth test
	if ( ns.depthTest && !current.depthTest ) {
		glEnable( GL_DEPTH_TEST );
	}
	else if ( !ns.depthTest && current.depthTest ) {
		glDisable( GL_DEPTH_TEST );
	}

	current = ns;
	CHECK_GL_ERROR;

#if (XENOENGINE_OPENGL == 1 ) && defined( USING_GL ) && defined( DEBUG )
#	define ASSERT_SAME( x, y ) if ( x ) { GLASSERT( y ); } else { GLASSERT( !(y) ); }

	void* ptr = 0;

	glActiveTexture( GL_TEXTURE1 );
	glClientActiveTexture( GL_TEXTURE1 );
	ASSERT_SAME( current.texture1, glIsEnabled( GL_TEXTURE_2D ) );
	ASSERT_SAME( current.texture1, current.stream.HasTexture1() );
	ASSERT_SAME( current.texture1, glIsEnabled( GL_TEXTURE_COORD_ARRAY ) );
	if ( current.texture1 ) {
		glGetPointerv( GL_TEXTURE_COORD_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == PTR( current.streamPtr, current.stream.texture1Offset ) );
	}

	glActiveTexture( GL_TEXTURE0 );
	glClientActiveTexture( GL_TEXTURE0 );
	ASSERT_SAME( current.texture0, glIsEnabled( GL_TEXTURE_2D ) );
	ASSERT_SAME( current.texture0, current.stream.HasTexture0() );
	ASSERT_SAME( current.texture0, glIsEnabled( GL_TEXTURE_COORD_ARRAY ) );
	if ( current.texture0 ) {
		glGetPointerv( GL_TEXTURE_COORD_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == PTR( current.streamPtr, current.stream.texture0Offset) );
	}

	ASSERT_SAME( current.stream.HasPos(), glIsEnabled( GL_VERTEX_ARRAY ) );
	if ( current.stream.HasPos() ) {
		glGetPointerv( GL_VERTEX_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == PTR( current.streamPtr, current.stream.posOffset) );
		GLASSERT( current.stream.posOffset + 4*current.stream.nPos <= current.stream.stride );
	}
	ASSERT_SAME( current.stream.HasNormal(), glIsEnabled( GL_NORMAL_ARRAY ) );
	if ( current.stream.HasNormal() ) {
		glGetPointerv( GL_NORMAL_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == PTR( current.streamPtr, current.stream.normalOffset) );
		GLASSERT( current.stream.normalOffset + 4*current.stream.nNormal <= current.stream.stride );
	}
	ASSERT_SAME( current.stream.HasColor(), glIsEnabled( GL_COLOR_ARRAY ) );
	if ( current.stream.HasColor() ) {
		glGetPointerv( GL_COLOR_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == PTR( current.streamPtr, current.stream.colorOffset) );
		GLASSERT( current.stream.colorOffset + 4*current.stream.nColor <= current.stream.stride );
	}

	GLboolean blendIsEnabled = glIsEnabled( GL_BLEND );
	GLboolean alphaTestIsEnabled = glIsEnabled( GL_ALPHA_TEST );
	GLboolean lightingIsEnabled = glIsEnabled( GL_LIGHTING );
	ASSERT_SAME( current.blend, blendIsEnabled );
	ASSERT_SAME( current.alphaTest, alphaTestIsEnabled );
	ASSERT_SAME( current.lighting, lightingIsEnabled );

	GLboolean param;
	glGetBooleanv( GL_DEPTH_WRITEMASK, &param );

	ASSERT_SAME( current.depthWrite, param );
	ASSERT_SAME( current.depthTest, glIsEnabled( GL_DEPTH_TEST ) );

	{
		int testMode = 0;
		glGetIntegerv( GL_MATRIX_MODE, &testMode );
		switch( current.matrixMode ) {
//		case TEXTURE_MATRIX:	GLASSERT( testMode == GL_TEXTURE );	break;
		case MODELVIEW_MATRIX:	GLASSERT( testMode == GL_MODELVIEW );	break;
		case PROJECTION_MATRIX:	GLASSERT( testMode == GL_PROJECTION );	break;
		default: GLASSERT( 0 ); break;
		}
	}

#	undef ASSERT_SAME
#endif

	CHECK_GL_ERROR;
}


void GPUShader::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

#if 0
/*static*/void GPUShader::SetMatrixMode( MatrixType m )
{
	matrixMode = m;
}
#else

/*static*/ void GPUShader::SetViewport( int w, int h )
{
	glViewport( 0, 0, w, h );
}


/*static*/ void GPUShader::SetOrthoTransform( int screenWidth, int screenHeight, int rotation )
{
	Matrix4 r, t;
	r.SetZRotation( (float)rotation );
	
	// the tricky bit. After rotating the ortho display, move it back on screen.
	switch (rotation) {
		case 0:
			break;
		case 90:
			t.SetTranslation( 0, (float)(-screenWidth), 0 );	
			break;
			
		default:
			GLASSERT( 0 );	// work out...
			break;
	}
	Matrix4 view2D = r*t;

#if XENOENGINE_OPENGL == 1
	SwitchMatrixMode( PROJECTION_MATRIX );
	glLoadIdentity();				// projection
	glOrthofX( 0, screenWidth, screenHeight, 0, -100, 100 );	// go with helping the driver.
#else
	Matrix4 ortho;
	ortho.SetOrtho( 0, (float)screenWidth, (float)screenHeight, 0, -100.f, 100.f );
	SetMatrix( PROJECTION_MATRIX, ortho );
#endif

	SetMatrix( MODELVIEW_MATRIX, view2D );
	CHECK_GL_ERROR;
}


/*static*/ void GPUShader::SetPerspectiveTransform( float left, float right, 
													 float bottom, float top, 
													 float near, float far,
													 int rotation)
{
	// Give the driver hints:
#if XENOENGINE_OPENGL == 1
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustumfX( left, right, bottom, top, near, far );
	glRotatef( (float)(-rotation), 0, 0, 1 );
	glMatrixMode(GL_MODELVIEW);	
#else
	GLASSERT( rotation == 0 );
	Matrix4 perspective;
	perspective.SetFrustum( left, right, bottom, top, near, far );
	SetMatrix( PROJECTION_MATRIX, perspective );
#endif
	CHECK_GL_ERROR;
}


/*static*/ void GPUShader::SetCameraTransform( const grinliz::Matrix4& camera )
{
	glMatrixMode(GL_MODELVIEW);
	// In normalized coordinates.
#if XENOENGINE_OPENGL == 1
	glLoadMatrixf( camera.x );
#elif XENOENGINE_OPENGL == 2
	GLASSERT( mvStack.NumMatrix() == 1 );
	mvStack.Set( camera );
#endif
	CHECK_GL_ERROR;
}


/*static*/ void GPUShader::SetScissor( int x, int y, int w, int h )
{
	if ( w > 0 && h > 0 ) {
		glEnable( GL_SCISSOR_TEST );
		glScissor( x, y, w, h );
	}
	else {
		glDisable( GL_SCISSOR_TEST );
	}
}
#endif

GPUShader::~GPUShader()
{
	GLASSERT( matrixDepth[0] == 0 );
	GLASSERT( matrixDepth[1] == 0 );
	GLASSERT( matrixDepth[2] == 0 );
}


void GPUShader::Draw()
{
	GRINLIZ_PERFTRACK
	if ( nIndex == 0 )
		return;
	GLASSERT( nIndex % 3 == 0 );

	trianglesDrawn += nIndex / 3;
	++drawCalls;

	if ( indexPtr ) {
	
#ifdef EL_USE_VBO
		if ( vertexBuffer ) {
			glBindBufferX( GL_ARRAY_BUFFER, vertexBuffer );
		}
#endif
		SetState( *this );

		GLRELASSERT( !indexBuffer );
		glDrawElements( GL_TRIANGLES, nIndex, GL_UNSIGNED_SHORT, indexPtr );

#ifdef EL_USE_VBO
		if ( vertexBuffer ) {
			glBindBufferX( GL_ARRAY_BUFFER, 0 );
		}
#endif
	}
	else {
#ifdef EL_USE_VBO
		GLRELASSERT( vertexBuffer );
		GLRELASSERT( indexBuffer );
		GLRELASSERT( stream.stride == sizeof(Vertex) );	// Just a current limitation that only Vertex structs go in a VBO. Could be fixed.

		// This took a long time to figure. OpenGL is a state machine, except, apparently, when it isn't.
		// The VBOs impact how the SetxxxPointers work. If they aren't set, then the wrong thing gets
		// bound. What a PITA. And ugly design problem with OpenGL.
		//
		glBindBufferX( GL_ARRAY_BUFFER, vertexBuffer );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
		SetState( *this );

#if defined( _MSC_VER ) && (XENOENGINE_OPENGL == 1)
		GLASSERT( glIsEnabled( GL_VERTEX_ARRAY ) );
		GLASSERT( !glIsEnabled( GL_COLOR_ARRAY ) );
		GLASSERT( !glIsEnabled( GL_INDEX_ARRAY ) );
		GLASSERT( glIsEnabled( GL_TEXTURE_COORD_ARRAY ) );
#endif
		glDrawElements( GL_TRIANGLES, nIndex, GL_UNSIGNED_SHORT, 0 );

		glBindBufferX( GL_ARRAY_BUFFER, 0 );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
#else	
		GLASSERT( 0 );
#endif
	}
	CHECK_GL_ERROR;
}


void GPUShader::Debug_DrawQuad( const grinliz::Vector3F p0, const grinliz::Vector3F p1 )
{
#ifdef DEBUG
	grinliz::Vector3F pos[4] = { 
		{ p0.x, p0.y, p0.z },
		{ p1.x, p0.y, p0.z },
		{ p1.x, p1.y, p1.z },
		{ p0.x, p1.y, p1.z },
	};
	static const U16 index[6] = { 0, 2, 1, 0, 3, 2 };
	GPUStream stream;
	stream.stride = sizeof(grinliz::Vector3F);
	stream.nPos = 3;
	stream.posOffset = 0;

	SetStream( stream, pos, 6, index );
	Draw();
#endif
}



void GPUShader::SwitchMatrixMode( MatrixType type )
{
#if defined( USING_GL ) && defined( DEBUG ) && (XENOENGINE_OPENGL == 1 )
	{
		int testMode = 0;
		glGetIntegerv( GL_MATRIX_MODE, &testMode );
		switch( matrixMode ) {
		//case TEXTURE_MATRIX:	GLASSERT( testMode == GL_TEXTURE );	break;
		case MODELVIEW_MATRIX:	GLASSERT( testMode == GL_MODELVIEW );	break;
		case PROJECTION_MATRIX:	GLASSERT( testMode == GL_PROJECTION );	break;
		default: GLASSERT( 0 ); break;
		}
	}
#endif

#if XENOENGINE_OPENGL == 1
	if ( matrixMode != type ) {
		switch ( type ) {
		case MODELVIEW_MATRIX:
			glMatrixMode( GL_MODELVIEW );
			break;
		case PROJECTION_MATRIX:
			glMatrixMode( GL_PROJECTION );
			break;
		default:
			GLASSERT( 0 );
			return;
		}
	}
#endif
	matrixMode = type;
	CHECK_GL_ERROR;
}


void GPUShader::PushMatrix( MatrixType type )
{
	SwitchMatrixMode( type );

	switch( type ) {
#if XENOENGINE_OPENGL == 1
	case MODELVIEW_MATRIX:
	case PROJECTION_MATRIX:
		glPushMatrix();
		break;
#elif XENOENGINE_OPENGL == 2
	case MODELVIEW_MATRIX:
		mvStack.Push();
		break;
	case PROJECTION_MATRIX:
		projStack.Push();
		break;
#endif
	default:
		GLASSERT( 0 );
		return;
	}
	matrixDepth[(int)type] += 1;

#ifdef DEBUG
	GLenum error = glGetError();
	if ( error  != GL_NO_ERROR ) 
	{	
		GLOUTPUT(( "GL Error: %x matrixMode=%d depth=%d\n", error, (int)type, matrixDepth[(int)type] ));	
		GLASSERT( 0 );							
	}
#endif
}


void GPUShader::MultMatrix( MatrixType type, const grinliz::Matrix4& m )
{
	// A lot of identities seem to get through...
	if ( m.IsIdentity() )
		return;

	SwitchMatrixMode( type );

	switch( type ) {
#if XENOENGINE_OPENGL == 1
	case MODELVIEW_MATRIX:
	case PROJECTION_MATRIX:
		glMultMatrixf( m.x );
		break;
#elif XENOENGINE_OPENGL == 2
	case MODELVIEW_MATRIX:
		mvStack.Multiply( m );
		break;
	case PROJECTION_MATRIX:
		projStack.Multiply( m );
		break;
#endif
	default:
		GLASSERT( 0 );
		return;
	}	
	CHECK_GL_ERROR;
}


void GPUShader::PopMatrix( MatrixType type )
{
	CHECK_GL_ERROR;

	SwitchMatrixMode( type );
	GLASSERT( matrixDepth[(int)type] > 0 );
	switch( type ) {
#if XENOENGINE_OPENGL == 1
	case MODELVIEW_MATRIX:
	case PROJECTION_MATRIX:
		glPopMatrix();
		break;
#elif XENOENGINE_OPENGL == 2
	case MODELVIEW_MATRIX:
		mvStack.Pop();
		break;
	case PROJECTION_MATRIX:
		projStack.Pop();
		break;
#endif
	default:
		GLASSERT( 0 );
		return;
	}	
	
	matrixDepth[(int)type] -= 1;

	CHECK_GL_ERROR;
}


void GPUShader::SetMatrix( MatrixType type, const Matrix4& m )
{
	CHECK_GL_ERROR;

	SwitchMatrixMode( type );
	GLASSERT( matrixDepth[(int)type] == 0 );

#if XENOENGINE_OPENGL == 1
	glLoadMatrixf( m.x );
#else
	if ( type == PROJECTION_MATRIX ) {
		GLASSERT( projStack.NumMatrix() == 1 );
		projStack.Set( m ); 
	}
	else if ( type == MODELVIEW_MATRIX ) {
		GLASSERT( mvStack.NumMatrix() == 1 );
		mvStack.Set( m );
	}
#endif
}


#if XENOENGINE_OPENGL == 2
const grinliz::Matrix4& GPUShader::ConcatedMatrix( MatrixType type )
{
	if ( type == MODELVIEW_MATRIX ) {
		return mvStack.Top();
	}
	return projStack.Top();
}


const grinliz::Matrix4& GPUShader::ConcatedTextureMatrix( int unit )
{
	GLASSERT( unit >= 0 && unit < 2 );
	return textureStack[unit].Top();
}

#endif

void GPUShader::PushTextureMatrix( int mask )
{
	if ( mask & 1 ) textureStack[0].Push();
	if ( mask & 2 ) textureStack[1].Push();
}


void GPUShader::MultTextureMatrix( int mask, const grinliz::Matrix4& m )
{
	if ( mask & 1 ) textureStack[0].Multiply( m );
	if ( mask & 2 ) textureStack[1].Multiply( m );
}

void GPUShader::PopTextureMatrix( int mask )
{
	if ( mask & 1 ) textureStack[0].Pop();
	if ( mask & 2 ) textureStack[1].Pop();
}


void GPUStream::Clear()
{
	memset( this, 0, sizeof(*this) );
}


GPUStream::GPUStream( const Vertex* vertex )
{
	Clear();

	stride = sizeof( Vertex );
	nPos = 3;
	posOffset = Vertex::POS_OFFSET;
	nNormal = 3;
	normalOffset = Vertex::NORMAL_OFFSET;
	nTexture0 = 2;
	texture0Offset = Vertex::TEXTURE_OFFSET;
}


GPUStream::GPUStream( GamuiType )
{
	Clear();
	stride = sizeof( gamui::Gamui::Vertex );
	nPos = 2;
	posOffset = gamui::Gamui::Vertex::POS_OFFSET;
	nTexture0 = 2;
	texture0Offset = gamui::Gamui::Vertex::TEX_OFFSET;
}


GPUStream::GPUStream( const PTVertex2* vertex )
{
	Clear();
	stride = sizeof( PTVertex2 );
	nPos = 2;
	posOffset = PTVertex2::POS_OFFSET;
	nTexture0 = 2;
	texture0Offset = PTVertex2::TEXTURE_OFFSET;

}


CompositingShader::CompositingShader( bool _blend )
{
	blend = _blend;
	depthWrite = false;
	depthTest = false;
}


int LightShader::locked = 0;


LightShader::LightShader( const Color4F& ambient, const grinliz::Vector4F& direction, const Color4F& diffuse, bool blend )
{
	GLASSERT( !(blend && alphaTest ) );	// technically fine, probably not intended behavior.

	//this->alphaTest = alphaTest;
	this->blend = blend;
	this->ambient = ambient;
	this->direction = direction;
	this->diffuse = diffuse;

	if ( !locked ) {
		SetLightParams();
	}
	locked++;
}


LightShader::~LightShader()
{
	--locked;
}


void LightShader::SetLightParams() const
{
#if XENOENGINE_OPENGL == 1
	static const float white[4]	= { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float black[4]	= { 0.0f, 0.0f, 0.0f, 1.0f };

	// Light 0. The Sun or Moon.
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, &direction.x );
	glLightfv(GL_LIGHT0, GL_AMBIENT,  &ambient.r );
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  &diffuse.r );
	glLightfv(GL_LIGHT0, GL_SPECULAR, black );
	CHECK_GL_ERROR;

	// The material.
	glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, black );
	glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, black );
	glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  white );
	glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  white );
	CHECK_GL_ERROR;
#endif
}

/* static */ int PointParticleShader::particleSupport = 0;

/*static*/ bool PointParticleShader::IsSupported()
{
	if ( particleSupport == 0 ) {
		const char* extensions = (const char*)glGetString( GL_EXTENSIONS );
		const char* sprite = strstr( extensions, "point_sprite" );
		particleSupport = (sprite) ? 1 : -1;
	}
	return ( particleSupport > 0);
}


PointParticleShader::PointParticleShader()
{
	this->depthTest = true;
	this->depthWrite = false;
	this->blend = true;
}


void PointParticleShader::DrawPoints( Texture* texture, float pointSize, int start, int count )
{
	#ifdef USING_GL	
		glEnable(GL_POINT_SPRITE);
		glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
	#else
		glEnable(GL_POINT_SPRITE_OES);
		glTexEnvx(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
	#endif
	CHECK_GL_ERROR;

	GLASSERT( texture0 == 0 );
	GLASSERT( texture1 == 0 );
	
	// Will disable the texture units:
	SetState( *this );

	// Which is a big cheat because we need to bind a texture without texture coordinates.
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture->GLID() );

	glPointSize( pointSize );
	CHECK_GL_ERROR;

	glDrawArrays( GL_POINTS, start, count );
	CHECK_GL_ERROR;

	#ifdef USING_GL	
		glDisable(GL_POINT_SPRITE);
	#else
		glDisable(GL_POINT_SPRITE_OES);
	#endif

	// Clear the texture thing back up.
	glDisable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, 0 );
		
	drawCalls++;
	trianglesDrawn += count;
}


QuadParticleShader::QuadParticleShader()
{
	this->depthTest = true;
	this->depthWrite = false;
	this->blend = true;
}

