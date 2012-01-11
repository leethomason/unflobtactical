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

#ifndef UIRENDERING_INCLUDED
#define UIRENDERING_INCLUDED

#include "../gamui/gamui.h"
#include "gpustatemanager.h"

class Screenport;


class UIRenderer : public gamui::IGamuiRenderer, public gamui::IGamuiText
{
public:
	enum {
		RENDERSTATE_UI_NORMAL = 1,		// normal UI rendering (1.0 alpha)
		RENDERSTATE_UI_NORMAL_OPAQUE,	// same, but for resources that don't blend (background images)
		RENDERSTATE_UI_DISABLED,		// disabled of both above
		RENDERSTATE_UI_TEXT,			// text rendering
		RENDERSTATE_UI_TEXT_DISABLED,
		RENDERSTATE_UI_DECO,			// deco rendering
		RENDERSTATE_UI_DECO_DISABLED,
	};

	UIRenderer() : shader( true ), textRed( 1 ), textGreen( 1 ), textBlue( 1 ) {}

	void SetTextColor( float r, float g, float b )		{ textRed = r; textGreen = g; textBlue = b; }

	virtual void BeginRender();
	virtual void EndRender();

	virtual void BeginRenderState( const void* renderState );
	virtual void BeginTexture( const void* textureHandle );
	virtual void Render( const void* renderState, const void* textureHandle, int nIndex, const uint16_t* index, int nVertex, const gamui::Gamui::Vertex* vertex ) ;

	static void SetAtomCoordFromPixel( int x0, int y0, int x1, int y1, int w, int h, gamui::RenderAtom* );
	static void LayoutListOnScreen( gamui::UIItem* items, int nItems, int stride, float x, float y, float vSpace, const Screenport& port );
	virtual void GamuiGlyph( int c, int c1, float lineHeight, gamui::IGamuiText::GlyphMetrics* metric );
	
private:
	CompositingShader shader;
	float textRed, textGreen, textBlue;
};


class DecoEffect
{
public:
	DecoEffect() : startPauseTime( 0 ), 
		           invisibleWhenDone( false ), 
				   rotation( 0 ), 
				   item( 0 ) {}

	void Attach( gamui::UIItem* item )						{ this->item = item; }
	void Play( int startPauseTime, bool invisibleWhenDone );
	void DoTick( U32 delta );

private:
	int startPauseTime;
	bool invisibleWhenDone;
	float rotation;
	gamui::UIItem* item;
};

#endif // UIRENDERING_INCLUDED