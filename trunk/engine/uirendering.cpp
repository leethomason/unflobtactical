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


UIButtonBox::UIButtonBox( const Texture* texture, const Texture* decoTexture, const Screenport& port ) : screenport( port )
{
	this->texture = texture;
	this->decoTexture = decoTexture;
	nIcons = 0;

	const int SIZE = 50;
	const int PAD = 5;
	cacheValid = false;

	origin.Set( 0, 0 );
	size.Set( SIZE, SIZE );
	columns = 1;
	pad.Set( PAD, PAD );
	alpha = 1.0f;

	memset( icons, 0, sizeof( Icon )*MAX_ICONS );
}


void UIButtonBox::CalcDimensions( int *x, int *y, int *w, int *h )
{
	if ( x ) 
		*x = origin.x;
	if ( y )
		*y = origin.y;
	int rows = nIcons / columns;

	if ( w ) {
		if ( columns > 1 )
			*w = columns*size.x + (columns-1)*pad.x;
		else
			*w = size.x;
	}
	if ( h ) {
		if ( rows > 1 )
			*h = rows*size.y + (rows-1)*pad.y;
		else
			*h = rows*size.y;
	}
}


void UIButtonBox::InitButtons( const int* _icons, int _nIcons )
{
	nIcons = _nIcons;
	GLASSERT( nIcons <= MAX_ICONS );

	for( int i=0; i<nIcons; ++i ) {
		icons[i].id = _icons[i];
		icons[i].decoID = DECO_NONE;
		icons[i].enabled = true;
		icons[i].text[0] = 0;
	}

	cacheValid = false;
}


void UIButtonBox::SetButton( int index, int iconID )
{
	GLASSERT( index < MAX_ICONS );
	if ( index >= nIcons ) {
		nIcons = index+1;
		cacheValid = false;
	}

	if ( icons[index].id != iconID ) {
		icons[index].id = iconID;
		cacheValid = false;
	}
}


void UIButtonBox::SetDeco( int index, int decoID )
{
	GLASSERT( index < nIcons );
	if ( icons[index].decoID != decoID ) {
		icons[index].decoID = decoID;
		cacheValid = false;
	}
}


void UIButtonBox::SetText( const char** text ) 
{
	for( int i=0; i<nIcons; ++i ) {
		icons[i].text[0] = 0;
	}
	if ( text ) {
		for( int i=0; i<nIcons; ++i ) {
			if ( text[i] ) {
				strncpy( icons[i].text, text[i], MAX_TEXT_LEN );
				PositionText( i );
			}
		}
	}
	// Do NOT invalidate the cache. Just a text change.
}


void UIButtonBox::SetText( int index, const char* text ) 
{
	GLASSERT( index >=0 && index < nIcons );
	icons[index].text[0] = 0;
	if ( text && *text ) {
		strncpy( icons[index].text, text, MAX_TEXT_LEN );
		PositionText( index );
	}
}


void UIButtonBox::PositionText( int index ) 
{
	int w, h;
	UFOText::GlyphSize( icons[index].text, &w, &h );
	icons[index].textPos.x = size.x/2 - w/2;
	icons[index].textPos.y = size.y/2 - h/2;
}


void UIButtonBox::SetEnabled( int index, bool enabled )
{
	GLASSERT( index >=0 && index < nIcons );
	if ( icons[index].enabled != enabled ) {
		icons[index].enabled = enabled;
		cacheValid = false;
	}
}


