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

#include "uirendering.h"
#include "platformgl.h"
using namespace grinliz;

const int XSTART = 0;
const int YSTART = 270;
const int SIZE = 50;

UIWidgets::UIWidgets()
{
	for( int i=0; i<ICON_COUNT; ++i ) {
		pos[i*4+0].Set( XSTART+i*SIZE,		YSTART );		
		pos[i*4+1].Set( XSTART+i*SIZE+SIZE, YSTART );		
		pos[i*4+2].Set( XSTART+i*SIZE+SIZE, YSTART+SIZE );		
		pos[i*4+3].Set( XSTART+i*SIZE,		YSTART+SIZE );		

		float dx = (float)(i%4)*0.25f;
		float dy = (i/4)*0.25f;

		tex[i*4+0].Set( dx,			dy );
		tex[i*4+1].Set( dx+0.25f,	dy );
		tex[i*4+2].Set( dx+0.25f,	dy+0.25f );
		tex[i*4+3].Set( dx,			dy+0.25f );
		
		U16 idx = i*4;
		index[i*6+0] = idx+0;
		index[i*6+1] = idx+1;
		index[i*6+2] = idx+2;

		index[i*6+3] = idx+0;
		index[i*6+4] = idx+2;
		index[i*6+5] = idx+3;
	}
}


void UIWidgets::Draw( int width, int height, int rotation )
{
	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisableClientState( GL_NORMAL_ARRAY );

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();					// save projection
	glLoadIdentity();				// projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();					// model
	glLoadIdentity();				// model

	glRotatef( 90.0f * (float)rotation, 0.0f, 0.0f, 1.0f );
#ifdef __APPLE__
	glOrthof( 0.f, (float)width, 0.f, (float)height, -1.0f, 1.0f );
#else
	glOrtho( 0, width, 0, height, -1, 1 );
#endif

	glVertexPointer(   2, GL_SHORT, 0, pos );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex );  
	CHECK_GL_ERROR;
	glDrawElements( GL_TRIANGLES, ICON_COUNT*6, GL_UNSIGNED_SHORT, index );
	CHECK_GL_ERROR;
		

	glPopMatrix();					// model
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();					// projection
	glMatrixMode(GL_MODELVIEW);

	glEnableClientState( GL_NORMAL_ARRAY );
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
}


int UIWidgets::QueryTap( int x, int y )
{
	int dx = ( x - XSTART ) / SIZE;
	int dy = ( y - YSTART ) / SIZE;

	if ( dy == 0 && dx >= 0 && dx < ICON_COUNT )
		return dx;
	return ICON_COUNT;
}
