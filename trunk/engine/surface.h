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

#ifndef UFOATTACK_SURFACE_INCLUDED
#define UFOATTACK_SURFACE_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../engine/ufoutil.h"
#include "enginelimits.h"
#include <stdio.h>

class Surface
{
public:
	struct RGBA {
		U8 r, g, b, a;
	};

	static U16 CalcColorRGB16( RGBA rgba )
	{
		U32 c =   ((rgba.r>>3) << 11)
				| ((rgba.g>>2) << 5)
				| ((rgba.b>>3) );
		return (U16)c;
	}

	static void CalcRGB16( U16 c, RGBA* rgb ) {
		U32 r = (c & 0xF800) >> 11;		// 5, 0-31
		U32 g = (c & 0x07E0) >> 5;		// 6, 0-61
		U32 b = (c & 0x001F);			// 5, 0-31

		// 0-15 is the range.
		// 0  -> 0
		// 15 -> 255
		rgb->r = (r<<3)|(r>>2);
		rgb->g = (g<<2)|(g>>4);
		rgb->b = (b<<3)|(b>>2);
		rgb->a = 255;
	}
	static void CalcRGBA16( U16 color, RGBA* rgb ) {
		U32 r = (color>>12);
		U32 g = (color>>8)&0x0f;
		U32 b = (color>>4)&0x0f;
		U32 a = color&0xff;

		// 0-15 is the range.
		// 0  -> 0
		// 15 -> 255
		rgb->r = (r<<4)|r;
		rgb->g = (g<<4)|g;
		rgb->b = (b<<4)|b;
		rgb->a = (a<<4)|a;
	}

	Surface();
	~Surface();

	enum {		// channels	bytes
		RGBA16,	// 4444		2
		RGB16,	// 565		2
		ALPHA,	// 8		1
	};

	int			Width()	const			{ return w; }
	int			Height() const			{ return h; }
	U8*			Pixels()				{ return pixels; }
	const U8*	Pixels() const			{ return pixels; }
	int			BytesPerPixel() const	{ return (format==ALPHA) ? 1 : 2; }
	int			BytesInImage() const	{ GLASSERT( w*h*BytesPerPixel() <= allocated ); return w*h*BytesPerPixel(); }
	bool		Alpha() const			{ return (format!=RGB16) ? true : false; }
	int			Format() const			{ return format; }

	// Surfaces are flipped (as are images) for OpenGL. This "unflips" back to map==pixel coordinates.
	U16	ImagePixel16( int x, int y ) const	{ 
		GLASSERT( x >=0 && x < Width() );
		GLASSERT( y >=0 && y < Height() );
		GLASSERT( BytesPerPixel() == 2 );
		return *((const U16*)pixels + (h-1-y)*w + x);
	}
	void SetImagePixel16( int x, int y, U16 c ) {
		GLASSERT( x >=0 && x < Width() );
		GLASSERT( y >=0 && y < Height() );
		GLASSERT( BytesPerPixel() == 2 );
		*((U16*)pixels + (h-1-y)*w + x) = c;
	}

	U16 Pixel16( int x, int y ) const {
		GLASSERT( x >=0 && x < Width() );
		GLASSERT( y >=0 && y < Height() );
		GLASSERT( BytesPerPixel() == 2 );
		return *((const U16*)pixels + (h-1-y)*w + x);
	}

	void SetPixel16( int x, int y, U16 c ) {
		GLASSERT( x >=0 && x < Width() );
		GLASSERT( y >=0 && y < Height() );
		GLASSERT( BytesPerPixel() == 2 );
		*((U16*)pixels + y*w + x) = c;
	}

	// Set the format and allocate memory.
	void Set( int format, int w, int h );

	// Create an opengl texture from this surface.
	U32 CreateTexture();
	// Update an opengl texture from this surface.
	void UpdateTexture( U32 glID );

	// convert a string to one of the supported surface formats
	static int QueryFormat( const char* formatString );
	// calculate the opengl format and type
	void CalcOpenGL(int* glFormat, int* glType );

	void SetName( const char* n );
	const char* Name() const			{ return name; }

private:
	int format;
	int w;
	int h;
	int allocated;
	char name[EL_FILE_STRING_LEN];
	U8* pixels;
};


class Texture
{
public:
	char name[16];		// must be first in the class for search to work! (strcmp used in the TextureManager)
	bool alpha;
	U32	 glID;

	void Set( const char* name, U32 glID, bool alpha );
};


class TextureManager
{
public:
	static TextureManager* Instance()	{ GLASSERT( instance ); return instance; }
	
	const Texture* GetTexture( const char* name );
	void AddTexture( const char* name, U32 glID, bool alpha );

	static void Create();
	static void Destroy();
private:
	TextureManager();
	~TextureManager();

	static int Compare( const void * elem1, const void * elem2 );

	enum {
		MAX_TEXTURES = 30		// increase as needed
	};

	static TextureManager* instance;
	CArray< Texture, MAX_TEXTURES > textureArr;		// textures
	Texture* texturePtr[MAX_TEXTURES];				// sorted pointers to textures
	bool sorted;
};

#endif // UFOATTACK_SURFACE_INCLUDED