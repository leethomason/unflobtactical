#include "surface.h"
#include "platformgl.h"
#include "../grinliz/glutil.h"
#include <string.h>

Surface::Surface() : w( 0 ), h( 0 ), bpp( 0 ), allocated( 0 ), pixels( 0 )
{}

Surface::~Surface()
{
	if ( pixels ) {
		delete [] pixels;
	}
}

void Surface::Set( int w, int h, int bpp ) 
{
	this->w = w;
	this->h = h;
	this->bpp = bpp;
	
	int needed = w*h*bpp;

	if ( allocated < needed ) {
		if ( pixels ) {
			delete [] pixels;
		}
		pixels = new U8[needed];
		allocated = needed;
	}
}

U32 Surface::LoadTexture( FILE* fp )
{
	int _format, _type, _w, _h;
	char name[16];
	fread( name, 16, 1, fp );		// not currently used.
	fread( &_format, 4, 1, fp );
	fread( &_type, 4, 1, fp );
	fread( &_w, 4, 1, fp );
	fread( &_h, 4, 1, fp );

	_format = grinliz::SwapBE32( _format );
	_type = grinliz::SwapBE32( _type );
	_w = grinliz::SwapBE32( _w );
	_h = grinliz::SwapBE32( _h );

	switch ( _format ) {
		case GL_ALPHA:	Set( _w, _h, 1 );	break;
		case GL_RGB:	Set( _w, _h, 2 );	break;
		case GL_RGBA:	Set( _w, _h, 2 );	break;
		default:
			GLASSERT( 0 );
			break;
	}

	fread( pixels, 1, w*h*bpp, fp );
	switch ( bpp ) {
		case 1:													break;
		case 2:	grinliz::SwapBufferBE16( (U16*)pixels, w*h );	break;
		default:
			GLASSERT( 0 );
			break;
	}
	GLOUTPUT(( "Load texture: %s\n", name ));

	return LowerCreateTexture( _format, _type );
}


void Surface::CalcFormat( bool alpha, int* format, int* type )
{
	*format = GL_RGB;
	*type = GL_UNSIGNED_BYTE;

	if ( bpp == 2 ) {
		if ( alpha ) {
			*format = GL_RGBA;
			*type = GL_UNSIGNED_SHORT_4_4_4_4;
		}
		else {
			*format = GL_RGB;
			*type = GL_UNSIGNED_SHORT_5_6_5;
		}
	}
	else if ( bpp == 3 ) {
		GLASSERT( !alpha );
		*format = GL_RGB;
		*type = GL_UNSIGNED_BYTE;
	}
	else if ( bpp == 4 ) {
		GLASSERT( alpha );
		*format = GL_RGBA;
	}
}


U32 Surface::CreateTexture( bool alpha )
{
	int format, type;
	CalcFormat( alpha, &format, &type );
	return LowerCreateTexture( format, type );
}


U32 Surface::LowerCreateTexture( int format, int type )
{
	GLASSERT( pixels );
	GLASSERT( w && h && bpp );
	TESTGLERR();

	glEnable( GL_TEXTURE_2D );

	GLuint texID;
	glGenTextures( 1, &texID );
	glBindTexture( GL_TEXTURE_2D, texID );


	glTexParameteri(	GL_TEXTURE_2D,
						GL_GENERATE_MIPMAP,
						GL_TRUE );


	glTexParameteri(	GL_TEXTURE_2D,
						GL_TEXTURE_MAG_FILTER,
						GL_LINEAR );

	glTexParameteri(	GL_TEXTURE_2D,
						GL_TEXTURE_MIN_FILTER,
						GL_LINEAR_MIPMAP_NEAREST );
	/*
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	*/
					
	glTexImage2D(	GL_TEXTURE_2D,
					0,
					format,
					w,
					h,
					0,
					format,
					type,
					pixels );

	TESTGLERR();

	return texID;
}


void Surface::UpdateTexture( bool alpha, U32 glID )
{
	int format, type;
	CalcFormat( alpha, &format, &type );

	glBindTexture( GL_TEXTURE_2D, glID );

	glTexImage2D(	GL_TEXTURE_2D,
					0,
					format,
					w,
					h,
					0,
					format,
					type,
					pixels );

	TESTGLERR();
}


/*void DrawQuad( float x0, float y0, float x1, float y1, U32 textureID )
{
	float v[8] = { x0, y0, x1, y0, x1, y1, x0, y1 };
	float t[8] = { 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f };

	glColor4f( 1.f, 1.f, 1.f, 1.f );
	glVertexPointer( 2, GL_FLOAT, 0, v );
	if ( textureID ) {
		glTexCoordPointer( 2, GL_FLOAT, 0, t );
	}

	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
}
*/


void Texture::Set( const char* name, U32 glID, bool alphaTest )
{
	GLASSERT( strlen( name ) < 16 );
	strcpy( this->name, name );
	this->glID = glID;
	this->alphaTest = alphaTest;
}
