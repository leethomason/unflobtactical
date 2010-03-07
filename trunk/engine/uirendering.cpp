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
#include "../grinliz/glstringutil.h"

using namespace grinliz;

const float ALPHA_DISABLED	= 0.3f;
const float ALPHA_DECO		= 0.5f;
const int BIAS = 5;

extern int trianglesRendered;	// FIXME: should go away once all draw calls are moved to the enigine
extern int drawCalls;			// ditto


UIWidget::UIWidget( const Screenport& port ) : screenport( port )
{
	origin.Set( 0, 0 );
}



UIBar::UIBar( const Screenport& port ) : UIWidget( port )
{
	valid = false;
	nSteps = 10;
	minValue = 0;
	maxValue = 100;
	value0 = 50;
	value1 = 75;

	size.Set( 50, 10 );
	texture = TextureManager::Instance()->GetTexture( "basehp" );

	// 3 2     7 6
	// 0 1 ... 4 5
	for( int i=0; i<MAX_STEPS; ++i ) {
		index[i*6+0] = i*4 + 0;
		index[i*6+1] = i*4 + 1;
		index[i*6+2] = i*4 + 2;
		index[i*6+3] = i*4 + 0;
		index[i*6+4] = i*4 + 2;
		index[i*6+5] = i*4 + 3;
	}
}


UIBar::~UIBar()
{}


void UIBar::SetSize( int dx, int dy )
{
	if ( dx != size.x || dy != size.y ) {
		size.Set( dx, dy );
		valid = false;
	}
}


void UIBar::SetSteps( int steps )
{
	GLASSERT( steps <= MAX_STEPS && steps > 0 );
	if ( nSteps != steps ) {
		nSteps = steps;
		valid = false;
	}
}
	

void UIBar::SetRange( int min, int max )
{
	GLASSERT( max > min );
	if ( min != minValue || max != maxValue ) {
		minValue = min;
		maxValue = max;
		valid = false;
	}
}


void UIBar::SetValue0( int v0 )
{
	if ( v0 != value0 ) {
		value0 = v0;
		valid = false;
	}
}


void UIBar::SetValue1( int v1 )
{
	if ( v1 != value1 ) {
		value1 = v1;
		valid = false;
	}
}


void UIBar::Draw()
{
	if ( !valid ) {
		int range = maxValue - minValue;
		int round = range / nSteps - 1;
		int step0 = ( value0 - minValue + round ) * nSteps / range;
		int step1 = ( value1 - minValue + round ) * nSteps / range;

		for ( int i=0; i<nSteps; ++i ) {

			float fraction0 = (float)i / (float)nSteps;
			float fraction1 = (float)(i+1) / (float)nSteps;

			vertex[i*4+0].pos.Set( fraction0 * (float)size.x, 0.0f );
			vertex[i*4+1].pos.Set( fraction1 * (float)size.x, 0.0f );
			vertex[i*4+2].pos.Set( fraction1 * (float)size.x, (float)size.y );
			vertex[i*4+3].pos.Set( fraction0 * (float)size.x, (float)size.y );

			float tx = 0.0f;
			float ty = 0.0f;
			if ( i<step0 ) {
				// nothing
			}
			else if ( i<step1 ) {
				tx = 0.50f;
				ty = 0.0f;
			}
			else {
				tx = 0.0f;
				ty = 0.50f;
			}

			vertex[i*4+0].tex.Set( tx+0.0f, ty+0.0f );
			vertex[i*4+1].tex.Set( tx+0.5f, ty+0.0f );
			vertex[i*4+2].tex.Set( tx+0.5f, ty+0.5f );
			vertex[i*4+3].tex.Set( tx+0.0f, ty+0.5f );
		}
		valid = true;
	}

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
	glTranslatef( (float)origin.x, (float)origin.y, 0.0f );

	glVertexPointer(   2, GL_FLOAT, sizeof( Vertex2D ), &vertex[0].pos.x );
	glTexCoordPointer( 2, GL_FLOAT, sizeof( Vertex2D ), &vertex[0].tex.x ); 
	//glColorPointer( 4, GL_UNSIGNED_BYTE, 0, color );

	CHECK_GL_ERROR;
	glDrawElements( GL_TRIANGLES, 6*nSteps, GL_UNSIGNED_SHORT, index );
	trianglesRendered += 6*nSteps;
	drawCalls++;
	CHECK_GL_ERROR;
		
	screenport.PopUI();

	glEnableClientState( GL_NORMAL_ARRAY );
	//glDisableClientState( GL_COLOR_ARRAY );
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
}

	
UIImage::UIImage( const Screenport& port ) : UIWidget( port )
{
	w = h = 0;
	texture = 0;
	texCoord.Set( 0.0f, 0.0f, 1.0f, 1.0f );
	zRot = yRot = 0.0f;
}