void UIButtonBox::CalcButtons()
{
	// Don't apply the origin here. It is applied at render.
	int row = 0;
	int col = 0;
	const float iconTX = 1.0f / (float)ICON_DX;
	const float iconTY = 1.0f / (float)ICON_DY;
	const float decoTX = 1.0f / (float)DECO_DX;
	const float decoTY = 1.0f / (float)DECO_DY;
	const float ALPHA_DISABLED	= 0.3f;
	const float ALPHA_DECO		= 0.5f;
	
	for( int i=0; i<nIcons; ++i ) 
	{
		int x = col*size.x + col*pad.x;
		int y = row*size.y + row*pad.x;

		pos[i*4+0].Set( x,			y );		
		pos[i*4+1].Set( x+size.x,	y );		
		pos[i*4+2].Set( x+size.x,	y+size.y );		
		pos[i*4+3].Set( x,			y+size.y );		

		int id = icons[i].id;
		float dx = (float)(id%ICON_DX)*iconTX;
		float dy = (float)(id/ICON_DX)*iconTY;

		tex[i*4+0].Set( dx,			dy );
		tex[i*4+1].Set( dx+iconTX,	dy );
		tex[i*4+2].Set( dx+iconTX,	dy+iconTY );
		tex[i*4+3].Set( dx,			dy+iconTY );

		int decoID = icons[i].decoID;
		dx = (float)(decoID%DECO_DX)*decoTX;
		dy = (float)(decoID/DECO_DX)*decoTY;

		texDeco[i*4+0].Set( dx,			dy );
		texDeco[i*4+1].Set( dx+decoTX,	dy );
		texDeco[i*4+2].Set( dx+decoTX,	dy+decoTY );
		texDeco[i*4+3].Set( dx,			dy+decoTY );

		Vector4<U8> c0 = { 255, 255, 255, (U8)LRintf( alpha*255.0f ) };
		Vector4<U8> c1 = { 255, 255, 255, (U8)LRintf( alpha*255.0f*ALPHA_DECO ) };
		if ( !icons[i].enabled ) {
			c0.Set( 255, 255, 255, (U8)LRintf( alpha*255.0f*ALPHA_DISABLED ) );
			c1.Set( 255, 255, 255, (U8)LRintf( alpha*255.0f*ALPHA_DISABLED*ALPHA_DECO ) );
		}
		color[i*4+0] = color[i*4+1] = color[i*4+2] = color[i*4+3] = c0;
		colorDeco[i*4+0] = colorDeco[i*4+1] = colorDeco[i*4+2] = colorDeco[i*4+3] = c1;
		
		U16 idx = i*4;
		index[i*6+0] = idx+0;
		index[i*6+1] = idx+1;
		index[i*6+2] = idx+2;

		index[i*6+3] = idx+0;
		index[i*6+4] = idx+2;
		index[i*6+5] = idx+3;

		col++;
		if ( col >= columns ) {
			col = 0;
			++row;
		}
	}
	cacheValid = true;
}


void UIButtonBox::Draw()
{
	if ( nIcons == 0 )
		return;
	if (!cacheValid)
		CalcButtons();

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture->glID );

	screenport.PushUI();
	glTranslatef( (float)origin.x, (float)origin.y, 0.0f );

	glVertexPointer(   2, GL_SHORT, 0, pos );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex ); 
	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, color );

	CHECK_GL_ERROR;
	glDrawElements( GL_TRIANGLES, nIcons*6, GL_UNSIGNED_SHORT, index );
	CHECK_GL_ERROR;
		
	glTexCoordPointer( 2, GL_FLOAT, 0, texDeco ); 
	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorDeco );
	glBindTexture( GL_TEXTURE_2D, decoTexture->glID );
	CHECK_GL_ERROR;
	glDrawElements( GL_TRIANGLES, nIcons*6, GL_UNSIGNED_SHORT, index );
	CHECK_GL_ERROR;

	screenport.PopUI();

	glEnableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );

	UFOText::Begin();
	for( int i=0; i<nIcons; ++i ) {
		if ( icons[i].text[0] ) {
			int x = pos[i*4].x + origin.x;
			int y = pos[i*4].y + origin.y;
			UFOText::Stream( x+icons[i].textPos.x, y+icons[i].textPos.y, "%s", icons[i].text );
		}
	}
	UFOText::End();
}


int UIButtonBox::QueryTap( int x, int y )
{
	int dx = ( x - origin.x ) / (size.x + pad.x);
	int dy = ( y - origin.y ) / (size.y + pad.y);

	if ( dx >= 0 && dx < columns && dy >=0 ) {

		// local coordinates:
		int buttonX = x - origin.x - dx*(size.x+pad.x);
		int buttonY = y - origin.y - dy*(size.y+pad.y);

		if (    buttonX >= 0 && buttonX < size.x
			 && buttonY >= 0 && buttonY < size.y ) {

			int icon = dy * columns + dx;

			if ( icon >=0 && icon < nIcons && icons[icon].enabled ) 
			{
				return icon;
			}
		}
	}
	return -1;
}