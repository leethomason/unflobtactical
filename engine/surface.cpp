/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include "surface.h"
#include "platformgl.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glstringutil.h"
#include <string.h>
#include "serialize.h"

using namespace grinliz;

Surface::Surface() : format( -1 ), w( 0 ), h( 0 ), allocated( 0 ), pixels( 0 )
{
	name[0] = 0;
}

Surface::~Surface()
{
	if ( pixels ) {
		delete [] pixels;
	}
}


void Surface::SetName( const char* n )
{
	StrNCpy( name, n, EL_FILE_STRING_LEN );
}


void Surface::Set( int f, int w, int h ) 
{
	this->format = f;
	this->w = w;
	this->h = h;
	
	int needed = w*h*BytesPerPixel();

	if ( allocated < needed ) {
		if ( pixels ) {
			delete [] pixels;
		}
		pixels = new U8[needed];
		allocated = needed;
	}
}


/*static*/ int Surface::QueryFormat( const char* formatString )
{
	if ( grinliz::StrEqual( "RGBA16", formatString ) ) {
		return RGBA16;
	}
	else if ( grinliz::StrEqual( "RGB16", formatString ) ) {
		return RGB16;
	}
	else if ( grinliz::StrEqual( "ALPHA", formatString ) ) {
		return ALPHA;
	}
	GLASSERT( 0 );
	return -1;
}


/*
U32 Surface::LoadTexture( FILE* fp, bool *alpha )
{
	TextureHeader header;
	header.Load( fp );

	switch ( header.format ) {
		case GL_ALPHA:	Set( header.width, header.height, 1 );	*alpha = true;	break;
		case GL_RGB:	Set( header.width, header.height, 2 );	*alpha = false;	break;
		case GL_RGBA:	Set( header.width, header.height, 2 );	*alpha = true;	break;
		default:
			GLASSERT( 0 );
			break;
	}

	fread( pixels, 1, w*h*bpp, fp );
//	switch ( bpp ) {
//		case 1:													break;
//		case 2:	grinliz::SwapBufferBE16( (U16*)pixels, w*h );	break;
//		default:
//			GLASSERT( 0 );
//			break;
//	}
	GLOUTPUT(( "Load texture: %s\n", header.name ));

	return LowerCreateTexture( header.format, header.type );
}
*/

/*
void Surface::LoadSurface( FILE* fp, bool* alpha )
{
	TextureHeader header;
	header.Load( fp );

	switch ( header.format ) {
		case GL_ALPHA:	Set( header.width, header.height, 1 );	*alpha = true;	break;
		case GL_RGB:	Set( header.width, header.height, 2 );	*alpha = false;	break;
		case GL_RGBA:	Set( header.width, header.height, 2 );	*alpha = true;	break;
		default:
			GLASSERT( 0 );
			break;
	}

	fread( pixels, 1, w*h*bpp, fp );
}
*/


void Surface::CalcOpenGL( int* glFormat, int* glType )
{
	*glFormat = GL_RGB;
	*glType = GL_UNSIGNED_BYTE;

	switch( format ) {
		case RGBA16:
			*glFormat = GL_RGBA;
			*glType = GL_UNSIGNED_SHORT_4_4_4_4;
			break;

		case RGB16:
			*glFormat = GL_RGB;
			*glType = GL_UNSIGNED_SHORT_5_6_5;
			break;

		case ALPHA:
			*glFormat = GL_ALPHA;
			*glType = GL_UNSIGNED_BYTE;
			break;

		default:
			GLASSERT( 0 );
	}
}


U32 Surface::CreateTexture( int flags )
{
	int glFormat, glType;
	CalcOpenGL( &glFormat, &glType );

	GLASSERT( pixels );
	GLASSERT( w && h && (format >= 0) );
	TESTGLERR();

	glEnable( GL_TEXTURE_2D );

	GLuint texID;
	glGenTextures( 1, &texID );
	glBindTexture( GL_TEXTURE_2D, texID );


	if ( flags & PARAM_NEAREST ) {
		glTexParameteri(	GL_TEXTURE_2D,
							GL_GENERATE_MIPMAP,
							GL_FALSE );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MAG_FILTER,
							GL_NEAREST );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MIN_FILTER,
							GL_NEAREST );
	}
	else {
		glTexParameteri(	GL_TEXTURE_2D,
							GL_GENERATE_MIPMAP,
							GL_TRUE );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MAG_FILTER,
							GL_LINEAR );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MIN_FILTER,
							GL_LINEAR_MIPMAP_NEAREST );
	}
					
	glTexImage2D(	GL_TEXTURE_2D,
					0,
					glFormat,
					w,
					h,
					0,
					glFormat,
					glType,
					pixels );

	TESTGLERR();

	return texID;
}


void Surface::UpdateTexture( U32 glID )
{
	int glFormat, glType;
	CalcOpenGL( &glFormat, &glType );

	glBindTexture( GL_TEXTURE_2D, glID );

	glTexImage2D(	GL_TEXTURE_2D,
					0,
					glFormat,
					w,
					h,
					0,
					glFormat,
					glType,
					pixels );

	TESTGLERR();
}


void Texture::Set( const char* name, U32 glID, bool alpha )
{
	GLASSERT( strlen( name ) < MAX_TEXTURE_NAME );
	StrNCpy( this->name, name, MAX_TEXTURE_NAME );
	this->glID = glID;
	this->alpha = alpha;
}


/*static*/ void TextureManager::Create()
{
	GLASSERT( instance == 0 );
	instance = new TextureManager();
}


/*static*/ void TextureManager::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}


TextureManager::TextureManager()
{
}


TextureManager::~TextureManager()
{
	for( unsigned i=0; i<textureArr.Size(); ++i ) {
		if ( textureArr[i].glID ) {
			glDeleteTextures( 1, (const GLuint*) &textureArr[i].glID );
			textureArr[i].name[0] = 0;
		}
	}
}


/*static*/ TextureManager* TextureManager::instance = 0;


const Texture* TextureManager::GetTexture( const char* name )
{
	return map.Get( name );
}


///*static*/ int TextureManager::Compare( const void * elem1, const void * elem2 )
//{/
//	const Texture* t1 = *((const Texture**)elem1);
//	const Texture* t2 = *((const Texture**)elem2);
//	return strcmp( t1->name, t2->name );
//}



void TextureManager::AddTexture( const char* name, U32 glID, bool alphaTest )
{
	Texture* t = textureArr.Push();
	t->Set( name, glID, alphaTest );
	map.Add( t->name, t );
}




/*static*/ ImageManager* ImageManager::instance = 0;

/*static*/ void ImageManager::Create()
{
	GLASSERT( instance == 0 );
	instance = new ImageManager();
}


/*static*/ void ImageManager::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}

const Surface* ImageManager::GetImage( const char* name )
{
	return map.Get( name );
}

Surface* ImageManager::AddLockedSurface()
{
	GLASSERT( arr.Size() < MAX_IMAGES );
	return arr.Push();
}

void ImageManager::Unlock()
{
	map.Add( arr[ arr.Size()-1 ].Name(), &arr[ arr.Size()-1 ] );
}
