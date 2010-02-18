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
#include "platformgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "../grinliz/glutil.h"

using namespace grinliz;

const int ADVANCE = 10;
const int ADVANCE_SMALL = 8;
const float SMALL_SCALE = 0.75f;
const int GLYPH_CX = 16;
const int GLYPH_CY = 8;
static int GLYPH_WIDTH = 256 / GLYPH_CX;
static int GLYPH_HEIGHT = 128 / GLYPH_CY;

extern int trianglesRendered;	// FIXME: should go away once all draw calls are moved to the enigine
extern int drawCalls;			// ditto


Vector2F UFOText::vBuf[BUF_SIZE*4];
Vector2F UFOText::tBuf[BUF_SIZE*4];
U16 UFOText::iBuf[BUF_SIZE*6] = { 0, 0 };

Screenport UFOText::screenport( 320, 480, 1 );
U32 UFOText::textureID = 0;

void UFOText::InitScreen( const Screenport& sp )
{
	screenport = sp;
}


void UFOText::InitTexture( U32 textTextureID )
{
	GLASSERT( textTextureID );
	textureID = textTextureID;
}


void UFOText::Begin()
{
	GLASSERT( textureID );

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisableClientState( GL_NORMAL_ARRAY );

	glColor4f( 1.f, 1.f, 1.f, 1.f );

	screenport.PushUI();
	glBindTexture( GL_TEXTURE_2D, textureID );
}


void UFOText::End()
{
	glEnableClientState( GL_NORMAL_ARRAY );
	screenport.PopUI();

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
}


void UFOText::TextOut( const char* str, int x, int y, int* w, int *h )
{
	bool smallText = false;
	bool rendering = true;
	int xStart = x;

	if ( w ) {
		GLASSERT( h );
		*w = 0;
		*h = GLYPH_HEIGHT;
		rendering = false;
	}

	if ( rendering ) {
		glVertexPointer( 2, GL_FLOAT, 0, vBuf );
		glTexCoordPointer( 2, GL_FLOAT, 0, tBuf );

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
		if ( *str == '.' ) {
			smallText = true;
			++str;
			continue;
		}
		else 
		if ( *str == ' ' ) {
			if ( smallText ) {
				x += ADVANCE_SMALL;
				smallText = false;
			}
			else {
				x += ADVANCE;
			}
			++str;
			continue;
		}

		if ( smallText && !( *str >= '0' && *str <= '9' ) ) {
			smallText = false;
		}

		// Draw a glyph or nothing, at this point:
		GLASSERT( pos < BUF_SIZE );
		int c = *str - 32;
		if ( c >= 1 && c < 128-32 )
		{
			if ( rendering ) {
				float tx0 = ( 1.0f / float( GLYPH_CX ) ) * ( c % GLYPH_CX );
				float tx1 = tx0 + ( 1.0f / float( GLYPH_CX ) );

				float ty0 = 1.0f - ( 1.0f / float( GLYPH_CY ) ) * (float)( c / GLYPH_CX + 1 );
				float ty1 = ty0 + ( 1.0f / float( GLYPH_CY ) );

				tBuf[pos*4+0].Set( tx0, ty0 );
				tBuf[pos*4+1].Set( tx1, ty0 );
				tBuf[pos*4+2].Set( tx1, ty1 );
				tBuf[pos*4+3].Set( tx0, ty1 );
			}
			if ( !smallText ) {
				if ( rendering ) {
					vBuf[pos*4+0].Set( (float)x,					(float)y );	
					vBuf[pos*4+1].Set( (float)(x+GLYPH_WIDTH),	(float)y );	
					vBuf[pos*4+2].Set( (float)(x+GLYPH_WIDTH),	(float)(y+GLYPH_HEIGHT) );	
					vBuf[pos*4+3].Set( (float)x,					(float)(y+GLYPH_HEIGHT) );	
				}
				x += ADVANCE;
			}
			else {
				if ( rendering ) {
					float y0 = (float)(y+GLYPH_HEIGHT) - (float)GLYPH_HEIGHT * SMALL_SCALE;
					float x1 = (float)x + (float)GLYPH_WIDTH*SMALL_SCALE;

					vBuf[pos*4+0].Set( (float)x,					y0 );
					vBuf[pos*4+1].Set( x1,						y0 );
					vBuf[pos*4+2].Set( x1,						(float)(y+GLYPH_HEIGHT) );
					vBuf[pos*4+3].Set( (float)x,					(float)(y+GLYPH_HEIGHT) );
				}
				x += ADVANCE_SMALL;
			}

			if ( rendering ) {
				pos++;
				if ( pos == BUF_SIZE || *(str+1) == 0 ) {
					glDrawElements( GL_TRIANGLES, pos*6, GL_UNSIGNED_SHORT, iBuf );

					trianglesRendered += pos*3;
					drawCalls++;

					pos = 0;
				}
			}
		}
		
		++str;
	}
	if ( w ) {
		*w = x - xStart;
		if ( *w > 0 )
			*w += GLYPH_WIDTH-ADVANCE;
	}
}


void UFOText::GlyphSize( const char* str, int* width, int* height )
{
	TextOut( str, 0, 0, width, height );
}


/*
void UFOText::GlyphSize( const char* str, int* width, int* height )
{
	*width = 0;
	*height = 0;

	if ( str && *str ) {
		*height = GLYPH_HEIGHT;
	}
	bool small = false;
	bool anyglyph = false;

	for( const char* p = str; p && *p; ++p ) {
		if ( *p == '.' ) {
			small = true;
		}
		else if ( small ) {
			if ( *p >= '0' && *p <= '9' ) {
				*width += ADVANCE_SMALL;
			}
			else {
				*width += ADVANCE;
				small = false;
				anyglyph = true;
			}
		}
		else {
			*width += ADVANCE;
			anyglyph = true;
		}
	}
	if ( anyglyph ) {
		*width += GLYPH_WIDTH - ADVANCE_SMALL;	// the advance is less than the width
	}
}
*/

void UFOText::Draw( int x, int y, const char* format, ... )
{
	Begin();

    va_list     va;
	const int	size = 1024;
    char		buffer[size];

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    int result = vsnprintf_s( buffer, size, _TRUNCATE, format, va );
#else
	// Reading the spec, the size does seem correct. The man pages
	// say it will aways be null terminated (whereas the strcpy is not.)
	// Pretty nervous about the implementation, so force a null after.
    int result = vsnprintf( buffer, size, format, va );
	buffer[size-1] = 0;
#endif
	va_end( va );

    TextOut( buffer, x, y, 0, 0 );
	End();
}


void UFOText::Stream( int x, int y, const char* format, ... )
{
    va_list     va;
	const int	size = 1024;
    char		buffer[size];

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    int result = vsnprintf_s( buffer, size, _TRUNCATE, format, va );
#else
	// Reading the spec, the size does seem correct. The man pages
	// say it will aways be null terminated (whereas the strcpy is not.)
	// Pretty nervous about the implementation, so force a null after.
    int result = vsnprintf( buffer, size, format, va );
	buffer[size-1] = 0;
#endif
	va_end( va );

    TextOut( buffer, x, y, 0, 0 );
}
