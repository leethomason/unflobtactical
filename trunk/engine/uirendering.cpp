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
#include "surface.h"
#include "text.h"

using namespace grinliz;

const int XSTART = 0;
const int YSTART = 270;
const int SIZE = 50;

UIButtonBox::UIButtonBox( const Texture* texture )
{
	this->texture = texture;
	nIcons = 0;
}

void UIButtonBox::SetButtons( const int* _icons, const char** _text, int _nIcons )
{
	nIcons = _nIcons;
	for( int i=0; i<nIcons; ++i ) 
	{
		icons[i].id = _icons[i];
		icons[i].text[0] = 0;
		if ( _text && _text[i] ) {
			strncpy( icons[i].text, _text[i], MAX_TEXT_LEN );
		}

		int x = XSTART+i*SIZE;
		int y = YSTART;

		pos[i*4+0].Set( x,		y );		
		pos[i*4+1].Set( x+SIZE, y );		
		pos[i*4+2].Set( x+SIZE, y+SIZE );		
		pos[i*4+3].Set( x,		y+SIZE );		

		int id = icons[i].id;
		float dx = (float)(id%4)*0.25f;
		float dy = (id/4)*0.25f;

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

		if ( _text && _text[i] ) {
			int w, h;
			UFOText::GlyphSize( _text[i], &w, &h );
			icons[i].textPos.x = x + SIZE/2 - w/2;
			icons[i].textPos.y = y + SIZE/2 - h/2;
		}
	}
}


void UIButtonBox::Draw( int width, int height, int rotation )
{
	if ( nIcons == 0 )
		return;

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisableClientState( GL_NORMAL_ARRAY );

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture->glID );

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();					// save projection
	glLoadIdentity();				// projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();					// model
	glLoadIdentity();				// model

	glRotatef( 90.0f * (float)rotation, 0.0f, 0.0f, 1.0f );
#ifdef USING_ES
	glOrthof( 0.f, (float)width, 0.f, (float)height, -1.0f, 1.0f );
#else
	glOrtho( 0, width, 0, height, -1, 1 );
#endif

	glVertexPointer(   2, GL_SHORT, 0, pos );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex );  
	CHECK_GL_ERROR;
	glDrawElements( GL_TRIANGLES, nIcons*6, GL_UNSIGNED_SHORT, index );
	CHECK_GL_ERROR;
		

	glPopMatrix();					// model
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();					// projection
	glMatrixMode(GL_MODELVIEW);

	glEnableClientState( GL_NORMAL_ARRAY );
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );

	UFOText::Begin();
	for( int i=0; i<nIcons; ++i ) {
		if ( icons[i].text[0] ) {
			UFOText::Stream( icons[i].textPos.x, icons[i].textPos.y, "%s", icons[i].text );
		}
	}
	UFOText::End();
}


int UIButtonBox::QueryTap( int x, int y )
{
	int dx = ( x - XSTART ) / SIZE;
	int dy = ( y - YSTART ) / SIZE;

	if ( dy == 0 && dx >= 0 && dx < nIcons )
		return dx;
	return -1;
}
