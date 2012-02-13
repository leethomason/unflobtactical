#include "shadermanager.h"
#include "platformgl.h"
#include "texture.h"

#if XENOENGINE_OPENGL == 2
#include "shaders.inc"

using namespace grinliz;

ShaderManager* ShaderManager::instance = 0;

static const char* gAttributeName[ShaderManager::MAX_ATTRIBUTE] =
{
	"a_uv0",
	"a_uv1",
	"a_pos",
	"a_normal",
	"a_color"
};


static const char* gUniformName[ShaderManager::MAX_UNIFORM] = 
{
	"u_mvpMatrix",
	"u_normalMatrix",
	"u_texMat0",
	"u_texMat1",
	"u_colorMult",
	"u_lightDir",
	"u_ambient",
	"u_diffuse"
};


int ShaderManager::Shader::GetAttributeLocation( int attribute )
{
	GLASSERT( attribute >= 0 && attribute < ShaderManager::MAX_ATTRIBUTE );

	if ( attributeLoc[attribute] < 0 ) {
		attributeLoc[attribute] = glGetAttribLocation( prog, gAttributeName[attribute] );
		GLASSERT( attributeLoc[attribute] >= 0 );
	}
	return attributeLoc[attribute];
}


int ShaderManager::Shader::GetUniformLocation( int uniform )
{
	GLASSERT( uniform >= 0 && uniform < ShaderManager::MAX_UNIFORM );

	if ( uniformLoc[uniform] < 0 ) {
		uniformLoc[uniform] = glGetUniformLocation( prog, gUniformName[uniform] );
		GLASSERT( uniformLoc[uniform] >= 0 );
	}
	return uniformLoc[uniform];
}


ShaderManager::ShaderManager() : active( 0 )
{
}


ShaderManager::~ShaderManager()
{
	for( int i=0; i<shaderArr.Size(); ++i ) {
		if ( shaderArr[i].prog ) {
			DeleteProgram( &shaderArr[i] );
		}
	}
}


void ShaderManager::DeviceLoss() 
{
	// The programs are already deleted.
	active = 0;
	activeStreams.Clear();
	shaderArr.Clear();
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
	int loc = active->GetAttributeLocation( id );
	GLASSERT( loc >= 0 );
#ifdef DEBUG
	const float* ft = (const float*)data;
	for( int i=0; i<activeStreams.Size(); ++i ) {
		GLASSERT( activeStreams[i] != loc );
	}
#endif
	glVertexAttribPointer( loc, size, type, 0, stride, data );
	glEnableVertexAttribArray(loc);
	CHECK_GL_ERROR;

	activeStreams.Push( loc );
}


void ShaderManager::SetUniform( int id, const grinliz::Matrix4& value )
{
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniformMatrix4fv( loc, 1, false, value.x );
	CHECK_GL_ERROR;
}


void ShaderManager::SetUniform( int id, const grinliz::Vector4F& value )
{
	CHECK_GL_ERROR;
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniform4fv( loc, 1, &value.x );
	CHECK_GL_ERROR;
}


void ShaderManager::SetUniform( int id, const grinliz::Vector3F& value )
{
	CHECK_GL_ERROR;
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniform3fv( loc, 1, &value.x );
	CHECK_GL_ERROR;
}


void ShaderManager::SetTexture( int index, Texture* texture )
{
	char name[9] = "texture0";
	name[7] = '0' + index;
	int loc = glGetUniformLocation( active->prog, name );
	GLASSERT( loc >= 0 );
	glUniform1i( loc, index );
	CHECK_GL_ERROR;
}

/*
void ShaderManager::SetTextureTransform( int index, const grinliz::Matrix4& mat )
{
	int loc = glGetUniformLocation( active->prog, index==0 ? "u_texMat0" : "u_texMat1" );
	GLASSERT( loc >= 0 );
	glUniformMatrix4fv( loc, 1, false, mat.x );
}


void ShaderManager::SetColorMultiplier( const grinliz::Color4F& color )
{
	int loc = glGetUniformLocation( active->prog, "u_colorMult" );
//	GLASSERT( loc >= 0 );
	if ( loc >= 0 )
		glUniform4fv( loc, 1, &color.r );
	CHECK_GL_ERROR;
}


void ShaderManager::SetDiffuse( const grinliz::Vector4F& dir, const grinliz::Vector4F& ambient, const grinliz::Vector4F& diffuse )
{
	int loc = glGetUniformLocation( active->prog, "u_lightDir" );
	//GLASSERT( loc >= 0 );
	glUniform3fv( loc, 1, &dir.x );
	loc = glGetUniformLocation( active->prog, "u_ambient" );
	GLASSERT( loc >= 0 );
	glUniform4fv( loc, 1, &ambient.x );
	loc = glGetUniformLocation( active->prog, "u_diffuse" );
	GLASSERT( loc >= 0 );
	glUniform4fv( loc, 1, &diffuse.x );
}
*/


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
	Shader* shader = CreateProgram( flags );
	CHECK_GL_ERROR;
	glUseProgram( shader->prog );
	active = shader;
	CHECK_GL_ERROR;
}


ShaderManager::Shader* ShaderManager::CreateProgram( int flags )
{
	static const int LEN = 200;
	char buf[LEN];
	int outLen = 0;

	for( int i=0; i<shaderArr.Size(); ++i ) {
		if ( shaderArr[i].prog && shaderArr[i].flags == flags ) {
			return &shaderArr[i];
		}
		if ( shaderArr[i].prog == 0 ) {
			break;
		}
	}

	Shader* shader = shaderArr.Push();
	shader->Init();
	shader->flags = flags;

	shader->vertexProg = glCreateShader( GL_VERTEX_SHADER );
	shader->fragmentProg = glCreateShader( GL_FRAGMENT_SHADER );

	header = "";
	AppendFlag( &header, "TEXTURE0",			flags & TEXTURE0 );
	AppendFlag( &header, "TEXTURE0_ALPHA_ONLY",	flags & TEXTURE0_ALPHA_ONLY );
	AppendFlag( &header, "TEXTURE0_TRANSFORM",	flags & TEXTURE0_TRANSFORM );
	AppendFlag( &header, "TEXTURE0_3COMP",		flags & TEXTURE0_3COMP );
	
	AppendFlag( &header, "TEXTURE1",			flags & TEXTURE1 );
	AppendFlag( &header, "TEXTURE1_ALPHA_ONLY",	flags & TEXTURE1_ALPHA_ONLY );
	AppendFlag( &header, "TEXTURE1_TRANSFORM",	flags & TEXTURE1_TRANSFORM );
	AppendFlag( &header, "TEXTURE1_3COMP",		flags & TEXTURE1_3COMP );
	
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
	