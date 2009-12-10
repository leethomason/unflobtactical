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

const float ALPHA_DISABLED	= 0.3f;
const float ALPHA_DECO		= 0.5f;


UIButtons::UIButtons( const Screenport& port ) : screenport( port )
{
	this->texture     = TextureManager::Instance()->GetTexture( "icons" );
	this->decoTexture = TextureManager::Instance()->GetTexture( "iconDeco" );
	nIcons = 0;

	const int SIZE = 50;
	const int PAD = 5;
	cacheValid = false;

	origin.Set( 0, 0 );
	size.Set( SIZE, SIZE );

	pad.Set( PAD, PAD );
	alpha = 1.0f;

	memset( icons, 0, sizeof( Icon )*MAX_ICONS );
}


UIButtonBox::UIButtonBox( const Screenport& port ) : UIButtons( port )
{
	columns = 1;
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


void UIButtons::InitButtons( const int* _icons, int _nIcons )
{
	nIcons = _nIcons;
	GLASSERT( nIcons <= MAX_ICONS );

	for( int i=0; i<nIcons; ++i ) {
		icons[i].id = _icons[i];
		icons[i].decoID = DECO_NONE;
		icons[i].enabled = true;
		icons[i].text0[0] = 0;
		icons[i].text1[0] = 0;
	}

	cacheValid = false;
}


void UIButtons::SetButton( int index, int iconID )
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


void UIButtons::SetDeco( int index, int decoID )
{
	GLASSERT( index < nIcons );
	if ( icons[index].decoID != decoID ) {
		icons[index].decoID = decoID;
		cacheValid = false;
	}
}


void UIButtons::SetText( const char** text ) 
{
	for( int i=0; i<nIcons; ++i ) {
		icons[i].text0[0] = 0;
		icons[i].text1[0] = 0;
	}
	if ( text ) {
		for( int i=0; i<nIcons; ++i ) {
			if ( text[i] ) {
				strncpy( icons[i].text0, text[i], MAX_TEXT_LEN );
				PositionText( i );
			}
		}
	}
	// Do NOT invalidate the cache. Just a text change.
}


void UIButtons::SetText( int index, const char* text ) 
{
	GLASSERT( index >=0 && index < nIcons );
	icons[index].text0[0] = 0;
	icons[index].text1[0] = 0;
	if ( text && *text ) {
		strncpy( icons[index].text0, text, MAX_TEXT_LEN );
		PositionText( index );
	}
}


void UIButtons::SetText( int index, const char* text0, const char* text1 )
{
	GLASSERT( index >=0 && index < nIcons );
	icons[index].text0[0] = 0;
	icons[index].text1[0] = 0;

	if ( text0 && *text0 ) {
		strncpy( icons[index].text0, text0, MAX_TEXT_LEN );
		PositionText( index );
	}
	if ( text1 && *text1 ) {
		strncpy( icons[index].text1, text1, MAX_TEXT_LEN );
		PositionText( index );
	}
}


const char* UIButtons::GetText( int index )
{
	GLASSERT( index >= 0 && index < nIcons );
	return icons[index].text0;
}


void UIButtons::PositionText( int index ) 
{
	int w, h;
	UFOText::GlyphSize( icons[index].text0, &w, &h );

	icons[index].textPos0.x = size.x/2 - w/2;
	icons[index].textPos0.y = size.y/2 - h/2;

	if ( icons[index].text1[0] ) {
		int w1, h1;
		UFOText::GlyphSize( icons[index].text1, &w1, &h1 );

		int s = size.y - (h+h1);
		
		icons[index].textPos0.y = size.y - s/2 - h;

		icons[index].textPos1.x = size.x/2 - w1/2;
		icons[index].textPos1.y = size.y - s/2 - (h+h1);
	}
}


void UIButtons::SetEnabled( int index, bool enabled )
{
	GLASSERT( index >=0 && index < nIcons );
	if ( icons[index].enabled != enabled ) {
		icons[index].enabled = enabled;
		cacheValid = false;
	}
}


void UIButtonBox::CalcButtons()
{
	if ( cacheValid )
		return;

	// Don't apply the origin here. It is applied at render.
	int row = 0;
	int col = 0;
	const float iconTX = 1.0f / (float)ICON_DX;
	const float iconTY = 1.0f / (float)ICON_DY;
	const float decoTX = 1.0f / (float)DECO_DX;
	const float decoTY = 1.0f / (float)DECO_DY;
	
	bounds.Set( 0, 0, 0, 0 );

	for( int i=0; i<nIcons; ++i ) 
	{
		int x = col*size.x + col*pad.x;
		int y = row*size.y + row*pad.x;

		pos[i*4+0].Set( x,			y );		
		pos[i*4+1].Set( x+size.x,	y );		
		pos[i*4+2].Set( x+size.x,	y+size.y );		
		pos[i*4+3].Set( x,			y+size.y );	
		bounds.DoUnion( x, y );
		bounds.DoUnion( x+size.x, y+size.y );

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


void UIButtons::Draw()
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
		float c = icons[i].enabled ? 1.0f : ALPHA_DISABLED;
		glColor4f( c, c, c, 1.0f );

		if ( icons[i].text0[0] ) {
			int x = pos[i*4].x + origin.x;
			int y = pos[i*4].y + origin.y;
			UFOText::Stream( x+icons[i].textPos0.x, y+icons[i].textPos0.y, "%s", icons[i].text0 );
		}
		if ( icons[i].text1[0] ) {
			int x = pos[i*4].x + origin.x;
			int y = pos[i*4].y + origin.y;
			UFOText::Stream( x+icons[i].textPos1.x, y+icons[i].textPos1.y, "%s", icons[i].text1 );
		}
	}
	UFOText::End();
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
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


UIButtonGroup::UIButtonGroup( const Screenport& port ) : UIButtons( port )
{
	memset( bPos, 0, sizeof(Vector2I)*MAX_ICONS );
}


void UIButtonGroup::SetPos( int index, int x, int y )
{
	GLASSERT( index >= 0 && index < nIcons );
	bPos[index].Set( x, y );
}


int UIButtonGroup::QueryTap( int x, int y )
{
	Rectangle2I b;
	Vector2I p ={ x, y };

	for( int i=0; i<nIcons; ++i ) {
		b.Set( bPos[i].x,
					bPos[i].y,
					bPos[i].x + size.x,
					bPos[i].y + size.y );
		if ( b.Contains( p ) ) {
			return i;
		}
	}
	return -1;
}


void UIButtonGroup::CalcButtons()
{
	if ( cacheValid )
		return;

	// Don't apply the origin here. It is applied at render.
	const float iconTX = 1.0f / (float)ICON_DX;
	const float iconTY = 1.0f / (float)ICON_DY;
	const float decoTX = 1.0f / (float)DECO_DX;
	const float decoTY = 1.0f / (float)DECO_DY;
	
	bounds.Set( 0, 0, 0, 0 );

	for( int i=0; i<nIcons; ++i ) 
	{
		int x = bPos[i].x;
		int y = bPos[i].y;

		pos[i*4+0].Set( x,			y );		
		pos[i*4+1].Set( x+size.x,	y );		
		pos[i*4+2].Set( x+size.x,	y+size.y );		
		pos[i*4+3].Set( x,			y+size.y );	
		bounds.DoUnion( x, y );
		bounds.DoUnion( x+size.x, y+size.y );

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
	}
	cacheValid = true;
}


/*static*/ void UIRendering::DrawQuad(	const Screenport& screenport,
										const grinliz::Rectangle2I& pos, 
										const grinliz::Rectangle2F& uv,
										const Texture* texture )
{
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisableClientState( GL_NORMAL_ARRAY );
	//glEnableClientState( GL_COLOR_ARRAY );

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture->glID );

	screenport.PushUI();

	grinliz::Vector2< S16 > p[4] = {
		{ pos.min.x, pos.min.y },
		{ pos.max.x, pos.min.y },
		{ pos.max.x, pos.max.y },
		{ pos.min.x, pos.max.y }
	};
	grinliz::Vector2F tex[4] = {
		{ uv.min.x, uv.min.y },
		{ uv.max.x, uv.min.y },
		{ uv.max.x, uv.max.y },
		{ uv.min.x, uv.max.y }
	};

	glVertexPointer(   2, GL_SHORT, 0, p );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex ); 

	CHECK_GL_ERROR;
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
	CHECK_GL_ERROR;

	screenport.PopUI();

	glEnableClientState( GL_NORMAL_ARRAY );
	//glDisableClientState( GL_COLOR_ARRAY );
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
}


/*static*/ void UIRendering::GetDecoUV( int decoID, grinliz::Rectangle2F* uv )
{
	const float decoTX = 1.0f / (float)UIButtonBox::DECO_DX;
	const float decoTY = 1.0f / (float)UIButtonBox::DECO_DY;

	float dx = (float)(decoID%UIButtonBox::DECO_DX)*decoTX;
	float dy = (float)(decoID/UIButtonBox::DECO_DX)*decoTY;

	uv->Set( dx, dy, dx+decoTX,	dy+decoTY );
}

