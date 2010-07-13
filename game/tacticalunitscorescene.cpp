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

using namespace grinliz;

TacticalUnitScoreScene::TacticalUnitScoreScene( Game* _game, const TacticalEndSceneData* d ) : Scene( _game )
{
	Engine* engine = GetEngine();
	data = d;
	nAwards = 0;

	engine->EnableMap( false );

	gamui::RenderAtom nullAtom;
	backgroundUI.Init( game, &gamui2D );


	int count=0;
	float yPos = 100.0f;
	float xPos0 = 50.0f;
	float xPos1 = 220.0f;
	float xPos2 = 300.0f;
	float size = 20.0f;

	for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i ) {
		if ( data->units[i].Status() != Unit::STATUS_NOT_INIT ) {
			
			nameRank[count].Init( &gamui2D );
			nameRank[count].Set( xPos0, yPos, &data->units[i], false );

			status[count].Init( &gamui2D );
			status[count].SetText( data->units[i].IsAlive() ? "Active" : "KIA" );
			status[count].SetPos( xPos1, yPos );

			int kills = data->units[i].KillsCredited();
			bool purpleCircle = ( data->units[i].HP() != data->units[i].GetStats().TotalHP() && data->units[i].Status() == Unit::STATUS_ALIVE );
			int exp = kills + (purpleCircle ? 2 : 0);
			float x = xPos2;

			if ( purpleCircle && nAwards < MAX_AWARDS ) {
				award[nAwards].Init( &gamui2D, UIRenderer::CalcIconAtom( ICON_AWARD_PURPLE_CIRCLE ) );
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

				award[nAwards].Init( &gamui2D, UIRenderer::CalcIconAtom( icon ) );
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
	GetEngine()->EnableMap( true );
}


void TacticalUnitScoreScene::DrawHUD()
{
//	UFOText::Draw( 20, 25, "Game restart not yet implemented." );
//	UFOText::Draw( 20, 10, "Close and select 'New' to play again." );
}


void TacticalUnitScoreScene::Tap(	int count, 
									const grinliz::Vector2F& screen,
									const grinliz::Ray& world )
{
/*	float ux, uy;
	GetEngine()->GetScreenport().ViewToUI( screen.x, screen.y, &ux, &uy );
	gamui::UIItem* item = gamui2D.Tap( ux, uy );
	if ( tap == 0 ) {
//		game->QueueReset();
	///	game->PopScene();
	}
	*/
}

