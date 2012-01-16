#include "newgeooptions.h"

#include "../engine/engine.h"
#include "game.h"
#include "tacticalintroscene.h"

using namespace grinliz;
using namespace gamui;


NewGeoOptions::NewGeoOptions( Game* game ) : Scene( game )
{
	backgroundUI.Init( game, &gamui2D, false );

	const gamui::ButtonLook& green = game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );

	goButton.Init( &gamui2D, blue );
	goButton.SetSize( GAME_BUTTON_SIZE_F*2.0f, GAME_BUTTON_SIZE_F );
	goButton.SetText( "Go!" );

	const static char* TEXT[NUM_DIFFICULTY] = { "Easier", "Normal", "Hard", "Smoking Crater" };

	for( int i=0; i<NUM_DIFFICULTY; ++i ) {
		toggle[i].Init( &gamui2D, green );
		toggle[i].SetSize( GAME_BUTTON_SIZE_F*2.0f, GAME_BUTTON_SIZE_F );
		toggle[i].SetText( TEXT[i] );
		toggle[0].AddToToggleGroup( &toggle[i] );
	}
	toggle[1].SetDown();
}


void NewGeoOptions::Resize()
{
	const Screenport& port = GetEngine()->GetScreenport();

	LayoutCalculator layout( port.UIWidth(), port.UIHeight() );
	layout.SetGutter( GAME_GUTTER );
	layout.SetSpacing( GAME_BUTTON_SPACING );
	layout.SetSize( GAME_BUTTON_SIZE_F*2.0f, GAME_BUTTON_SIZE_F );

	backgroundUI.background.SetSize( port.UIWidth(), port.UIHeight() );
	for( int i=0; i<NUM_DIFFICULTY; ++i ) {
		layout.PosAbs( &toggle[i], 0, i );
	}
	layout.PosAbs( &goButton, -1, -1 );
}


void NewGeoOptions::Tap(	int action, 
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
	else if ( action == GAME_TAP_MOVE ) {
		return;
	}
	else if ( action == GAME_TAP_UP ) {
		item = gamui2D.TapUp( ui.x, ui.y );
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		return;
	}

	if ( item == &goButton ) {
		int difficulty = 1;
		for( int i=0; i<NUM_DIFFICULTY; ++i ) {
			if ( toggle[i].Down() ) {
				difficulty = i;
				break;
			}
		}
		game->PopScene( difficulty );
	}
}




