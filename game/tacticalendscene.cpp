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
#include "cgame.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

TacticalEndScene::TacticalEndScene( Game* _game, const TacticalEndSceneData* d ) : Scene( _game )
{
	Engine* engine = GetEngine();
	data = d;

	engine->EnableMap( false );

	gamui::RenderAtom nullAtom;
	backgroundUI.Init( game, &gamui2D );

	for( int i=0; i<TEXT_ROW*TEXT_COL; ++i ) {
		textTable[i].Init( &gamui2D );
	}

	static const float XPOS = 50.f;
	static const float XPOS2 = 200.f;
	static const float YPOS = 100.f;
	static const float DELTA = 20.0f;
	
	victory.Init( &gamui2D );
	victory.SetText( data->nTerransAlive ? "Victory!" : "Defeat" );
	victory.SetPos( XPOS, YPOS );

	const char* text[TEXT_ROW] = { "Soldiers survived",  "Soldiers killed", "Aliens survived", "Aliens killed" };
	int value[TEXT_ROW]		   = { data->nTerransAlive,	data->nTerrans - data->nTerransAlive, data->nAliensAlive, data->nAliens - data->nAliensAlive };

	for( int i=0; i<TEXT_ROW; ++i ) {
		textTable[i*TEXT_COL].SetText( text[i] );
		textTable[i*TEXT_COL].SetPos( XPOS, YPOS + (float)(i+1)*DELTA );

		CStr<16> sBuf = value[i];
		textTable[i*TEXT_COL+1].SetText( sBuf.c_str() );
		textTable[i*TEXT_COL+1].SetPos( XPOS2, YPOS + (float)(i+1)*DELTA );
	}

	const gamui::ButtonLook& look = game->GetButtonLook( Game::GREEN_BUTTON );
	okayButton.Init( &gamui2D, look );
	okayButton.SetText( "OK" );
	okayButton.SetPos( 400, (float)(engine->GetScreenport().UIHeight() - (GAME_BUTTON_SIZE + 5)) );
	okayButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
}


TacticalEndScene::~TacticalEndScene()
{
	GetEngine()->EnableMap( true );
}


void TacticalEndScene::DrawHUD()
{
//	UFOText::Draw( 50, 200, "%s", data->nTerransAlive ? "Victory!" : "Defeat" );
//	UFOText::Draw( 50, 80,  "For a new game, close and select 'new'" );
}


void TacticalEndScene::Tap(	int action, 
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
		game->PopScene();
		game->PushScene( Game::UNIT_SCORE_SCENE, (void*)data );
	}
}

