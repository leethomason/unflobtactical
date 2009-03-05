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
	U32 LoadTexture( FILE* fp );

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