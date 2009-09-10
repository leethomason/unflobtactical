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
#include <stdio.h>

class Surface
{
public:
	Surface();
	~Surface();

	int Width()	const			{ return w; }
	int Height() const			{ return h; }
	U8* Pixels()				{ return pixels; }
	const U8* Pixels() const	{ return pixels; }
	int BytesPerPixel() const	{ return bpp; }

	void Set( int w, int h, int bytesPP );

	// Create an opengl texture from this surface.
	U32 CreateTexture( bool alpha );
	// Update an opengl texture from this surface.
	void UpdateTexture( bool alpha, U32 glID );
	// Load the file
	U32 LoadTexture( FILE* fp, bool* alpha );
	// Load the file (no texture)
	void LoadSurface( FILE* fp, bool* alpha );

private:
	void CalcFormat( bool alpha, int* format, int* type );
	U32 LowerCreateTexture( int format, int type );

	int w;
	int h;
	int bpp;
	int allocated;
	U8* pixels;
};


class Texture
{
public:
	char name[16];		// must be first in the class for search to work! (strcmp used in the TextureManager)
	bool alphaTest;
	U32	 glID;

	void Set( const char* name, U32 glID, bool alphaTest=false );
};


class TextureManager
{
public:
	static TextureManager* Instance()	{ GLASSERT( instance ); return instance; }
	
	const Texture* GetTexture( const char* name );
	void AddTexture( const char* name, U32 glID, bool alphaTest=false );

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
	//CDynArray< Texture > textureArr;	// works...except changes texture pointers around after the fact.
	CArray< Texture, MAX_TEXTURES > textureArr;		// textures
	Texture* texturePtr[MAX_TEXTURES];				// sorted pointers to textures
	bool sorted;
};

#endif // UFOATTACK_SURFACE_INCLUDED