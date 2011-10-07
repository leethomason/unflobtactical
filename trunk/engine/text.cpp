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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "../grinliz/glutil.h"
#include "../grinliz/glrectangle.h"
#include "gpustatemanager.h"

using namespace grinliz;

const float SMALL_SCALE = 0.75f;
const float DEFAULT_HEIGHT = 16.f;
//const int TEXTURE_WIDTH = 256;
//const int TEXTURE_HEIGHT = 128;
//const int GLYPH_WIDTH = TEXTURE_WIDTH / UFOText::GLYPH_CX;
//const int GLYPH_HEIGHT = TEXTURE_HEIGHT / UFOText::GLYPH_CY;


PTVertex2 UFOText::vBuf[BUF_SIZE*4];
//Vector2F UFOText::tBuf[BUF_SIZE*4];
U16 UFOText::iBuf[BUF_SIZE*6] = { 0, 0 };

Screenport* UFOText::screenport = 0;
Texture* UFOText::texture = 0;
const gamedb::Reader* UFOText::database = 0;
//GlyphMetric UFOText::glyphMetric[GLYPH_CX*GLYPH_CY];

void UFOText::InitScreen( Screenport* sp )
{
	screenport = sp;
}


void UFOText::Init( Texture* tex, const gamedb::Reader* db )
{
	GLASSERT( tex );
	GLASSERT( db );
	texture = tex;
	database = db;
}


void UFOText::Metrics(	int c, int cPrev,
						float lineHeight,
						gamui::IGamuiText::GlyphMetrics* metric )
{
	if ( c < 0 )  c += 256;
	if ( cPrev < 0 ) cPrev += 256;

	float s2 = 1.f;
	if ( c > 128 ) {
		s2 = 0.75f;
		c -= 128;
	}

	char buffer[] = "char0";
	buffer[4] = (char)c;
	char kernbuf[] = "kerning00";
	kernbuf[7] = (char)cPrev;
	kernbuf[8] = (char)c;

	// Find (or not) the item is the database.
	const gamedb::Item* fontItem = database->Root()->Child( "data" )
												   ->Child( "fonts" )
												   ->Child( "font" );
	fontItem = database->ChainItem( fontItem );
	const gamedb::Item* infoItem = fontItem->Child( "info" );
	const gamedb::Item* commonItem = fontItem->Child( "common" );
	const gamedb::Item* charItem = fontItem->Child( "chars" )->Child( buffer );
	const gamedb::Item* kernsItem = fontItem->Child( "kernings" );
	const gamedb::Item* kernItem = 0;
	if ( kernsItem ) {
		kernItem = kernsItem->Child( kernbuf );
	}

	float fontSize = (float)infoItem->GetInt( "size" );
	float scale = lineHeight / fontSize;
	float texWidthInv = 1.0f / (float)commonItem->GetInt( "scaleW" );
	float texHeight = (float)commonItem->GetInt( "scaleH" );
	float texHeightInv = 1.0f / texHeight;
	float kern = 0;
	if ( kernItem && s2 == 1.0f ) {
		kern = (float)kernItem->GetInt( "amount" );
	}

	if ( charItem ) {
		metric->advance = ((float)charItem->GetInt( "xadvance" )+kern) * scale * s2;

		float x = (float)charItem->GetInt( "x" );
		float y = (float)charItem->GetInt( "y" );
		float width = (float)charItem->GetInt( "width" );
		float height = (float)charItem->GetInt( "height" );

		metric->x = ((float)charItem->GetInt( "xoffset" )+kern) * scale;
		metric->w = width * scale * s2;
		metric->y = (float)charItem->GetInt( "yoffset" ) * scale;
		metric->h = height * scale * s2;

		metric->tx0 = x * texWidthInv;
		metric->tx1 = (x + width) * texWidthInv;
		metric->ty0 = (texHeight - 1.f - y) * texHeightInv;
		metric->ty1 = (texHeight - 1.f - y - height) * texHeightInv;
	}
	else {
		metric->advance = lineHeight * 0.4f;

		metric->x = metric->y = metric->w = metric->h = 0;
		metric->tx0 = metric->tx1 = metric->ty0 = metric->ty1 = 0;
	}
}


void UFOText::TextOut( GPUShader* shader, const char* str, int _x, int _y, int _h, int* w )
{
	bool rendering = true;
	float x = (float)_x;
	float y = (float)_y;
	float h = (float)_h;

	if ( w ) {
		*w = 0;
		rendering = false;
	}

	GLASSERT( !rendering || shader );

	if ( rendering ) {
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
		// Draw a glyph or nothing, at this point:
		GLASSERT( pos < BUF_SIZE );

		gamui::IGamuiText::GlyphMetrics metric;
		Metrics( *str, *(str+1), (float)h, &metric );

		if ( rendering ) {
			vBuf[pos*4+0].tex.Set( metric.tx0, metric.ty0 );
			vBuf[pos*4+1].tex.Set( metric.tx1, metric.ty0 );
			vBuf[pos*4+2].tex.Set( metric.tx1, metric.ty1 );
			vBuf[pos*4+3].tex.Set( metric.tx0, metric.ty1 );

			vBuf[pos*4+0].pos.Set( (float)x+metric.x,			(float)y+metric.y );	
			vBuf[pos*4+1].pos.Set( (float)x+metric.x+metric.w,	(float)y+metric.y );	
			vBuf[pos*4+2].pos.Set( (float)x+metric.x+metric.w,	(float)y+metric.y+metric.h );	
			vBuf[pos*4+3].pos.Set( (float)x+metric.x,			(float)y+metric.y+metric.h );				
		}
		x += metric.advance;
		++pos;

		if ( rendering ) {
			if ( pos == BUF_SIZE || *(str+1) == 0 ) {
				if ( pos > 0 ) {
					GPUShader::Stream stream( vBuf );
					shader->SetStream( stream, vBuf, pos*6, iBuf );
					shader->SetTexture0( texture );

					shader->Draw();
					pos = 0;
				}
			}
		}
		++str;
	}
	if ( w ) {
		*w = int(x+0.5f) - _x;
	}
}


//void UFOText::GlyphSize( const char* str, int* width, int* height )
//{
//	TextOut( 0, str, 0, 0, width, height );
//}


void UFOText::Draw( int x, int y, const char* format, ... )
{
	screenport->SetUI( 0 );
	CompositingShader shader( true );

    va_list     va;
	const int	size = 1024;
    char		buffer[size];

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    vsnprintf_s( buffer, size, _TRUNCATE, format, va );
#else
	// Reading the spec, the size does seem correct. The man pages
	// say it will aways be null terminated (whereas the strcpy is not.)
	// Pretty nervous about the implementation, so force a null after.
    vsnprintf( buffer, size, format, va );
	buffer[size-1] = 0;
#endif
	va_end( va );

    TextOut( &shader, buffer, x, y, 0, 0 );
}
