#include "tga.h"
#include "../grinliz/gldebug.h"

struct TGAHeader
{
    U8  identsize;          // size of ID field that follows 18 byte header (0 usually)
    U8  colourmaptype;      // type of colour map 0=none, 1=has palette
    U8  imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    U16 colourmapstart;     // first colour map entry in palette
    U16 colourmaplength;    // number of colours in palette
    U8  colourmapbits;      // number of bits per palette entry 15,16,24,32

    U16 xstart;             // image x origin
    U16 ystart;             // image y origin
    U16 width;              // image width in pixels
    U16 height;             // image height in pixels
    U8  bits;               // image bits per pixel 8,16,24,32
    U8  descriptor;         // image descriptor bits (vh flip bits)
};


U8 Read8( FILE* fp )
{
	U8 data = (U8) fgetc( fp );
	return data;
}

U16 Read16( FILE* fp ) {
	U32 data0 = (U32) fgetc( fp );
	U32 data1 = (U32) fgetc( fp );

	return (U16)(data0 | (data1<<8 ));
}

U32 Read32( FILE* fp ) {
	U32 data0 = (U32) fgetc( fp );
	U32 data1 = (U32) fgetc( fp );
	U32 data2 = (U32) fgetc( fp );
	U32 data3 = (U32) fgetc( fp );

	return data0 | (data1<<8 ) | (data2<<16) | (data3<<24);
}


bool LoadTGA( Surface* surface, FILE* fp, bool invert )
{
	TGAHeader header;
	header.identsize = Read8( fp );
	header.colourmaptype = Read8( fp );
	header.imagetype = Read8( fp );
	header.colourmapstart = Read16( fp );
	header.colourmaplength = Read16( fp );
	header.colourmapbits = Read8( fp );
	header.xstart = Read16( fp );
	header.ystart = Read16( fp );
	header.width = Read16( fp );
	header.height = Read16( fp );
	header.bits = Read8( fp );
	header.descriptor = Read8( fp );

	// type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

	GLASSERT( header.imagetype == 2 );
	GLASSERT( header.bits == 24 || header.bits == 32 );
	GLASSERT( header.colourmapstart == 0 );
	GLASSERT( header.colourmaplength == 0 );
	GLOUTPUT(( "width=%d height=%d imagetype=%d bitsPerPixel=%d\n", header.width, header.height, header.imagetype, header.bits ));

	surface->Set( header.width, header.height, header.bits/8 );

	int start = 0;
	int end = surface->Height();
	int bias = 1;
	int scanline = header.width * (header.bits / 8);
	U8* pixels = surface->Pixels();

	if ( invert ) {
		start = surface->Height()-1;
		end = -1;
		bias = -1;
	}

	for( int j=start; j!=end; j+=bias ) {
		fread( pixels, scanline, 1, fp );
		pixels += scanline;
	}
	return true;
}
