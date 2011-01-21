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

#include "text.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "../grinliz/glutil.h"
#include "../grinliz/glrectangle.h"
#include "gpustatemanager.h"

using namespace grinliz;

const float SMALL_SCALE = 0.75f;
const int TEXTURE_WIDTH = 256;
const int TEXTURE_HEIGHT = 128;
const int GLYPH_WIDTH = TEXTURE_WIDTH / UFOText::GLYPH_CX;
const int GLYPH_HEIGHT = TEXTURE_HEIGHT / UFOText::GLYPH_CY;


PTVertex2 UFOText::vBuf[BUF_SIZE*4];
//Vector2F UFOText::tBuf[BUF_SIZE*4];
U16 UFOText::iBuf[BUF_SIZE*6] = { 0, 0 };

Screenport* UFOText::screenport = 0;
Texture* UFOText::texture = 0;
GlyphMetric UFOText::glyphMetric[GLYPH_CX*GLYPH_CY];

void UFOText::InitScreen( Screenport* sp )
{
	screenport = sp;
}


void UFOText::InitTexture( Texture* tex )
{
	GLASSERT( tex );
	texture = tex;
}


void UFOText::Metrics(	int c,							// character in 
						int* advance,					// advance, in pixels
						int* width,
						grinliz::Rectangle2I* src )		// location in texture, in pixels
{
	int cy = (int)c / GLYPH_CX;
	int cx = (int)c - cy*GLYPH_CX;

	// Flip the y axis:
	cy = GLYPH_CY - cy;

	GLASSERT( c < GLYPH_CX*GLYPH_CY );
	GlyphMetric* g = &glyphMetric[c];
	*width = g->width;
	*advance = g->width+1;

	if ( c == 0 ) {
		*advance = *width = GLYPH_WIDTH * 2 / 4;
	}
	src->Set(	cx*GLYPH_WIDTH + g->offset,
				(cy-1)*GLYPH_HEIGHT,
				cx*GLYPH_WIDTH + g->offset + g->width,
				cy*GLYPH_HEIGHT );
}


void UFOText::TextOut( GPUShader* shader, const char* str, int x, int y, int* w, int *h )
{
#ifdef USE_SMALLTEXT
	bool smallText = false;
#endif
	bool rendering = true;
	int xStart = x;

	if ( w ) {
		GLASSERT( h );
		*w = 0;
		*h = GLYPH_HEIGHT;
		rendering = false;
	}

	GLASSERT( !rendering || shader );

	if ( rendering ) {
		if ( iBuf[1] == 0 ) {
			// not initialized
			for( int pos=0; pos<BUF_SIZE; ++pos ) {
				iBuf[pos*6+0] = pos*4 + 0;
				iBuf[pos*6+1] = pos*4 + 1;
				iBuf[pos*6+2] = pos*4 + 2;
				iBuf[pos*6+3] = pos*4 + 0;
				iBuf[pos*6+4] = pos*4 + 2;
				iBuf[pos*6+5] = pos*4 + 3;
			}
		}
	}

	int pos = 0;
	while( *str )
	{
#ifdef USE_SMALLTEXT
		if ( *str == '.' && *(str+1) ) {
			smallText = true;
			++str;
			continue;
		}
		if ( smallText && !( *str >= '0' && *str <= '9' ) ) {
			smallText = false;
		}
#endif

		// Draw a glyph or nothing, at this point:
		GLASSERT( pos < BUF_SIZE );
		int c = *str - 32;

		Rectangle2I src;
		int advance, width;
		Metrics( c, &advance, &width, &src );

		if ( c<=0 || c >= 128 ) {
#ifdef USE_SMALLTEXT
			float scale = smallText ? SMALL_SCALE : 1.0f;
#else
			float scale = 1.0f;
#endif
			x += LRintf((float)advance*scale);
		}
		else {
			if ( rendering ) {
				float tx0 = (float)src.min.x / (float)TEXTURE_WIDTH;
				float tx1 = (float)src.max.x / (float)TEXTURE_WIDTH;
				float ty0 = (float)src.min.y / (float)TEXTURE_HEIGHT;
				float ty1 = (float)src.max.y / (float)TEXTURE_HEIGHT;

				vBuf[pos*4+0].tex.Set( tx0, ty0 );
				vBuf[pos*4+1].tex.Set( tx1, ty0 );
				vBuf[pos*4+2].tex.Set( tx1, ty1 );
				vBuf[pos*4+3].tex.Set( tx0, ty1 );
			}
#ifdef USE_SMALLTEXT
			float scale = smallText ? SMALL_SCALE : 1.0f;
#else
			float scale = 1.0f;
#endif
			if ( rendering ) {
				vBuf[pos*4+0].pos.Set( (float)x,					(float)(y+GLYPH_HEIGHT) );	
				vBuf[pos*4+1].pos.Set( (float)x+(float)width*scale,	(float)(y+GLYPH_HEIGHT) );	
				vBuf[pos*4+2].pos.Set( (float)x+(float)width*scale,	(float)(y+GLYPH_HEIGHT) - (float)GLYPH_HEIGHT*scale );	
				vBuf[pos*4+3].pos.Set( (float)x,					(float)(y+GLYPH_HEIGHT) - (float)GLYPH_HEIGHT*scale );	
			}
			x += LRintf((float)advance*scale);
			++pos;
		}
		if ( rendering ) {
			if ( pos == BUF_SIZE || *(str+1) == 0 ) {
				if ( pos > 0 ) {
					GPUShader::Stream stream( vBuf );
					shader->SetStream( stream, vBuf, pos*6, iBuf );
					shader->SetTexture0( texture );

					shader->Draw();
					pos = 0;
				}
			}
		}
		++str;
	}
	if ( w ) {
		*w = x - xStart;
	}
}


void UFOText::GlyphSize( const char* str, int* width, int* height )
{
	TextOut( 0, str, 0, 0, width, height );
}


void UFOText::Draw( int x, int y, const char* format, ... )
{
	screenport->SetUI( 0 );
	CompositingShader shader( true );

    va_list     va;
	const int	size = 1024;
    char		buffer[size];

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    vsnprintf_s( buffer, size, _TRUNCATE, format, va );
#else
	// Reading the spec, the size does seem correct. The man pages
	// say it will aways be null terminated (whereas the strcpy is not.)
	// Pretty nervous about the implementation, so force a null after.
    vsnprintf( buffer, size, format, va );
	buffer[size-1] = 0;
#endif
	va_end( va );

    TextOut( &shader, buffer, x, y, 0, 0 );
}
