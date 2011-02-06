#include "game.h"
#include "areaWidget.h"


AreaWidget::AreaWidget( Game* game,
						gamui::Gamui* container,
						const char* _name )
{
	static const float W =  3.f;
	static const float H =  5.f;
	static const float S =  1.f;

	name.Init( container );
	name.SetText( _name );

	gamui::RenderAtom tick0Atom = UIRenderer::CalcPaletteAtom( UIRenderer::PALETTE_BLUE, UIRenderer::PALETTE_BLUE, 0, W, H );
	tick0Atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_NORMAL;
	gamui::RenderAtom tick1Atom = UIRenderer::CalcPaletteAtom( UIRenderer::PALETTE_GREEN, UIRenderer::PALETTE_GREEN, 0, W, H );
	tick1Atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_NORMAL;
	gamui::RenderAtom tick2Atom = UIRenderer::CalcPaletteAtom( UIRenderer::PALETTE_GREY, UIRenderer::PALETTE_GREY, UIRenderer::PALETTE_DARK, W, H );
	tick1Atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_NORMAL;

	static const float SPACING = 0.1f;

	bar.Init( container, 10, tick0Atom, tick1Atom, tick2Atom, S );

	SetOrigin( 0, 0 );
}


void AreaWidget::SetOrigin( float x, float y ) 
{
	static const float DY = 16.0f;

	name.SetPos( x, y );
	bar.SetPos( x, y+DY );
}

