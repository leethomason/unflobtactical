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

#include "game.h"
#include "areaWidget.h"
#include "geoscene.h"

using namespace gamui;

AreaWidget::AreaWidget( Game* game,
						gamui::Gamui* container,
						const char* _name )
{
	RenderAtom nullAtom;

	name.Init( container );
	name.SetText( _name );
	for( int i=0; i<2; ++i ) {
		trait[i].Init( container, nullAtom, true );
		trait[i].SetSize( name.Height()*1.2f, name.Height()*1.1f );
	}

	gamui::RenderAtom tick0Atom = Game::CalcPaletteAtom( Game::PALETTE_BLUE, Game::PALETTE_BLUE, 0 );
	tick0Atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_NORMAL;
	gamui::RenderAtom tick1Atom = Game::CalcPaletteAtom( Game::PALETTE_GREEN, Game::PALETTE_GREEN, 0 );
	tick1Atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_NORMAL;
	gamui::RenderAtom tick2Atom = Game::CalcPaletteAtom( Game::PALETTE_GREY, Game::PALETTE_GREY, Game::PALETTE_DARK );
	tick1Atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_NORMAL;

	bar.Init( container, 10, tick0Atom, tick1Atom, tick2Atom );
	bar.SetSize( 70, 6 );
	bar.SetRange( 0, 0 );

	SetOrigin( 0, 0 );
}


void AreaWidget::SetOrigin( float x, float y ) 
{
	static const float DY = 16.0f;
	static const float SPACE = 0.0f;

	name.SetPos( x, y );
	bar.SetPos( x, y+DY );

	trait[0].SetPos( name.X() + name.Width() + SPACE, 
		             name.Y()+name.Height()-trait[0].Height() );
	trait[1].SetPos( trait[0].X() + trait[0].Width(), 
		             trait[0].Y() );
}


void AreaWidget::SetInfluence( float x )
{
	bar.SetRange( 0, x/10.f );
}


void AreaWidget::SetTraits( int traits )
{
	int count=0;
	RenderAtom nullAtom;
	trait[0].SetAtom( nullAtom );
	trait[1].SetAtom( nullAtom );

	for( int i=0; i<16 && count<2; ++i ) {
		int value = traits & (1<<i);
		int id = 0;

		if ( value ) {
			switch ( value ) {
			case RegionData::TRAIT_CAPATALIST:		id=ICON2_CAP;	break;
			case RegionData::TRAIT_MILITARISTIC:	id=ICON2_MIL;	break;
			case RegionData::TRAIT_SCIENTIFIC:		id=ICON2_SCI;	break;
			case RegionData::TRAIT_NATIONALIST:		id=ICON2_NAT;	break;
			case RegionData::TRAIT_TECH:			id=ICON2_TEC;	break;
			case RegionData::TRAIT_MANUFACTURE:		id=ICON2_MAN;	break;
			}

			RenderAtom atom = Game::CalcIcon2Atom( id, true );
			atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;
			trait[count++].SetAtom( atom );
		}
	}
}
