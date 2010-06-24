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