UIImage::~UIImage()
{
}


void UIImage::Init( const Texture* texture, int w, int h )
{
	this->texture = texture;
	this->w = w;
	this->h = h;
}


void UIImage::Draw()
{
	if ( !texture || !w || !h )
		return;

	Vector2<S16>	pos[4] =	{ { 0, 0 }, { w, 0 }, { w, h }, { 0, h } };
	Vector2F		tex[4] =	{	{ texCoord.min.x, texCoord.min.y }, 
									{ texCoord.max.x, texCoord.min.y }, 
									{ texCoord.max.x, texCoord.max.y }, 
									{ texCoord.min.x, texCoord.max.y } };
	U16				index[6] =	{ 0, 1, 2, 0, 2, 3 };

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	if ( texture->alpha ) {
		glEnable( GL_BLEND );
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	glDisableClientState( GL_NORMAL_ARRAY );
	//glEnableClientState( GL_COLOR_ARRAY );

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture->glID );

	screenport.PushUI();
	// Over translate so rotation is about center axis.
	glTranslatef( (float)(origin.x+w/2), (float)(origin.y+h/2), 0.0f );
	// Don't want to fool around with rendering back faces, cheat on
	// rotation amount.
	float yPrime = yRot;
	if ( yPrime >= 270 )
		yPrime = yRot - 360.0f;
	else if ( yPrime >= 90 )
		yPrime = yRot - 180.0f;
	
	float zPrime = zRot;
	while( zPrime < 0 ) zPrime += 360.0f;
	while( zPrime >= 360.0f ) zPrime -= 360.0f;
	glRotatef( zPrime, 0.0f, 0.0f, 1.0f );

	glRotatef( yPrime, 0.0f, 1.0f, 0.0f );
	glTranslatef( (float)(-w/2), (float)(-h/2), 0.0f );

	glVertexPointer(   2, GL_SHORT, 0, pos );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex ); 
	//glColorPointer( 4, GL_UNSIGNED_BYTE, 0, color );

	CHECK_GL_ERROR;
	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, index );
	trianglesRendered += 2;
	drawCalls++;
	CHECK_GL_ERROR;
		
	screenport.PopUI();

	glEnableClientState( GL_NORMAL_ARRAY );
	//glDisableClientState( GL_COLOR_ARRAY );
	//glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
}


UIButtons::UIButtons( const Screenport& port ) : UIWidget( port )
{
	this->texture     = TextureManager::Instance()->GetTexture( "icons" );
	this->decoTexture = TextureManager::Instance()->GetTexture( "iconDeco" );
	nIcons = 0;

	const int SIZE = 50;
	const int PAD = 5;
	cacheValid = false;

	size.Set( SIZE, SIZE );

	pad.Set( PAD, PAD );
	alpha = 1.0f;

	memset( icons, 0, sizeof( Icon )*MAX_ICONS );
	textLayout = LAYOUT_CENTER;
}


UIButtonBox::UIButtonBox( const Screenport& port ) : UIButtons( port )
{
	columns = 1;
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

int UIButtons::GetButton( int index ) const
{
	GLASSERT( index < nIcons );
	return icons[index].id;
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
				StrNCpy( icons[i].text0, text[i], MAX_TEXT_LEN );
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
		StrNCpy( icons[index].text0, text, MAX_TEXT_LEN );
		PositionText( index );
	}
}


