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
#include "texture.h"
#include "../grinliz/glvector.h"
#include "text.h"

using namespace grinliz;
using namespace gamui;


void UIRenderer::BeginRender()
{
	// Should be completely uneeded, but fixes bugs on a netbook. (With questionable drivers.)
	GPUShader::ResetState();
}


void UIRenderer::EndRender()
{
}


void UIRenderer::BeginRenderState( const void* renderState )
{
	switch ( (int)renderState )
	{
	case RENDERSTATE_UI_NORMAL:
		shader.SetColor( 1, 1, 1, 1 );
		shader.SetBlend( true );
		break;

	case RENDERSTATE_UI_NORMAL_OPAQUE:
		shader.SetColor( 1, 1, 1, 1 );
		shader.SetBlend( false );
		break;

	case RENDERSTATE_UI_DISABLED:
		shader.SetColor( 1, 1, 1, 0.5f );
		shader.SetBlend( true );
		break;

	case RENDERSTATE_UI_TEXT:
		shader.SetColor( textRed, textGreen, textBlue, 1 );
		shader.SetBlend( true );
		break;

	case RENDERSTATE_UI_TEXT_DISABLED:
		shader.SetColor( textRed, textGreen, textBlue, 0.5f );
		shader.SetBlend( true );
		break;

	case RENDERSTATE_UI_DECO:
		shader.SetColor( 1, 1, 1, 0.7f );
		shader.SetBlend( true );
		break;

	case RENDERSTATE_UI_DECO_DISABLED:
		shader.SetColor( 1, 1, 1, 0.2f );
		shader.SetBlend( true );
		break;

	default:
		GLASSERT( 0 );
		break;
	}
}


void UIRenderer::BeginTexture( const void* textureHandle )
{
	Texture* texture = (Texture*)textureHandle;
	//glBindTexture( GL_TEXTURE_2D, texture->GLID() );
	shader.SetTexture0( texture );
}


void UIRenderer::Render( const void* renderState, const void* textureHandle, int nIndex, const uint16_t* index, int nVertex, const Gamui::Vertex* vertex )
{
//	shader.SetVertex( 2, sizeof(Gamui::Vertex), &vertex[0].x );
//	shader.SetTexture0( 2, sizeof(Gamui::Vertex), &vertex[0].tx );
	GPUStream stream( GPUStream::kGamuiType );
	shader.SetStream( stream, vertex, nIndex, index );

	shader.Draw();
}


void UIRenderer::SetAtomCoordFromPixel( int x0, int y0, int x1, int y1, int w, int h, RenderAtom* atom )
{
	atom->tx0 = (float)x0 / (float)w;
	atom->tx1 = (float)x1 / (float)w;

	atom->ty0 = (float)(h-y1) / (float)h;
	atom->ty1 = (float)(h-y0) / (float)h;
}




void UIRenderer::GamuiGlyph( int c, int c1, float height, IGamuiText::GlyphMetrics* metric )
{
	UFOText::Instance()->Metrics( c, c1, height, metric );
}


/*static*/ void UIRenderer::LayoutListOnScreen( gamui::UIItem* items, int nItems, int stride, float _x, float _y, float vSpace, const Screenport& port )
{
	float w = items->Width();
	float h = items->Height()*(float)nItems + vSpace*(float)(nItems-1);
	float x = _x;
	float y = _y - h*0.5f;

	if ( x < port.UIBoundsClipped3D().min.x ) {
		x = port.UIBoundsClipped3D().min.x;
	}
	else if ( x+w >= port.UIBoundsClipped3D().max.x ) {
		x = port.UIBoundsClipped3D().max.x - w;
	}
	if ( y < port.UIBoundsClipped3D().min.y ) {
		y = port.UIBoundsClipped3D().min.y;
	}
	else if ( y+h >= port.UIBoundsClipped3D().max.y ) {
		y = port.UIBoundsClipped3D().max.y - h;
	}
	for( int i=0; i<nItems; ++i ) {
		gamui::UIItem* item = (UIItem*)((U8*)items + stride*i);
		item->SetPos( x, y + items->Height()*(float)i + vSpace*(float)i );
	}
}


void DecoEffect::Play( int startPauseTime, bool invisibleWhenDone )	
{
	this->startPauseTime = startPauseTime;
	this->invisibleWhenDone = invisibleWhenDone;
	rotation = 0;
	if ( item ) {
		item->SetVisible( true );
		item->SetRotationY( 0 );
	}
}


void DecoEffect::DoTick( U32 delta )
{
	startPauseTime -= (int)delta;
	if ( startPauseTime <= 0 ) {
		static const float CYCLE = 5000.f;
		rotation += (float)delta * 360.0f / CYCLE;

		if ( rotation > 90.f && invisibleWhenDone ) {
			if ( item ) 
				item->SetVisible( false );
		}
		while( rotation >= 360.f )
			rotation -= 360.f;

		if ( rotation > 90 && rotation < 270 )
			rotation += 180.f;

		if ( item && item->Visible() )
			item->SetRotationY( rotation );
	}
}

