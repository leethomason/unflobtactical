#include "text.h"
#include "platformgl.h"

static U32 textureID = 0;
const float CONSOLE_XSCALE = 2.0f / (float)(CONSOLE_WIDTH);
const float CONSOLE_YSCALE = 2.0f / (float)(CONSOLE_HEIGHT);
const int STDFONT_ROW = 6;
const int STDFONT_COL = 16;
const float STDFONT_WIDTH = 10;
const float STDFONT_HEIGHT = 20;


void UFOInitDrawText( U32 textTextureID )
{
	textureID = textTextureID;
}

void UFOTextOut( const char* str, int x, int _y )
{
	GLASSERT( textureID );
	if ( !textureID )
		return;

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_ALPHA_TEST );
	glAlphaFunc( GL_GREATER, 0.3f );

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();					// save projection
	glLoadIdentity();				// projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();					// model
	glLoadIdentity();				// model
	glTranslatef(-1.0f, -1.0f, 0.0f);

	glScalef( CONSOLE_XSCALE, CONSOLE_YSCALE, 1.0f );
	int y = CONSOLE_HEIGHT - _y - 1;

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

			t[0] = tx0;			t[1] = ty0;
			v[0] = (float)x;	v[1] = (float)y;
		
			t[2] = tx1;			t[3] = ty0;
			v[2] = float(x+1);	v[3] = (float)y;

			t[4] = tx1;			t[5] = ty1;
			v[4] = float(x+1);	v[5] = (float)(y+1);

			t[6] = tx0;			t[7] = ty1;
			v[6] = (float)x;	v[7] = float(y+1);

			glVertexPointer( 2, GL_FLOAT, 0, v );
			glTexCoordPointer( 2, GL_FLOAT, 0, t );
			glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
		}
		++str;
		++x;
	}
	glEnd();


	glPopMatrix();					// model
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();					// projection
	glMatrixMode(GL_MODELVIEW);

	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glDisable( GL_ALPHA_TEST );
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
