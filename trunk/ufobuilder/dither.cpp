#include "dither.h"
#include "builder.h"
#include "../grinliz/glutil.h"

/*
for each y from top to bottom
   for each x from left to right
      oldpixel := pixel[x][y]
      newpixel := find_closest_palette_color(oldpixel)
      pixel[x][y] := newpixel
      quant_error := oldpixel - newpixel
      pixel[x+1][y] := pixel[x+1][y] + 7/16 * quant_error
      pixel[x-1][y+1] := pixel[x-1][y+1] + 3/16 * quant_error
      pixel[x][y+1] := pixel[x][y+1] + 5/16 * quant_error
      pixel[x+1][y+1] := pixel[x+1][y+1] + 1/16 * quant_error

*/

// err is (colorWanted - colorPalette)
int ReducePixel( int c, int shift, int* err )
{
	//	err = c & ((1<<shift)-1)
	//	return c >> shift;
	int max = 256 >> shift;
	int offset = (max-1)/2;
	int cPrime = ( c + offset );
	if ( cPrime > 255 )
		cPrime = 255;

	int r = cPrime >> shift;
	*err = c - (r<<shift);
	return r;
}


void DitherTo16( const SDL_Surface* in, int format, bool invert, U16* target )
{
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom( in->pixels, in->w, in->h, in->format->BitsPerPixel, in->pitch,
													 in->format->Rmask, in->format->Gmask, in->format->Bmask, in->format->Amask );

	for( int j0=0; j0<surface->h; ++j0 ) {
		int j = (invert) ? (surface->h-1-j0) : j0;

		for( int i=0; i<surface->w; ++i ) 
		{
			U8 r, g, b, a;
			int errR, errG, errB;	// Don't dither the alpha - that has side effects around masking and blending.

			U32 c = GetPixel( surface, i, j );
			SDL_GetRGBA( c, surface->format, &r, &g, &b, &a );
			U16 p = 0;

			switch ( format ) {
				case RGBA16:
					p =	  
						  ( ReducePixel( r, 4, &errR ) << 12 )
						| ( ReducePixel( g, 4, &errG ) << 8 )
						| ( ReducePixel( b, 4, &errB ) << 4)
						| ( ( a>>4 ) << 0 );
					break;

				case RGB16:
					p = 
						  ( ReducePixel( r, 3, &errR ) << 11 )
						| ( ReducePixel( g, 2, &errG ) << 5 )
						| ( ReducePixel( b, 3, &errB ) );
					break;

				default:
					GLASSERT( 0 );
					break;
			}
			*target++ = p;
			
			const int dx[] = { 1, -1, 0, 1 };
			const int dy[] = { 0, 1, 1, 1 };
			const int t[]  = { 7, 3, 5, 1 };

			for( int k=0; k<4; ++k ) {
				int x = i+dx[k];
				int y = j+dy[k];
				if ( x < surface->w && x >= 0 && y < surface->h && y >= 0 ) 
				{
					U32 c0 = GetPixel( surface, x, y );
					U8 r0, g0, b0, a0;
					SDL_GetRGBA( c0, surface->format, &r0, &g0, &b0, &a0 );

					int r1 = grinliz::Clamp( (int)r0 + (errR * t[k])/16, 0, 255 );
					int g1 = grinliz::Clamp( (int)g0 + (errG * t[k])/16, 0, 255 );
					int b1 = grinliz::Clamp( (int)b0 + (errB * t[k])/16, 0, 255 );

					U32 cPrime = SDL_MapRGBA( surface->format, r1, g1, b1, a0 );
					PutPixel( surface, x, y, cPrime );
				}
			}
		}
	}
	SDL_FreeSurface( surface );
}

