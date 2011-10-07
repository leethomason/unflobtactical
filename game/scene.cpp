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
#include "scene.h"
#include "../grinliz/glstringutil.h"
#include "unit.h"

using namespace grinliz;


Scene::Scene( Game* _game )
	: game( _game )
{
	gamui2D.Init( &uiRenderer, game->GetRenderAtom( Game::ATOM_TEXT ), game->GetRenderAtom( Game::ATOM_TEXT_D ), &uiRenderer );
	gamui3D.Init( &uiRenderer, game->GetRenderAtom( Game::ATOM_TEXT ), game->GetRenderAtom( Game::ATOM_TEXT_D ), &uiRenderer );
	gamui2D.SetTextHeight( 18 );
	gamui3D.SetTextHeight( 18 );
}


Engine* Scene::GetEngine()
{
	return game->engine;
}



void BackgroundUI::Init( Game* game, gamui::Gamui* g, bool logo )
{
	background.Init( g, game->GetRenderAtom( Game::ATOM_TACTICAL_BACKGROUND ), false );
	background.SetSize( game->engine->GetScreenport().UIWidth(), game->engine->GetScreenport().UIHeight() );

	if ( logo ) {
		backgroundText.Init( g, game->GetRenderAtom( Game::ATOM_TACTICAL_BACKGROUND_TEXT ), true );
		backgroundText.SetForeground( true );
		backgroundText.SetPos( 20, 10 );
		static const float SIZE = 320.0f;
		backgroundText.SetSize( SIZE, SIZE*0.5f );
	}
}



void NameRankUI::Init( gamui::Gamui* g, Game* game )
{
	this->game = game;

	name.Init( g );
	gamui::RenderAtom nullAtom;
	rank.Init( g, nullAtom, true );
	face.Init( g, nullAtom, true );
	display = 255;
	faceSize = FACE_SIZE;
}


void NameRankUI::SetRank( int r )
{
	gamui::RenderAtom rankAtom = UIRenderer::CalcIconAtom( ICON_RANK_0 + r );
	rank.SetAtom( rankAtom );
}


void NameRankUI::Set( float x, float y, const Unit* unit, int display )
{
	this->display = display;
	CStr<90> cstr;

	if ( unit ) {
		SetRank( unit->GetStats().Rank() );
		cstr = unit->FirstName();
		cstr += " ";
		cstr += unit->LastName();
	}

	if ( (display & DISPLAY_WEAPON) && unit && unit->IsAlive() ) {
		if ( unit && unit->GetWeapon() ) {
			cstr += ", ";
			cstr += unit->GetWeapon()->DisplayName();
			cstr += " ";
			cstr += unit->GetWeapon()->Desc();
		}
	}
	name.SetText( cstr.c_str() );
	rank.SetSize( name.Height(), name.Height() );

	gamui::RenderAtom nullAtom;
	face.SetAtom( nullAtom );

	if ( (display & DISPLAY_FACE) && unit ) {

		Rectangle2F uv;
		Texture* texture = game->CalcFaceTexture( unit, &uv );
		gamui::RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, 
								(const void*)texture, 
								uv.min.x, uv.min.y, uv.max.x, uv.max.y );
		face.SetAtom( atom );

		static const float FUDGE = -2.0f;

		face.SetSize( faceSize, faceSize );
		face.SetPos( x, y );
		x += face.Width() + FUDGE;
	}

	if ( display & DISPLAY_RANK ) {
		rank.SetPos( x, y+1 );
		x += rank.Width();
	}
	else {
		rank.SetAtom( nullAtom );
	}

	name.SetPos( x+3.f, y );
}


void OKCancelUI::Init( Game* game, gamui::Gamui* gamui2D, float size ) 
{
	const Screenport& port = game->engine->GetScreenport();
	const gamui::ButtonLook& green = game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& red = game->GetButtonLook( Game::RED_BUTTON );

	okayButton.Init( gamui2D, green );
	okayButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_OKAY, true ), UIRenderer::CalcDecoAtom( DECO_OKAY, false ) );
	okayButton.SetSize( size*2, size );
	okayButton.SetPos( port.UIWidth()-size*4, port.UIHeight()-size );

	cancelButton.Init( gamui2D, red );
	cancelButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_END_TURN, true ), UIRenderer::CalcDecoAtom( DECO_END_TURN, false ) );
	cancelButton.SetSize( size*2, size );
	cancelButton.SetPos( size*2, port.UIHeight()-size );
}