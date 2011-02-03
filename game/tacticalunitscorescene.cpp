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

#include "tacticalunitscorescene.h"
#include "tacticalendscene.h"
#include "../engine/uirendering.h"
#include "../engine/engine.h"
#include "../engine/text.h"
#include "../grinliz/glrectangle.h"
#include "game.h"
#include "cgame.h"


using namespace grinliz;

TacticalUnitScoreScene::TacticalUnitScoreScene( Game* _game, const TacticalEndSceneData* d ) : Scene( _game )
{
	Engine* engine = GetEngine();
	data = d;
	nAwards = 0;

	gamui::RenderAtom nullAtom;
	backgroundUI.Init( game, &gamui2D, false );


	int count=0;
	float yPos = 100.0f;
	float xPos0 = 50.0f;
	float xPos1 = 240.0f;
	float xPos2 = 320.0f;
	float size = 20.0f;

	for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i ) {
		if ( data->units[i].InUse() ) {
			
			nameRank[count].Init( &gamui2D );
			nameRank[count].Set( xPos0, yPos, &data->units[i], false );

			status[count].Init( &gamui2D );

			if ( data->units[i].IsAlive() )
				status[count].SetText( "Active" );
			else if ( data->units[i].IsKIA() )
				status[count].SetText( "KIA" );
			else if ( data->units[i].IsUnconscious() )
				status[count].SetText( "Active(KO)" );
			else if ( data->units[i].IsMIA() )
				status[count].SetText( "MIA" );

			status[count].SetPos( xPos1, yPos );

			int kills = data->units[i].KillsCredited();
			bool purpleCircle = data->units[i].HP() != data->units[i].GetStats().TotalHP() && !data->units[i].IsKIA();
			//int exp = kills + (purpleCircle ? 2 : 0);
			float x = xPos2;

			if ( purpleCircle && nAwards < MAX_AWARDS ) {
				award[nAwards].Init( &gamui2D, UIRenderer::CalcIconAtom( ICON_AWARD_PURPLE_CIRCLE ), true );
				award[nAwards].SetPos( x, yPos );
				award[nAwards].SetSize( size, size );
				x += size;
				++nAwards;
			}

			while ( kills && nAwards < MAX_AWARDS ) {
				int icon = ICON_AWARD_ALIEN_1;
				if ( kills >= 10 ) {
					icon = ICON_AWARD_ALIEN_10;
					kills -= 10;
				}
				else if ( kills >= 5 ) {
					icon = ICON_AWARD_ALIEN_5;
					kills -= 5;
				}
				else {
					kills -= 1;
				}

				award[nAwards].Init( &gamui2D, UIRenderer::CalcIconAtom( icon ), true );
				award[nAwards].SetPos( x, yPos );
				award[nAwards].SetSize( size, size );
				x += size;
				++nAwards;
			}
			++count;
			yPos += size;
		}
	}

	button.Init( &gamui2D, game->GetButtonLook( Game::GREEN_BUTTON ) );
	button.SetPos( 400, 320 - 5 - GAME_BUTTON_SIZE );
	button.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	button.SetText( "Done" );
}


TacticalUnitScoreScene::~TacticalUnitScoreScene()
{
}

void TacticalUnitScoreScene::Activate()
{
	GetEngine()->SetMap( 0 );
}



void TacticalUnitScoreScene::DrawHUD()
{
//	UFOText::Draw( 20, 25, "Game restart not yet implemented." );
//	UFOText::Draw( 20, 10, "Close and select 'New' to play again." );
}


void TacticalUnitScoreScene::Tap(	int action, 
									const grinliz::Vector2F& view,
									const grinliz::Ray& world )
{
	Vector2F ui;
	game->engine->GetScreenport().ViewToUI( view, &ui );
	const gamui::UIItem* item = 0;

	if ( action == GAME_TAP_DOWN )
		gamui2D.TapDown( ui.x, ui.y );
	else if ( action == GAME_TAP_UP )
		item = gamui2D.TapUp( ui.x, ui.y );
		
	if ( item == &button ) {
		GLString savePath = game->GameSavePath();
		FILE* fp = fopen( savePath.c_str(), "w" );
		if ( fp ) {
			fclose( fp );
		}
		game->PopAllAndReset();
	}
}