void UIButtons::SetText( int index, const char* text0, const char* text1 )
{
	GLASSERT( index >=0 && index < nIcons );
	icons[index].text0[0] = 0;
	icons[index].text1[0] = 0;

	if ( text0 && *text0 ) {
		StrNCpy( icons[index].text0, text0, MAX_TEXT_LEN );
		PositionText( index );
	}
	if ( text1 && *text1 ) {
		StrNCpy( icons[index].text1, text1, MAX_TEXT_LEN );
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
	int w, h, w1, h1;
	const int GUTTER = 6;

	UFOText::GlyphSize( icons[index].text0, &w, &h );
	w1 = w; h1 = h;

	icons[index].textPos0.y = size.y/2 - h/2;

	if ( icons[index].text1[0] ) {
		UFOText::GlyphSize( icons[index].text1, &w1, &h1 );

		int s = size.y - (h+h1);
		icons[index].textPos0.y = size.y - s/2 - h;
		icons[index].textPos1.y = size.y - s/2 - (h+h1);
	}

	if ( textLayout == LAYOUT_CENTER ) {
		icons[index].textPos0.x = size.x/2 - w/2;
		icons[index].textPos1.x = size.x/2 - w1/2;
	}
	else if ( textLayout == LAYOUT_LEFT ) {
		icons[index].textPos0.x = GUTTER;
		icons[index].textPos1.x = GUTTER;
	}
	else {
		icons[index].textPos0.x = size.x - w - GUTTER;
		icons[index].textPos1.x = size.x - w1 - GUTTER;
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


void UIButtons::SetHighLight( int index, bool highLight )
{
	GLASSERT( index >=0 && index < nIcons );
	if ( icons[index].highLight != highLight ) {
		icons[index].highLight = highLight;
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
		int bias = 0;
		if ( icons[i].highLight )
			bias = BIAS;

		pos[i*4+0].Set( x-bias,			y-bias );		
		pos[i*4+1].Set( x+bias+size.x,	y-bias );		
		pos[i*4+2].Set( x+bias+size.x,	y+bias+size.y );		
		pos[i*4+3].Set( x-bias,			y+bias+size.y );	
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
	trianglesRendered += nIcons*2;
	drawCalls++;
	CHECK_GL_ERROR;
		
	glTexCoordPointer( 2, GL_FLOAT, 0, texDeco ); 
	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorDeco );
	glBindTexture( GL_TEXTURE_2D, decoTexture->glID );
	CHECK_GL_ERROR;
	glDrawElements( GL_TRIANGLES, nIcons*6, GL_UNSIGNED_SHORT, index );
	trianglesRendered += nIcons*2;
	drawCalls++;
	CHECK_GL_ERROR;

	screenport.PopUI();

	glEnableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );

	// Stream out the text.
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
	memset( bSize, 0, MAX_ICONS*sizeof( Vector2I ) );
}


void UIButtonGroup::SetPos( int index, int x, int y )
{
	GLASSERT( index >= 0 && index < nIcons );
	bPos[index].Set( x, y );
}


void UIButtonGroup::SetItemSize( int index, int x, int y ) {
	GLASSERT( index >= 0 && index < nIcons );
	bSize[index].Set( x, y );
}


int UIButtonGroup::QueryTap( int x, int y )
{
	Rectangle2I b;
	Vector2I p ={ x-origin.x, y-origin.y };

	for( int i=0; i<nIcons; ++i ) {
		int sx = size.x;
		int sy = size.y;
		if ( bSize[i].x && bSize[i].y ) {
			sx = bSize[i].x;
			sy = bSize[i].y;
		}

		b.Set( bPos[i].x,
				bPos[i].y,
				bPos[i].x + sx,
				bPos[i].y + sy );
		if ( b.Contains( p ) ) {
			if ( icons[i].enabled )
				return i;
			else
				return -1;
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
		int bias = 0;
		if ( icons[i].highLight )
			bias = BIAS;

		int sx = size.x;
		int sy = size.y;
		if ( bSize[i].x && bSize[i].y ) {
			sx = bSize[i].x;
			sy = bSize[i].y;
		}

		pos[i*4+0].Set( x-bias,			y-bias );		
		pos[i*4+1].Set( x+bias+sx,		y-bias );		
		pos[i*4+2].Set( x+bias+sx,		y+bias+sy );		
		pos[i*4+3].Set( x-bias,			y+bias+sy );	
		bounds.DoUnion( x, y );
		bounds.DoUnion( x+sx, y+sy );

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
	trianglesRendered += 2;
	drawCalls++;
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

