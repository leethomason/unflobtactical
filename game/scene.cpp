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



void NameRankUI::Init( gamui::Gamui* g )
{
	gamui::RenderAtom rankAtom = UIRenderer::CalcIconAtom( ICON_RANK_0 );
	rank.Init( g, rankAtom, true );
	name.Init( g );
}


void NameRankUI::SetRank( int r )
{
	gamui::RenderAtom rankAtom = UIRenderer::CalcIconAtom( ICON_RANK_0 + r );
	rank.SetAtom( rankAtom );
	rank.SetSize( name.Height(), name.Height() );
}


void NameRankUI::Set( float x, float y, const Unit* unit, bool displayWeapon )
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
	}
	else {
		gamui::RenderAtom nullAtom;
		rank.SetAtom( nullAtom );
	}

	rank.SetPos( x, y+1 );
	name.SetPos( x+rank.Width()+3.f, y );
}
