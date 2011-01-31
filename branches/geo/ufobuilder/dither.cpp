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

#include "dither.h"
#include "builder.h"
#include "../grinliz/glutil.h"

int ReducePixelDiv( int c, int shift, int num, int denom )
{
	int max        = 256 >> shift;	// the max color value + 1
	int resolution = 1 << shift;	// the step size of color. So if the shift is 4, then the color resolation in 16

	// Increase cPrime by a number from [0,resolution)
	// The final step is to divide by the resolution,
	// so adding, on average, 1/2 the resolution to account
	// for the division truncating.
	//
	int cPrime = c + resolution*num/denom;	
	if ( cPrime > 255 ) cPrime = 255;

	int r = cPrime >> shift;
	return r;
}


//
// Ordered dithering:
// 1/5   1 3		(adds to 10)
//       4 2
//
// 1/17   1   9   3  11
//       13   5  15   7
//        4  12   2  10
//       16   8  14   6
//
// Originally the code was a diffusion-dither, which looked
// nice in the test images, but tended to created patterns
// that looked odd in the texture. The more regular appearence
// of the ordered dither looks better at run time.

void DitherTo16( const SDL_Surface* surface, int format, bool invert, U16* target )
{
	// Use the dither pattern to both fix error and round up. Since the shift/division tends
	// to round the color down, the dither can diffuse and correct brightness errors.
	//
	//const int pattern[4] = { 1, 3, 4, 2 };
	//const int denom = 5;
	const int pattern[16] = { 1, 9, 3, 11, 13, 5, 15, 7, 4, 12, 2, 10, 16, 8, 14, 6 };
	const int denom = 17;

	for( int j0=0; j0<surface->h; ++j0 ) {
		int j = (invert) ? (surface->h-1-j0) : j0;

		for( int i=0; i<surface->w; ++i ) 
		{
			U8 r, g, b, a;

			U32 c = GetPixel( surface, i, j );
			SDL_GetRGBA( c, surface->format, &r, &g, &b, &a );
			U16 p = 0;
			//int offset = (j&1)*2 + (i&1);
			int offset = (j&3)*4 + (i&3);
			const int numer = pattern[offset];

			switch ( format ) {
				case RGBA16:
					p =	  
						  ( ReducePixelDiv( r, 4, numer, denom ) << 12 )
						| ( ReducePixelDiv( g, 4, numer, denom ) << 8 )
						| ( ReducePixelDiv( b, 4, numer, denom ) << 4)
						| ( ( a>>4 ) << 0 );
					break;

				case RGB16:
					p = 
						  ( ReducePixelDiv( r, 3, numer, denom ) << 11 )
						| ( ReducePixelDiv( g, 2, numer, denom ) << 5 )
						| ( ReducePixelDiv( b, 3, numer, denom ) );
					break;

				default:
					GLASSERT( 0 );
					break;
			}
			*target++ = p;
		}
	}
}

