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
#include "../grinliz/glutil.h"

static U32 textureID = 0;

const int ADVANCE = 10;
const int GLYPH_CX = 16;
const int GLYPH_CY = 8;
static int GLYPH_WIDTH = 256 / GLYPH_CX;
static int GLYPH_HEIGHT = 128 / GLYPH_CY;
static int width;
static int height;

// 0 screen up
// 1 90 degree turn positive y, etc.
static int rotation = 0;

void UFOInitDrawText( U32 textTextureID, int w, int h, int r )
{
	if ( textTextureID ) {
		textureID = textTextureID;
	}
	else {
		GLASSERT( textureID );
	}

	width = w;
	height = h;

	if ( r&0x01 ) {
		grinliz::Swap( &width, &height );
	}

	rotation = r&0x03;

}

void UFOTextOut( const char* str, int x, int y )
{
	GLASSERT( textureID );
	if ( !textureID )
		return;

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisableClientState( GL_NORMAL_ARRAY );

	glColor4f( 1.f, 1.f, 1.f, 1.f );

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();					// save projection
	glLoadIdentity();				// projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();					// model
	glLoadIdentity();				// model

	glRotatef( 90.0f * (float)rotation, 0.0f, 0.0f, 1.0f );
	//glTranslatef(-1.0f, -1.0f, 0.0f);
	//glScalef( 2.0f/(float)width, 2.0f/(float)height, 1.0f );
#ifdef USING_ES
	glOrthof( 0.f, (float)width, 0.f, (float)height, -1.f, 1.f );
#else
	glOrtho( 0, width, 0, height, -1, 1 );
#endif

	glBindTexture( GL_TEXTURE_2D, textureID );

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
	glEnableClientState( GL_NORMAL_ARRAY );


	glPopMatrix();					// model
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();					// projection
	glMatrixMode(GL_MODELVIEW);

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
}

void UFODrawText( int x, int y, const char* format, ... )
{
    va_list     va;
    char		buffer[1024];

    //
    //  format and output the message..
    //
    va_start( va, format );
    vsprintf( buffer, format, va );
    va_end( va );

    UFOTextOut( buffer, x, y );
}
