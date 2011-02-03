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

	gamui::RenderAtom nullAtom;
	backgroundUI.Init( game, &gamui2D, false );

	for( int i=0; i<TEXT_ROW*TEXT_COL; ++i ) {
		textTable[i].Init( &gamui2D );
	}

	static const float X_NAME = 50.f;
	static const float X_COUNT = 250.f;
	static const float X_SCORE = 300.0f;
	static const float YPOS = 100.f;
	static const float DELTA = 20.0f;
	
	victory.Init( &gamui2D );
	if ( data->nTerransAlive && !data->nAliensAlive )
		victory.SetText( "Victory!" );
	else if ( !data->nTerransAlive && data->nTerrans )
		victory.SetText( "Defeat." );
	else 
		victory.SetText( "Mission Summary:" );
	victory.SetPos( X_NAME, YPOS );

	const char* text[TEXT_ROW] = { "Soldiers survived",  "Soldiers KIA/MIA/KO", "Aliens survived", "Aliens killed", "Civs Saved", "Civs Killed" };

	int soldiersOut = data->nTerrans - data->nTerransAlive;
	int aliensKilled = data->nAliens - data->nAliensAlive;
	int civsKilled = data->nCivs - data->nCivsAlive;

	int value[TEXT_ROW]		   = {	data->nTerransAlive, soldiersOut,
									data->nAliensAlive, aliensKilled,
									data->nCivsAlive, civsKilled
								 };

	// Lose points for soldiers killed,
	// Gain points for aliens killed,
	// Gain + Lose points for civs

	int score[3] = { 0 };

	for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i ) {
		if ( d->units[i].InUse() && !d->units[i].IsAlive() ) {
			score[0] -= d->units[i].GetStats().ScoreLevel();
		}
	}
	for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) {
		if ( d->units[i].InUse() && !d->units[i].IsAlive() ) {
			score[1] += d->units[i].GetStats().ScoreLevel();
		}
	}
	if ( !d->dayTime ) {
		// 50% bonus for night.
		score[1] = score[1]*3/2;
	}
	// Adjust for odds, based on the starting out numbers.
	score[1] = score[1] * d->nAliens / d->nTerrans;

	for( int i=CIV_UNITS_START; i<CIV_UNITS_END; ++i ) {
		if ( d->units[i].InUse() ) {
			if ( d->units[i].IsAlive() )
				score[2] += d->units[i].GetStats().ScoreLevel();
			else
				score[2] -= d->units[i].GetStats().ScoreLevel();
		}
	}

	for( int i=0; i<TEXT_ROW; ++i ) {
		textTable[i*TEXT_COL].SetText( text[i] );
		textTable[i*TEXT_COL].SetPos( X_NAME, YPOS + (float)(i+1)*DELTA );

		CStr<16> sBuf = value[i];
		textTable[i*TEXT_COL+1].SetText( sBuf.c_str() );
		textTable[i*TEXT_COL+1].SetPos( X_COUNT, YPOS + (float)(i+1)*DELTA );

		if ( i&1 ) {
			sBuf = score[i/2];
			textTable[i*TEXT_COL+2].SetText( sBuf.c_str() );
			textTable[i*TEXT_COL+2].SetPos( X_SCORE, YPOS + (float)(i+1)*DELTA );
		}
	}

	CStr<16> totalBuf = score[0]+score[1]+score[2];
	totalScoreValue.Init( &gamui2D );
	totalScoreValue.SetPos( X_SCORE, YPOS + (float)(TEXT_ROW+2)*DELTA );
	totalScoreValue.SetText( totalBuf.c_str() );

	totalScoreLabel.Init( &gamui2D );
	totalScoreLabel.SetPos( X_NAME, YPOS + (float)(TEXT_ROW+2)*DELTA );
	totalScoreLabel.SetText( "Total Score" );

	const gamui::ButtonLook& look = game->GetButtonLook( Game::GREEN_BUTTON );
	okayButton.Init( &gamui2D, look );
	okayButton.SetText( "OK" );
	okayButton.SetPos( 400, (float)(engine->GetScreenport().UIHeight() - (GAME_BUTTON_SIZE + 5)) );
	okayButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
}


TacticalEndScene::~TacticalEndScene()
{
}


void TacticalEndScene::Activate()
{
	GetEngine()->SetMap( 0 );
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
		game->PushScene( Game::UNIT_SCORE_SCENE, new TacticalEndSceneData( *data ) );
	}
}

