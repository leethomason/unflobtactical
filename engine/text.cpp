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

const int ADVANCE = 10;
const int GLYPH_CX = 16;
const int GLYPH_CY = 8;
static int GLYPH_WIDTH = 256 / GLYPH_CX;
static int GLYPH_HEIGHT = 128 / GLYPH_CY;


// 0 screen up
// 1 90 degree turn positive y, etc.

//int UFOText::width = 0;
//int UFOText::height = 0;
//int UFOText::rotation = 0;
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

	screenport.PushView();
	glBindTexture( GL_TEXTURE_2D, textureID );
}


void UFOText::End()
{
	glEnableClientState( GL_NORMAL_ARRAY );
	screenport.PopView();

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
}


void UFOText::TextOut( const char* str, int x, int y )
{
	float v[8];
	float t[8];

	while( *str )
	{
		int c = *str - 32;
		if ( c >= 1 && c < 128-32 )
		{
			float tx0 = ( 1.0f / float( GLYPH_CX ) ) * ( c % GLYPH_CX );
			float tx1 = tx0 + ( 1.0f / float( GLYPH_CX ) );

			float ty0 = 1.0f - ( 1.0f / float( GLYPH_CY ) ) * (float)( c / GLYPH_CX + 1 );
			float ty1 = ty0 + ( 1.0f / float( GLYPH_CY ) );

			t[0] = tx0;						t[1] = ty0;
			v[0] = (float)x;				v[1] = (float)y;
		
			t[2] = tx1;						t[3] = ty0;
			v[2] = (float)(x+GLYPH_WIDTH);	v[3] = (float)y;

			t[4] = tx1;						t[5] = ty1;
			v[4] = (float)(x+GLYPH_WIDTH);	v[5] = (float)(y+GLYPH_HEIGHT);

			t[6] = tx0;						t[7] = ty1;
			v[6] = (float)x;				v[7] = (float)(y+GLYPH_HEIGHT);

			glVertexPointer( 2, GL_FLOAT, 0, v );
			glTexCoordPointer( 2, GL_FLOAT, 0, t );

			glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
		}
		++str;
		x += ADVANCE;
	}
}


void UFOText::GlyphSize( const char* str, int* width, int* height )
{
	*width = 0;
	*height = 0;

	int len = strlen( str );
	if ( len == 0 ) {
		return;
	}
	*width = GLYPH_WIDTH + ADVANCE*(len-1);
	*height = GLYPH_HEIGHT;
}


void UFOText::Draw( int x, int y, const char* format, ... )
{
	Begin();

    va_list     va;
    char		buffer[1024];

    //
    //  format and output the message..
    //
    va_start( va, format );
    vsprintf( buffer, format, va );
    va_end( va );

    TextOut( buffer, x, y );
	End();
}


void UFOText::Stream( int x, int y, const char* format, ... )
{
    va_list     va;
    char		buffer[1024];

    //
    //  format and output the message..
    //
    va_start( va, format );
    vsprintf( buffer, format, va );
    va_end( va );

    TextOut( buffer, x, y );
}
