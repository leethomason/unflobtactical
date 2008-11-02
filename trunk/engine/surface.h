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

	void Set( int w, int h, int bpp );

	// Create an opengl texture from this surface.
	U32 CreateTexture();
	// Load the file
	U32 LoadTexture( FILE* fp );

private:
	U32 LowerCreateTexture( int format, int type );


	int w;
	int h;
	int bpp;
	int allocated;
	U8* pixels;
};

void DrawQuad( float x0, float y0, float x1, float y1, U32 textureID );


class Texture
{
public:
	char name[16];
	U32	 glID;

	void Set( const char* name, U32 glID );
};


#endif // UFOATTACK_SURFACE_INCLUDED