#include "gpustatemanager.h"
#include "platformgl.h"
#include "texture.h"


/*static*/ GPUShader GPUShader::current;
/*static*/ int GPUShader::trianglesDrawn = 0;
/*static*/ int GPUShader::drawCalls = 0;
/*static*/ uint32_t GPUShader::uid = 0;
/*static*/ GPUShader::MatrixType GPUShader::matrixMode = MODELVIEW_MATRIX;

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

	// Texture1
	if ( ns.texture1 || current.texture1 ) {

		glActiveTexture( GL_TEXTURE1 );
		glClientActiveTexture( GL_TEXTURE1 );

		if ( ns.texture1 && !current.texture1 ) {
			GLASSERT( ns.texture0 );
			GLASSERT( ns.texture1Ptr );

			glEnable( GL_TEXTURE_2D );
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );

			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		}
		else if ( !ns.texture1 && current.texture1 ) {
			glDisable( GL_TEXTURE_2D );
			glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		
		if (  ns.texture1 ) {
			glTexCoordPointer( ns.texture1Components, GL_FLOAT, ns.texture1Stride, ns.texture1Ptr );	
			if ( ns.texture1 != current.texture1 )
			{
				glBindTexture( GL_TEXTURE_2D, ns.texture1->GLID() );
			}
		}

		glActiveTexture( GL_TEXTURE0 );
		glClientActiveTexture( GL_TEXTURE0 );
	}

	CHECK_GL_ERROR;

	// Texture0
	if ( ns.texture0 && !current.texture0 ) {
		GLASSERT( ns.texture0 );
		GLASSERT( ns.texture0Ptr );

		glEnable( GL_TEXTURE_2D );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );

		glTexCoordPointer( ns.texture0Components, GL_FLOAT, ns.texture0Stride, ns.texture0Ptr );	
	}
	else if ( !ns.texture0 && current.texture0 ) {
		glDisable( GL_TEXTURE_2D );
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}
	
	if (  ns.texture0 ) {
		glTexCoordPointer( ns.texture0Components, GL_FLOAT, ns.texture0Stride, ns.texture0Ptr );	
		if ( ns.texture0 != current.texture0 )
		{
			glBindTexture( GL_TEXTURE_2D, ns.texture0->GLID() );
		}
	}

	// Vertex
	if ( ns.vertexPtr && !current.vertexPtr ) {
		glEnableClientState( GL_VERTEX_ARRAY );
	}
	else if ( !ns.vertexPtr && current.vertexPtr ) {
		glDisableClientState( GL_VERTEX_ARRAY );
	}
	if ( ns.vertexPtr ) {
		glVertexPointer( ns.vertexComponents, GL_FLOAT, ns.vertexStride, ns.vertexPtr );
	}

	// Normal
	if ( ns.normalPtr && !current.normalPtr ) {
		glEnableClientState( GL_NORMAL_ARRAY );
	}
	else if ( !ns.normalPtr && current.normalPtr ) {
		glDisableClientState( GL_NORMAL_ARRAY );
	}
	if ( ns.normalPtr ) {
		glNormalPointer( GL_FLOAT, ns.normalStride, ns.normalPtr );
	}

	// Color
	if ( ns.colorPtr && !current.colorPtr ) {
		glEnableClientState( GL_COLOR_ARRAY );
	}
	else if ( !ns.colorPtr && current.colorPtr ) {
		glDisableClientState( GL_COLOR_ARRAY );
	}
	if ( ns.colorPtr ) {
		glColorPointer( ns.colorComponents, GL_FLOAT, ns.colorStride, ns.colorPtr );
	}

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
	if ( ns.lightParams != current.lightParams ) {
		ns.SetLightParams();
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
	ASSERT_SAME( current.texture1, current.texture1Ptr );
	ASSERT_SAME( current.texture1, glIsEnabled( GL_TEXTURE_COORD_ARRAY ) );
	if ( current.texture1 ) {
		glGetPointerv( GL_TEXTURE_COORD_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == current.texture1Ptr );
	}

	glActiveTexture( GL_TEXTURE0 );
	glClientActiveTexture( GL_TEXTURE0 );
	ASSERT_SAME( current.texture0, glIsEnabled( GL_TEXTURE_2D ) );
	ASSERT_SAME( current.texture0, current.texture0Ptr );
	ASSERT_SAME( current.texture0, glIsEnabled( GL_TEXTURE_COORD_ARRAY ) );
	if ( current.texture0 ) {
		glGetPointerv( GL_TEXTURE_COORD_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == current.texture0Ptr );
	}

	ASSERT_SAME( current.vertexPtr, glIsEnabled( GL_VERTEX_ARRAY ) );
	if ( current.vertexPtr ) {
		glGetPointerv( GL_VERTEX_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == current.vertexPtr );
	}
	ASSERT_SAME( current.normalPtr, glIsEnabled( GL_NORMAL_ARRAY ) );
	if ( current.normalPtr ) {
		glGetPointerv( GL_NORMAL_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == current.normalPtr );
	}
	ASSERT_SAME( current.colorPtr, glIsEnabled( GL_COLOR_ARRAY ) );
	if ( current.colorPtr ) {
		glGetPointerv( GL_COLOR_ARRAY_POINTER, &ptr );
		GLASSERT( ptr == current.colorPtr );
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


void GPUShader::Draw( int index, const uint16_t* elements )
{
	SetState( *this );

	GLASSERT( index % 3 == 0 );
	trianglesDrawn += index / 3;
	++drawCalls;

	glDrawElements( GL_TRIANGLES, index, GL_UNSIGNED_SHORT, elements );
	CHECK_GL_ERROR;
}


void GPUShader::Draw( int index, const int* elements ) 
{
	SetState( *this );

	GLASSERT( index % 3 == 0 );
	trianglesDrawn += index / 3;
	++drawCalls;

	glDrawElements( GL_TRIANGLES, index, GL_INT, elements );
	CHECK_GL_ERROR;
}


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
}


void GPUShader::PushMatrix( MatrixType type )
{
	SwitchMatrixMode( type );
	glPushMatrix();
	matrixDepth[(int)type] += 1;

	CHECK_GL_ERROR;
}


void GPUShader::MultMatrix( MatrixType type, const grinliz::Matrix4& m )
{
	SwitchMatrixMode( type );
	glMultMatrixf( m.x );

	CHECK_GL_ERROR;
}


void GPUShader::PopMatrix( MatrixType type )
{
	CHECK_GL_ERROR;

	SwitchMatrixMode( type );
	GLASSERT( matrixDepth[(int)type] > 0 );
	glPopMatrix();
	matrixDepth[(int)type] -= 1;

	CHECK_GL_ERROR;
}


CompositingShader::CompositingShader( bool _blend )
{
	blend = _blend;
	depthWrite = false;
	depthTest = false;
}


LightShader::LightShader( const Color4F& ambient, const grinliz::Vector4F& direction, const Color4F& diffuse, bool alphaTest )
{
	lightParams = ++GPUShader::uid;
	this->alphaTest = alphaTest;
	this->lighting = true;
	this->ambient = ambient;
	this->direction = direction;
	this->diffuse = diffuse;
}


LightShader::~LightShader()
{

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
#ifdef WIN32
	return (GLEW_ARB_point_sprite) ? true : false;
#elif defined( USING_ES )
	return true;
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


void PointParticleShader::DrawPoints( float pointSize, int start, int count )
{
	#ifdef USING_GL	
		glEnable(GL_POINT_SPRITE);
		glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
	#else
		glEnable(GL_POINT_SPRITE_OES);
		glTexEnvx(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
	#endif
	CHECK_GL_ERROR;

	glPointSize( pointSize );
	CHECK_GL_ERROR;

	glDrawArrays( GL_POINTS, start, count );
	CHECK_GL_ERROR;

	drawCalls++;
	trianglesDrawn += count;
}


QuadParticleShader::QuadParticleShader()
{
	this->depthTest = true;
	this->depthWrite = false;
	this->blend = true;
}

