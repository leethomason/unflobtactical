#include "shadermanager.h"
#include "platformgl.h"
#include "texture.h"

#include "shaders.inc"

using namespace grinliz;

ShaderManager::ShaderManager()
{
	memset( shaderArr, 0, sizeof( Shader )*MAX_SHADERS );
}


ShaderManager::~ShaderManager()
{
	for( int i=0; i<MAX_SHADERS; ++i ) {
		DeleteProgram( &shaderArr[i] );
	}
}


void ShaderManager::AppendFlag( GLString* str, const char* flag, int set )
{
	str->append( flag );
	str->append( "=" );
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
	glUseProgram( shader->prog );
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
	}
	GLASSERT( i < MAX_SHADERS );
	if ( i == MAX_SHADERS ) {
		return &shaderArr[0];
	}

	Shader* shader = &shaderArr[i];
	shader->flags = flags;

	shader->vertexProg = glCreateShader( GL_VERTEX_SHADER );
	shader->fragmentProg = glCreateShader( GL_FRAGMENT_SHADER );

	header = "/* header */";
	AppendFlag( &header, "TEXTURE0",			flags & TEXTURE0 );
	AppendFlag( &header, "TEXTURE0_TRANSFORM",	flags & TEXTURE0_TRANSFORM );
	AppendFlag( &header, "TEXTURE1",			flags & TEXTURE1 );
	AppendFlag( &header, "TEXTURE1_TRANSFORM",	flags & TEXTURE1_TRANSFORM );
	AppendFlag( &header, "NORMALS",				flags & NORMALS );
	AppendFlag( &header, "COLORS",				flags & COLORS );
	AppendFlag( &header, "COLOR_MULTIPLIER",	flags & COLOR_MULTIPLIER );
	AppendFlag( &header, "LIGHTING_DIFFUSE",	flags & LIGHTING_DIFFUSE );	

	const char* vertexSrc[2] = { header.c_str(), uberVertex };
	glShaderSource( shader->vertexProg, 2, vertexSrc, 0 );
	glCompileShader( shader->vertexProg );
	glGetProgramInfoLog( shader->vertexProg, LEN, &outLen, buf );
	if ( outLen > 0 ) {
		GLOUTPUT(( "Vertex %d:\n%s\n", flags, buf ));
	}
	CHECK_GL_ERROR;

	const char* fragmentSrc[2] = { header.c_str(), uberFragment };
	glShaderSource( shader->fragmentProg, 2, fragmentSrc, 0 );
	glCompileShader( shader->fragmentProg );
	glGetProgramInfoLog( shader->fragmentProg, LEN, &outLen, buf );
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
	glDetachShader( shader->prog, shader->vertexProg );
	CHECK_GL_ERROR;
	glDetachShader( shader->prog, shader->fragmentProg );
	CHECK_GL_ERROR;
	glDeleteShader( shader->prog );
	glDeleteProgram( shader->vertexProg );
	glDeleteProgram( shader->fragmentProg );
	CHECK_GL_ERROR;
}


	