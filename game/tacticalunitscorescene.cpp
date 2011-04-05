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

TacticalUnitScoreScene::TacticalUnitScoreScene( Game* _game ) : Scene( _game )
{
	Engine* engine = GetEngine();
	nAwards = 0;

	gamui::RenderAtom nullAtom;
	backgroundUI.Init( game, &gamui2D, false );

	int count=0;
	float yPos			= GAME_GUTTER;
	float xPosName		= GAME_GUTTER;
	float xPosStatus	= GAME_GUTTER+200.0f;
	float xPosAward		= GAME_GUTTER+300.0f;
	float size = 20.0f;
	const Unit* soldiers = &game->battleData.units[TERRAN_UNITS_START];

	for( int i=0; i<MAX_TERRANS; ++i ) {
		if ( soldiers[i].InUse() ) {
			
			// Hack to compute XP since this scene can get run multiple times between saves.
			// The Unit* must be const, so the rank can't be changed on this screen.
			int oldRank = soldiers[i].GetStats().Rank();
			int newRank = Unit::XPToRank( soldiers[i].XP() + soldiers[i].KillsCredited() + ( soldiers[i].HP() < soldiers[i].GetStats().TotalHP() ) );

			nameRank[count].Init( &gamui2D );
			nameRank[count].Set( xPosName, yPos, &soldiers[i], false );
			nameRank[count].SetRank( newRank );

			status[count].Init( &gamui2D );

			if ( soldiers[i].IsAlive() )
				status[count].SetText( "Active" );
			else if ( soldiers[i].IsKIA() )
				status[count].SetText( "KIA" );
			else if ( soldiers[i].IsUnconscious() )
				status[count].SetText( "Active(KO)" );
			else if ( soldiers[i].IsMIA() )
				status[count].SetText( "MIA" );

			status[count].SetPos( xPosStatus, yPos );

			int kills = soldiers[i].KillsCredited();
			bool purpleCircle = soldiers[i].HP() != soldiers[i].GetStats().TotalHP() && !soldiers[i].IsKIA();

			float x = xPosAward;

			if ( soldiers[i].IsAlive() || soldiers[i].IsUnconscious() ) {

				if ( oldRank != newRank ) {
					award[nAwards].Init( &gamui2D, UIRenderer::CalcIconAtom( ICON_LEVEL_UP ), true );
					award[nAwards].SetPos( x, yPos );
					award[nAwards].SetSize( size, size );
					x += size;
					++nAwards;
				}
			}

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
	button.SetPos( 0, 320 - 5 - GAME_BUTTON_SIZE );
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
		game->PopScene( 0 );
	}
}

