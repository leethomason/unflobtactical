#include "text.h"
#include "platformgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "../grinliz/glutil.h"

static U32 textureID = 0;

// 			Width	Height
//	Screen	480		320
//	Glyph	 16		 16
static float CONSOLE_WIDTH  = 480.f/16.f;
static float CONSOLE_HEIGHT = 320.f/16.f;
static float CONSOLE_ADVANCE = 0.55f;
static float CONSOLE_XSCALE = 2.0f / CONSOLE_WIDTH;
static float CONSOLE_YSCALE = 2.0f / CONSOLE_HEIGHT;

static const int STDFONT_ROW = 8;	//6;
static const int STDFONT_COL = 16;

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

	CONSOLE_WIDTH  = w/16.f;
	CONSOLE_HEIGHT = h/16.f;

	if ( r&0x01 ) {
		grinliz::Swap( &CONSOLE_WIDTH, &CONSOLE_HEIGHT );
	}

	CONSOLE_ADVANCE = 0.55f;
	CONSOLE_XSCALE = 2.0f / CONSOLE_WIDTH;
	CONSOLE_YSCALE = 2.0f / CONSOLE_HEIGHT;
	rotation = r&0x03;

}

void UFOTextOut( const char* str, int x, int _y )
{
	GLASSERT( textureID );
	if ( !textureID )
		return;

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f( 1.f, 1.f, 1.f, 1.f );

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();					// save projection
	glLoadIdentity();				// projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();					// model
	glLoadIdentity();				// model

	glRotatef( 90.0f * (float)rotation, 0.0f, 0.0f, 1.0f );
	glTranslatef(-1.0f, -1.0f, 0.0f);

	glScalef( CONSOLE_XSCALE, CONSOLE_YSCALE, 1.0f );
	float y = CONSOLE_HEIGHT - (float)_y - 1.f;

	glBindTexture( GL_TEXTURE_2D, textureID );

	float v[8];
	float t[8];

	while( *str )
	{
		int c = *str - 32;
		if ( c >= 1 && c < 128-32 )
		{
			float tx0 = ( 1.0f / float( STDFONT_COL ) ) * ( c % STDFONT_COL );
			float ty0 = 1.0f - ( 1.0f / float( STDFONT_ROW ) ) * ( c / STDFONT_COL + 1 );
			float tx1 = tx0 + ( 1.0f / float( STDFONT_COL ) );
			float ty1 = ty0 + ( 1.0f / float( STDFONT_ROW ) );

			float xa = (float)x * CONSOLE_ADVANCE;

			t[0] = tx0;			t[1] = ty0;
			v[0] = xa;			v[1] = y;
		
			t[2] = tx1;			t[3] = ty0;
			v[2] = xa+1.f;		v[3] = y;

			t[4] = tx1;			t[5] = ty1;
			v[4] = xa+1.f;		v[5] = y+1.f;

			t[6] = tx0;			t[7] = ty1;
			v[6] = xa;			v[7] = y+1.f;

			glVertexPointer( 2, GL_FLOAT, 0, v );
			glTexCoordPointer( 2, GL_FLOAT, 0, t );
			glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
		}
		++str;
		++x;
	}


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
