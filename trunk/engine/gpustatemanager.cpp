#include "gpustatemanager.h"
#include "platformgl.h"
#include "texture.h"
#include "../gamui/gamui.h"	// for auto setting up gamui stream

/*static*/ GPUVertexBuffer GPUVertexBuffer::Create( const Vertex* vertex, int nVertex )
{
	GPUVertexBuffer buffer;
	if ( GPUShader::SupportsVBOs() ) {
		U32 dataSize  = sizeof(Vertex)*nVertex;
		glGenBuffersX( 1, (GLuint*) &buffer.id );
		glBindBufferX( GL_ARRAY_BUFFER, buffer.id );
		glBufferDataX( GL_ARRAY_BUFFER, dataSize, vertex, GL_STATIC_DRAW );
		glBindBufferX( GL_ARRAY_BUFFER, 0 );
		CHECK_GL_ERROR;
	}
	return buffer;
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

	U32 dataSize  = sizeof(U16)*nIndex;
	glGenBuffersX( 1, (GLuint*) &buffer.id );
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, buffer.id );
	glBufferDataX( GL_ELEMENT_ARRAY_BUFFER, dataSize, index, GL_STATIC_DRAW );
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;

	return buffer;
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
	stack[index] = stack[index] * m;
}


/*static*/ GPUShader GPUShader::current;
/*static*/ int GPUShader::trianglesDrawn = 0;
/*static*/ int GPUShader::drawCalls = 0;
/*static*/ uint32_t GPUShader::uid = 0;
/*static*/ GPUShader::MatrixType GPUShader::matrixMode = MODELVIEW_MATRIX;
/*static*/ MatrixStack GPUShader::textureStack;

/*static*/ int GPUShader::vboSupport = 0;


//static 
bool GPUShader::SupportsVBOs()
{
	if ( vboSupport == 0 ) {
		const char* extensions = (const char*)glGetString( GL_EXTENSIONS );
		const char* vbo = strstr( extensions, "vertex_buffer_object" );
		vboSupport = (vbo) ? 1 : -1;
	}
	return (vboSupport > 0);
}


//static 
void GPUShader::ResetState()
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
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );

	// Blend/Alpha
	glDisable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_ALPHA_TEST );
	glAlphaFunc( GL_GREATER, 0.5f );

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
	glEnable( GL_CULL_FACE );

	CHECK_GL_ERROR;
}


