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
#include <stdio.h>

class Surface
{
public:
	Surface();
	~Surface();

	int Width()				{ return w; }
	int Height()			{ return h; }
	U8* Pixels()			{ return pixels; }
	int BytesPerPixel()		{ return bpp; }

	void Set( int w, int h, int bytesPP );

	// Create an opengl texture from this surface.
	U32 CreateTexture( bool alpha );
	// Update an opengl texture from this surface.
	void UpdateTexture( bool alpha, U32 glID );
	// Load the file
	U32 LoadTexture( FILE* fp, bool* alpha );

private:
	void CalcFormat( bool alpha, int* format, int* type );
	U32 LowerCreateTexture( int format, int type );

	int w;
	int h;
	int bpp;
	int allocated;
	U8* pixels;
};

//void DrawQuad( float x0, float y0, float x1, float y1, U32 textureID );


class Texture
{
public:
	char name[16];
	bool alphaTest;
	U32	 glID;

	void Set( const char* name, U32 glID, bool alphaTest=false );
};


#endif // UFOATTACK_SURFACE_INCLUDED