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


enum {
	ICON_GREEN_BUTTON		= 0,
	ICON_BLUE_BUTTON		= 1,
	ICON_RED_BUTTON			= 2,
	ICON_GREEN_BUTTON_DOWN	= 8,
	ICON_BLUE_BUTTON_DOWN	= 9,
	ICON_RED_BUTTON_DOWN	= 10,

	ICON_AWARD_PURPLE_CIRCLE = 4,
	ICON_AWARD_ALIEN_1		= 5,
	ICON_AWARD_ALIEN_5		= 6,
	ICON_AWARD_ALIEN_10		= 7,

	ICON_GREEN_STAND_MARK	= 16,
	ICON_YELLOW_STAND_MARK	= 17,
	ICON_ORANGE_STAND_MARK	= 18,

	ICON_GREEN_STAND_MARK_OUTLINE	= 11,
	ICON_YELLOW_STAND_MARK_OUTLINE	= 12,
	ICON_ORANGE_STAND_MARK_OUTLINE	= 13,

	ICON_STAND_HIGHLIGHT	= 21,

	ICON_TARGET_STAND		= 19,
	ICON_TARGET_POINTER		= 20,

	ICON_GREEN_WALK_MARK	= 24,
	ICON_YELLOW_WALK_MARK	= 25,
	ICON_ORANGE_WALK_MARK	= 26,

	ICON_RANK_0				= 27,
	ICON_RANK_1				= 28,
	ICON_RANK_2				= 29,
	ICON_RANK_3				= 30,
	ICON_RANK_4				= 31,

	DECO_CHARACTER		= 0,
	DECO_PISTOL			= 1,
	DECO_RIFLE			= 2,
	DECO_SHELLS			= 3,
	DECO_ROCKET			= 4,
	DECO_CELL			= 5,
	DECO_RAYGUN			= 6,

	DECO_AIMED			= 8,
	DECO_AUTO			= 9,
	DECO_SNAP			= 10,
	DECO_MEDKIT			= 11,
	DECO_KNIFE			= 12,
	DECO_ALIEN			= 13,
	DECO_ARMOR			= 14,
	DECO_BOOM			= 15,

	DECO_METAL			= 16,
	DECO_TECH			= 17,
	DECO_FUEL			= 18,
	DECO_SWAP			= 19,
	DECO_HELP			= 20,
	DECO_LAUNCH			= 21,
	DECO_END_TURN		= 22,

	DECO_PREV			= 24,
	DECO_NEXT			= 25,
	DECO_ROTATE_CCW		= 26,
	DECO_ROTATE_CW		= 27,
	DECO_SHIELD			= 28,

	DECO_NONE			= 31,
};


class UIRenderer : public gamui::IGamuiRenderer, public gamui::IGamuiText
{
public:
	enum {
		RENDERSTATE_NORMAL = 1,
		RENDERSTATE_DISABLED,
		RENDERSTATE_DECO,
		RENDERSTATE_DECO_DISABLED,
		RENDERSTATE_OPAQUE,
	};

	UIRenderer() {}

	virtual void BeginRender();
	virtual void EndRender();

	virtual void BeginRenderState( const void* renderState );
	virtual void BeginTexture( const void* textureHandle );
	virtual void Render( const void* renderState, const void* textureHandle, int nIndex, const int16_t* index, int nVertex, const gamui::Gamui::Vertex* vertex ) ;

	static void SetAtomCoordFromPixel( int x0, int y0, int x1, int y1, int w, int h, gamui::RenderAtom* );

	static gamui::RenderAtom CalcDecoAtom( int id, bool enabled=true );
	static gamui::RenderAtom CalcParticleAtom( int id, bool enabled=true );
	static gamui::RenderAtom CalcIconAtom( int id, bool enabled=true );

	enum {
		PALETTE_RED, 
		PALETTE_GREEN, 
		PALETTE_BRIGHT_GREEN,
		PALETTE_DARK_GREY
	};
	static gamui::RenderAtom CalcPaletteAtom( int id, float w, float h, bool enable=true );

	virtual void GamuiGlyph( int c, gamui::IGamuiText::GlyphMetrics* metric );
	
};



#endif // UIRENDERING_INCLUDED