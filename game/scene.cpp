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
	}
}



void NameRankUI::Init( gamui::Gamui* g, Game* game )
{
	this->game = game;
	gamui::RenderAtom rankAtom = UIRenderer::CalcIconAtom( ICON_RANK_0 );
	rank.Init( g, rankAtom, false );
	name.Init( g );
	gamui::RenderAtom nullAtom;
	face.Init( g, nullAtom, true );
}


void NameRankUI::SetRank( int r )
{
	gamui::RenderAtom rankAtom = UIRenderer::CalcIconAtom( ICON_RANK_0 + r );
	rank.SetAtom( rankAtom );
	rank.SetSize( name.Height(), name.Height() );
}


void NameRankUI::Set( float x, float y, const Unit* unit, bool displayFace, bool displayWeapon )
{
	CStr<90> cstr;

	if ( unit ) {
		cstr = unit->FirstName();
		cstr += " ";
		cstr += unit->LastName();

	}

	if ( displayWeapon && unit && unit->IsAlive() ) {
		if ( unit && unit->GetWeapon() ) {
			cstr += ", ";
			cstr += unit->GetWeapon()->DisplayName();
			cstr += " ";
			cstr += unit->GetWeapon()->Desc();
		}
	}
	name.SetText( cstr.c_str() );

	if ( unit ) {
		SetRank( unit->GetStats().Rank() );

		Rectangle2F uv;
		Texture* texture = game->CalcFaceTexture( unit, &uv );
		gamui::RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, 
								(const void*)texture, 
								uv.min.x, uv.min.y, uv.max.x, uv.max.y, 1, 1 );
		face.SetAtom( atom );
	}
	else {
		gamui::RenderAtom nullAtom;
		rank.SetAtom( nullAtom );
		face.SetAtom( nullAtom );
	}

	if ( displayFace ) {
		static const float SIZE = 50.0f;
		static const float FUDGE = -10.0f;

		face.SetSize( SIZE, SIZE );
		face.SetPos( x, y );
		x += face.Width() + FUDGE;
	}
	rank.SetPos( x, y+1 );
	x += rank.Width();

	name.SetPos( x+3.f, y );
}
