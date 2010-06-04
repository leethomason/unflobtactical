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

#include "tacticalendscene.h"
#include "../engine/uirendering.h"
#include "../engine/engine.h"
#include "../engine/text.h"
#include "game.h"

TacticalEndScene::TacticalEndScene( Game* _game, const TacticalEndSceneData* d ) : Scene( _game )
{
	Engine* engine = GetEngine();
	data = d;

	engine->EnableMap( false );
	background = new UIImage( engine->GetScreenport() );

	// -- Background -- //
	Texture* bg = TextureManager::Instance()->GetTexture( "intro" );
	GLASSERT( bg );
	background->Init( bg, 480, 320 );

	int widths[2] = { 26, 12 };
	textTable = new UITextTable( engine->GetScreenport(), 2, 4, widths );
	textTable->SetOrigin( 50, 180 );
	textTable->SetText( 0, 0, "Soldiers survied" );
	textTable->SetInt( 1, 0, data->nTerransAlive );
	textTable->SetText( 0, 1, "Soldiers killed" );
	textTable->SetInt( 1, 1, data->nTerrans - data->nTerransAlive );
	textTable->SetText( 0, 2, "Aliens survived" );
	textTable->SetInt( 1, 2, data->nAliensAlive );
	textTable->SetText( 0, 3, "Aliens killed" );
	textTable->SetInt( 1, 3, data->nAliens - data->nAliensAlive );


	buttonBox = new UIButtonBox( engine->GetScreenport() );
	buttonBox->SetColumns( 1 );

	int icons[1] = { ICON_BLUE_BUTTON };

	const char* iconText[] = { "OK" };
	buttonBox->InitButtons( icons, 1 );
	buttonBox->SetOrigin( 400, 5 );
	buttonBox->SetText( iconText );
}


TacticalEndScene::~TacticalEndScene()
{
	GetEngine()->EnableMap( true );
	delete background;
	delete textTable;
	delete buttonBox;
}


void TacticalEndScene::DrawHUD()
{
	background->Draw();
	textTable->Draw();
	buttonBox->Draw();

	UFOText::Draw( 50, 200, "%s", data->nTerransAlive ? "Victory!" : "Defeat" );
//	UFOText::Draw( 50, 80,  "For a new game, close and select 'new'" );
}


void TacticalEndScene::Tap(	int count, 
							const grinliz::Vector2I& screen,
							const grinliz::Ray& world )
{
	int ux, uy;
	GetEngine()->GetScreenport().ViewToUI( screen.x, screen.y, &ux, &uy );
	int tap = buttonBox->QueryTap( ux, uy );
	if ( tap == 0 ) {
		game->PopScene();
		game->PushScene( Game::UNIT_SCORE_SCENE, (void*)data );
	}
}

