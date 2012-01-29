#include "shadermanager.h"
#include "platformgl.h"
#include "texture.h"

#if XENOENGINE_OPENGL == 2
#include "shaders.inc"

using namespace grinliz;

ShaderManager* ShaderManager::instance = 0;

ShaderManager::ShaderManager() : active( 0 )
{
}


ShaderManager::~ShaderManager()
{
	for( int i=0; i<MAX_SHADERS; ++i ) {
		if ( shaderArr[i].prog ) {
			DeleteProgram( &shaderArr[i] );
		}
	}
}


void ShaderManager::ClearStream()
{
	while( !activeStreams.Empty() ) {
		glDisableVertexAttribArray( activeStreams.Pop() );
	}
	CHECK_GL_ERROR;
}


void ShaderManager::SetStreamData( int id, int size, int type, int stride, const void* data )
{
	const char* var = "";
	switch ( id ) {
	case A_TEXTURE0:	var = "a_uv0";		break;
	case A_TEXTURE1:	var = "a_uv1";		break;
	case A_POS:			var = "a_pos";		break;
	case A_NORMAL:		var = "a_normal";	break;
	case A_COLOR:		var = "a_color";	break;
	default:			GLASSERT( 0 );		break;
	}
	
	int loc = glGetAttribLocation( active->prog, var );
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer( loc, size, type, 0, stride, data );
	CHECK_GL_ERROR;

	activeStreams.Push( loc );
}


void ShaderManager::SetTransforms( const grinliz::Matrix4& mvp, const grinliz::Matrix4& normal )
{
	int loc = glGetUniformLocation( active->prog, "u_mvpMatrix" );
	glUniformMatrix4fv( loc, 1, false, mvp.x );
	if ( active->flags & LIGHTING_DIFFUSE ) {
		int loc = glGetUniformLocation( active->prog, "u_normalMatrix" );
		glUniformMatrix4fv( loc, 1, false, normal.x );
	}
	CHECK_GL_ERROR;
}


void ShaderManager::SetTextureTransform( int index, const grinliz::Matrix4& mat )
{
	int loc = glGetUniformLocation( active->prog, index==0 ? "u_texMat0" : "u_texMat1" );
	glUniformMatrix4fv( loc, 1, false, mat.x );
}


void ShaderManager::SetColorMultiplier( const grinliz::Color4F& color )
{
	int loc = glGetUniformLocation( active->prog, "u_colorMult" );
	glUniform4fv( loc, 1, &color.r );
	CHECK_GL_ERROR;
}


void ShaderManager::SetDiffuse( const grinliz::Vector4F& dir, const grinliz::Vector4F& ambient, const grinliz::Vector4F& diffuse )
{
	int loc = glGetUniformLocation( active->prog, "u_lightDir" );
	glUniform4fv( loc, 1, &dir.x );
	loc = glGetUniformLocation( active->prog, "u_ambient" );
	glUniform4fv( loc, 1, &ambient.x );
	loc = glGetUniformLocation( active->prog, "u_diffuse" );
	glUniform4fv( loc, 1, &diffuse.x );
}


void ShaderManager::AppendFlag( GLString* str, const char* flag, int set )
{
	str->append( "#define " );
	str->append( flag );
	str->append( " " );
	if ( set ) {
		str->append( "1\n" );
	}
	else {
		str->append( "0\n" );
	}
}


void ShaderManager::ActivateShader( int flags )
{
	const Shader* shader = CreateProgram( flags );
	CHECK_GL_ERROR;
	glUseProgram( shader->prog );
	active = shader;
	CHECK_GL_ERROR;
}


const ShaderManager::Shader* ShaderManager::CreateProgram( int flags )
{
	static const int LEN = 200;
	char buf[LEN];
	int outLen = 0;

	int i;
	for( i=0; i<MAX_SHADERS; ++i ) {
		if ( shaderArr[i].prog && shaderArr[i].flags == flags ) {
			return &shaderArr[i];
		}
		if ( shaderArr[i].prog == 0 ) {
			break;
		}
	}
	GLASSERT( i < MAX_SHADERS );
	if ( i == MAX_SHADERS ) {
		return &shaderArr[0];
	}

	Shader* shader = &shaderArr[i];
	shader->flags = flags;

	shader->vertexProg = glCreateShader( GL_VERTEX_SHADER );
	shader->fragmentProg = glCreateShader( GL_FRAGMENT_SHADER );

	header = "";
	AppendFlag( &header, "TEXTURE0",			flags & TEXTURE0 );
	AppendFlag( &header, "TEXTURE0_TRANSFORM",	flags & TEXTURE0_TRANSFORM );
	AppendFlag( &header, "TEXTURE1",			flags & TEXTURE1 );
	AppendFlag( &header, "TEXTURE1_TRANSFORM",	flags & TEXTURE1_TRANSFORM );
	AppendFlag( &header, "COLORS",				flags & COLORS );
	AppendFlag( &header, "COLOR_MULTIPLIER",	flags & COLOR_MULTIPLIER );
	AppendFlag( &header, "LIGHTING_DIFFUSE",	flags & LIGHTING_DIFFUSE );	

	const char* vertexSrc[2] = { header.c_str(), fixedpipe_vert };
	glShaderSource( shader->vertexProg, 2, vertexSrc, 0 );
	glCompileShader( shader->vertexProg );
	glGetShaderInfoLog( shader->vertexProg, LEN, &outLen, buf );
	if ( outLen > 0 ) {
		GLOUTPUT(( "Vertex %d:\n%s\n", flags, buf ));
	}
	CHECK_GL_ERROR;

	const char* fragmentSrc[2] = { header.c_str(), fixedpipe_frag };
	glShaderSource( shader->fragmentProg, 2, fragmentSrc, 0 );
	glCompileShader( shader->fragmentProg );
	glGetShaderInfoLog( shader->fragmentProg, LEN, &outLen, buf );
	if ( outLen > 0 ) {
		GLOUTPUT(( "Fragment %d:\n%s\n", flags, buf ));
	}
	CHECK_GL_ERROR;

	shader->prog = glCreateProgram();
	glAttachShader( shader->prog, shader->vertexProg );
	glAttachShader( shader->prog, shader->fragmentProg );
	glLinkProgram( shader->prog );
	glGetProgramInfoLog( shader->prog, LEN, &outLen, buf );
	if ( outLen > 0 ) {
		GLOUTPUT(( "Link %d:\n%s\n", flags, buf ));
	}
	CHECK_GL_ERROR;

	// glUseProgram
	return shader;
}


void ShaderManager::DeleteProgram( Shader* shader )
{
	CHECK_GL_ERROR;
	glDetachShader( shader->prog, shader->vertexProg );
	CHECK_GL_ERROR;
	glDetachShader( shader->prog, shader->fragmentProg );
	CHECK_GL_ERROR;
	glDeleteShader( shader->vertexProg );
	glDeleteShader( shader->fragmentProg );
	glDeleteProgram( shader->prog );
	CHECK_GL_ERROR;
}

#endif
	