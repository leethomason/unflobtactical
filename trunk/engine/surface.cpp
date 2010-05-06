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
	delete [] pixels;
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


void Surface::Clear( int c )
{
	int needed = w*h*BytesPerPixel();
	memset( pixels, c, needed );
}


void Surface::BlitImg( const grinliz::Vector2I& target, const Surface* src, const grinliz::Rectangle2I& srcRect )
{
	GLASSERT( target.x >= 0 && target.y >= 0 );
	GLASSERT( target.x + srcRect.Width() <= w );
	GLASSERT( target.y + srcRect.Height() <= h );
	GLASSERT( srcRect.min.x >= 0 && srcRect.max.x < src->Width() );
	GLASSERT( srcRect.min.y >= 0 && srcRect.max.x < src->Height() );
	GLASSERT( src->format == format );

	const int scan = srcRect.Width() * BytesPerPixel();
	const int bpp = BytesPerPixel();

	for( int j=0; j<srcRect.Height(); ++j ) {
		memcpy( pixels + ((h-1)-(j+target.y))*w*bpp + target.x*bpp, 
			    src->pixels + ((src->Height()-1)-(j+srcRect.min.y))*src->Width()*bpp + srcRect.min.x*bpp,
				scan );
	}
}


void Surface::BlitImg(	const grinliz::Rectangle2I& target, 
					const Surface* src, 
					const Matrix2I& xform )
{
	Vector3I t = { 0, 0, 1 };
	Vector3I s = { 0, 0, 1 };

	for( t.y=target.min.y; t.y<=target.max.y; ++t.y ) {
		for( t.x=target.min.x; t.x<=target.max.x; ++t.x ) {
			MultMatrix2I( xform, t, &s );		
			GLASSERT( s.x >= 0 && s.x < src->Width() );
			GLASSERT( s.y >= 0 && s.y < src->Height() );
			GLASSERT( BytesPerPixel() == 2 );	// just a bug...other case not implemented. Also need to fix origin.
			SetImg16( t.x, t.y, src->GetImg16( s.x, s.y ) );
		}
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
