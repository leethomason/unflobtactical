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

#include "geoendscene.h"
#include "game.h"
#include "cgame.h"

using namespace grinliz;

GeoEndScene::GeoEndScene(  Game* game, const GeoEndSceneData* data ) : Scene(game)
{
	backgroundUI.Init( game, &gamui2D, true );

	if ( data->victory )
		backgroundUI.backgroundText.SetAtom( game->GetRenderAtom( Game::ATOM_GEO_VICTORY ) );
	else
		backgroundUI.backgroundText.SetAtom( game->GetRenderAtom( Game::ATOM_GEO_DEFEAT ) );

	okayButton.Init( &gamui2D, game->GetButtonLook( Game::GREEN_BUTTON ) );
	okayButton.SetPos( 400, 320 - 5 - GAME_BUTTON_SIZE );
	okayButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	okayButton.SetText( "Done" );

}


GeoEndScene::~GeoEndScene()
{

}


void GeoEndScene::Activate()
{
	GetEngine()->SetMap( 0 );
}


void GeoEndScene::Tap(	int action, 
							const grinliz::Vector2F& screen,
							const grinliz::Ray& world )
{
	Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( screen, &ui );

	const gamui::UIItem* item = 0;
	if ( action == GAME_TAP_DOWN ) {
		gamui2D.TapDown( ui.x, ui.y );
		return;
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		return;
	}
	else if ( action == GAME_TAP_UP ) {
		item = gamui2D.TapUp( ui.x, ui.y );
	}

	if ( item == &okayButton ) {
		game->DeleteSaveFile( SAVEPATH_GEO );
		game->DeleteSaveFile( SAVEPATH_TACTICAL );
		game->PopScene();
	}
}