//static 
void GPUShader::SetState( const GPUShader& ns )
{
	CHECK_GL_ERROR;
	GLASSERT( ns.stream.stride > 0 );

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

		glTexCoordPointer(	ns.stream.nTexture0, 
							GL_FLOAT, 
							ns.stream.stride, 
							PTR( ns.streamPtr, ns.stream.texture0Offset ) );	
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

	// Blend
	if ( ns.blend && !current.blend ) {
		glEnable( GL_BLEND );
	}
	else if ( !ns.blend && current.blend ) {
		glDisable( GL_BLEND );
	}

	// Alpha Test
	if ( ns.alphaTest && !current.alphaTest ) {
		glEnable( GL_ALPHA_TEST );
	}
	else if ( !ns.alphaTest && current.alphaTest ) {
		glDisable( GL_ALPHA_TEST );
	}

	// Lighting
	if ( ns.lighting && !current.lighting ) {
		glEnable( GL_LIGHTING );
	}
	else if ( !ns.lighting && current.lighting ) {
		glDisable( GL_LIGHTING );
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

	// color
	if ( ns.color != current.color ) {
		glColor4f( ns.color.x, ns.color.y, ns.color.z, ns.color.w );
	}

	current = ns;
	CHECK_GL_ERROR;

#if defined( USING_GL ) && defined( DEBUG )
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
	}
	ASSERT_SAME( current.stream.HasNormal(), glIsEnabled( GL_NORMAL_ARRAY ) );
	if ( current.stream.HasNormal() ) {
		glGetPointerv( GL_NORMAL_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == PTR( current.streamPtr, current.stream.normalOffset) );
	}
	ASSERT_SAME( current.stream.HasColor(), glIsEnabled( GL_COLOR_ARRAY ) );
	if ( current.stream.HasColor() ) {
		glGetPointerv( GL_COLOR_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == PTR( current.streamPtr, current.stream.colorOffset) );
	}

	ASSERT_SAME( current.blend, glIsEnabled( GL_BLEND ) );
	ASSERT_SAME( current.alphaTest, glIsEnabled( GL_ALPHA_TEST ) );
	ASSERT_SAME( current.lighting, glIsEnabled( GL_LIGHTING ) );

	GLboolean param;
	glGetBooleanv( GL_DEPTH_WRITEMASK, &param );

	ASSERT_SAME( current.depthWrite, param );
	ASSERT_SAME( current.depthTest, glIsEnabled( GL_DEPTH_TEST ) );

	{
		int testMode = 0;
		glGetIntegerv( GL_MATRIX_MODE, &testMode );
		switch( current.matrixMode ) {
		case TEXTURE_MATRIX:	GLASSERT( testMode == GL_TEXTURE );	break;
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


void GPUShader::SetMatrixMode( MatrixType m )
{
	matrixMode = m;
}


GPUShader::~GPUShader()
{
	GLASSERT( matrixDepth[0] == 0 );
	GLASSERT( matrixDepth[1] == 0 );
	GLASSERT( matrixDepth[2] == 0 );
}


void GPUShader::Draw()	// int index, const uint16_t* elements )
{
	SetState( *this );

	GLASSERT( nIndex % 3 == 0 );
	GLASSERT( indexPtr );

	trianglesDrawn += nIndex / 3;
	++drawCalls;

	glDrawElements( GL_TRIANGLES, nIndex, GL_UNSIGNED_SHORT, indexPtr );
	CHECK_GL_ERROR;
}

/*
void GPUShader::Draw( int index, const int* elements ) 
{
	CHECK_GL_ERROR;
	SetState( *this );

	GLASSERT( index % 3 == 0 );
	trianglesDrawn += index / 3;
	++drawCalls;

	glDrawElements( GL_TRIANGLES, index, GL_UNSIGNED_INT, elements );
	CHECK_GL_ERROR;
}
*/


void GPUShader::SwitchMatrixMode( MatrixType type )
{
#if defined( USING_GL ) && defined( DEBUG )
	{
		int testMode = 0;
		glGetIntegerv( GL_MATRIX_MODE, &testMode );
		switch( matrixMode ) {
		case TEXTURE_MATRIX:	GLASSERT( testMode == GL_TEXTURE );	break;
		case MODELVIEW_MATRIX:	GLASSERT( testMode == GL_MODELVIEW );	break;
		case PROJECTION_MATRIX:	GLASSERT( testMode == GL_PROJECTION );	break;
		default: GLASSERT( 0 ); break;
		}
	}
#endif

	if ( matrixMode != type ) {
		switch ( type ) {
		case TEXTURE_MATRIX:
			glMatrixMode( GL_TEXTURE );
			break;
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
	matrixMode = type;
	CHECK_GL_ERROR;
}


void GPUShader::PushMatrix( MatrixType type )
{
	SwitchMatrixMode( type );

	switch( type ) {
	case TEXTURE_MATRIX:
		textureStack.Push();
		break;
	case MODELVIEW_MATRIX:
	case PROJECTION_MATRIX:
		glPushMatrix();
		break;
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
	SwitchMatrixMode( type );

	switch( type ) {
	case TEXTURE_MATRIX:
		textureStack.Multiply( m );
		glLoadMatrixf( textureStack.Top().x );
		break;
	case MODELVIEW_MATRIX:
	case PROJECTION_MATRIX:
		glMultMatrixf( m.x );
		break;
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
	case TEXTURE_MATRIX:
		textureStack.Pop();
		glLoadMatrixf( textureStack.Top().x );
		break;
	case MODELVIEW_MATRIX:
	case PROJECTION_MATRIX:
		glPopMatrix();
		break;
	default:
		GLASSERT( 0 );
		return;
	}	
	
	matrixDepth[(int)type] -= 1;

	CHECK_GL_ERROR;
}


void GPUShader::Stream::Clear()
{
	memset( this, 0, sizeof(*this) );
}


GPUShader::Stream::Stream( const Vertex* vertex )
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


GPUShader::Stream::Stream( GamuiType )
{
	Clear();
	stride = sizeof( gamui::Gamui::Vertex );
	nPos = 2;
	posOffset = gamui::Gamui::Vertex::POS_OFFSET;
	nTexture0 = 2;
	texture0Offset = gamui::Gamui::Vertex::TEX_OFFSET;
}


GPUShader::Stream::Stream( const PTVertex2* vertex )
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


LightShader::LightShader( const Color4F& ambient, const grinliz::Vector4F& direction, const Color4F& diffuse, bool alphaTest, bool blend )
{
	GLASSERT( !(blend && alphaTest ) );	// technically fine, probably not intended behavior.

	this->alphaTest = alphaTest;
	this->blend = blend;
	this->lighting = true;
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
	static const float white[4]	= { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float black[4]	= { 0.0f, 0.0f, 0.0f, 1.0f };

	// Light 0. The Sun or Moon.
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, &direction.x );
	glLightfv(GL_LIGHT0, GL_AMBIENT,  &ambient.x );
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  &diffuse.x );
	glLightfv(GL_LIGHT0, GL_SPECULAR, black );
	CHECK_GL_ERROR;

	// The material.
	glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, black );
	glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, black );
	glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  white );
	glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  white );
	CHECK_GL_ERROR;
}


/*static*/ bool PointParticleShader::IsSupported()
{
#if defined( _MSC_VER )
	return (GLEW_ARB_point_sprite) ? true : false;
#elif defined( USING_ES )
	return false;				// FIXME This should totally be working. Need decent detection. (works on device but not emulator?)
#else
	GLASSERT( 0 );	// nothing wrong with this code path; just unexpected. Check that logic is correct and remove assert.
	return false;
#endif
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

