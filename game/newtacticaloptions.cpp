#include "newtacticaloptions.h"
#include "../engine/engine.h"
#include "game.h"
#include "tacticalintroscene.h"

using namespace grinliz;
using namespace gamui;

NewTacticalOptions::NewTacticalOptions( Game* _game ) : Scene( _game )
{
	backgroundUI.Init( game, &gamui2D, false );

	static const char* toggleLabel[TOGGLE_COUNT] = 
		{ "4", "6", "8", "Low", "Med", "Hi", 
		"Low", "Med", "Hi", "Day", "Night",
		"Fa-S", "T-S", "Fo-S", "D-S", "Fa-D", "T-D", "Fo-D", "D-D",
		"City", "BattleShip",	"AlienBase", "TerranBase",
		"Crash" };
	static const int toggleSize[TOGGLE_COUNT] = {
		1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 2, 2, 2, 
		1
	};

	const gamui::ButtonLook& green = game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );

	for( int i=0; i<TOGGLE_COUNT; ++i ) {
		GLASSERT( toggleLabel[i] );
		toggles[i].Init( &gamui2D, green );
		toggles[i].SetText( toggleLabel[i] );
		toggles[i].SetSize( GAME_BUTTON_SIZE_B()*(float)toggleSize[i], GAME_BUTTON_SIZE_B() );
	}

	toggles[SQUAD_4].AddToToggleGroup( &toggles[SQUAD_6] );
	toggles[SQUAD_4].AddToToggleGroup( &toggles[SQUAD_8] );

	toggles[TERRAN_LOW].AddToToggleGroup( &toggles[TERRAN_MED] );
	toggles[TERRAN_LOW].AddToToggleGroup( &toggles[TERRAN_HIGH] );

	toggles[ALIEN_LOW].AddToToggleGroup( &toggles[ALIEN_MED] );
	toggles[ALIEN_LOW].AddToToggleGroup( &toggles[ALIEN_HIGH] );

	toggles[TIME_DAY].AddToToggleGroup( &toggles[TIME_NIGHT] );

	toggles[SQUAD_8].SetDown();
	toggles[TERRAN_MED].SetDown();
	toggles[ALIEN_MED].SetDown();
	toggles[TIME_DAY].SetDown();
	toggles[FARM_SCOUT].SetDown();

	terranLabel.Init( &gamui2D );
	terranLabel.SetText( "Terran Squad" );

	alienLabel.Init( &gamui2D );
	alienLabel.SetText( "Aliens" );

	timeLabel.Init( &gamui2D );
	timeLabel.SetText( "Time" );

	for( int i=0; i<MAX_ROWS; ++i ) {
		static const char* row[] = { "Scout", "Frigate", "Special" };
		rowLabel[i].Init( &gamui2D );
		rowLabel[i].SetText( row[i] );
	}

	{
		scenarioLabel.Init( &gamui2D );
		scenarioLabel.SetText( "Farm  Tundra Forest  Desert" );
	}

	for( int i=FIRST_SCENARIO; i<=LAST_SCENARIO; ++i )
		toggles[FARM_SCOUT].AddToToggleGroup( &toggles[i] );
							
	goButton.Init( &gamui2D, blue );
	goButton.SetSize( GAME_BUTTON_SIZE_B()*2.0f, GAME_BUTTON_SIZE_B() );
	goButton.SetText( "Go!" );
}


void NewTacticalOptions::Resize()
{
	const Screenport& port = GetEngine()->GetScreenport();
	LayoutCalculator layout( port.UIWidth(), port.UIHeight() );
	layout.SetSize( GAME_BUTTON_SIZE_B(), GAME_BUTTON_SIZE_B() );
	layout.SetGutter( 0 );
	layout.SetSpacing( 0 );
	layout.SetOffset( 0, GAME_CROWDED_YTWEAK() );
	layout.SetTextOffset( 5.f, 32.f );

	// Left group.
	layout.PosAbs( &terranLabel, 0, 0 );
	for( int i=0; i<3; ++i ) {
		layout.PosAbs( &toggles[SQUAD_4+i], i, 1 );
		layout.PosAbs( &toggles[TERRAN_LOW+i], i, 2 );
	}
	layout.PosAbs( &alienLabel, 0, 3 );
	for( int i=0; i<3; ++i ) {
		layout.PosAbs( &toggles[ALIEN_LOW+i], i, 4 );
	}
	layout.PosAbs( &timeLabel, 0, 5 );
	for( int i=0; i<2; ++i ) {
		layout.PosAbs( &toggles[TIME_DAY+i], i, 6 );
	}

	// Right group
	static const int RIGHT=-6; //4;
	layout.PosAbs( &scenarioLabel, RIGHT, 0 );

	layout.SetTextOffset( 10.f, 15.f );
	for( int i=0; i<3; ++i ) {
		layout.PosAbs( &rowLabel[i], RIGHT+4, i+1 );
	}	
	for( int i=0; i<4; ++i ) {
		layout.PosAbs( &toggles[FARM_SCOUT+i], RIGHT+i, 1 );
		layout.PosAbs( &toggles[FARM_DESTROYER+i], RIGHT+i, 2 );
	}
	layout.PosAbs( &toggles[CITY], RIGHT, 3 );
	layout.PosAbs( &toggles[BATTLESHIP], RIGHT+1, 3 );
	layout.PosAbs( &toggles[ALIEN_BASE], RIGHT, 4 );
	layout.PosAbs( &toggles[TERRAN_BASE], RIGHT+2, 4 );

	layout.PosAbs( &toggles[UFO_CRASH], RIGHT, 6 );

	layout.PosAbs( &goButton, -3, 6 );
	backgroundUI.background.SetSize( port.UIWidth(), port.UIHeight() );
}


void NewTacticalOptions::Tap(	int action, 
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
		NewSceneOptionsReturn data;

		data.scenario = FARM_SCOUT;
		for( int i=FIRST_SCENARIO; i<=LAST_SCENARIO; ++i ) {
			if ( toggles[i].Down() ) {
				data.scenario = i;
				break;
			}
		}
		data.crash = toggles[UFO_CRASH].Down() ? 1 : 0;

		data.nTerrans = 8;
		data.terranRank = 0;

		if ( toggles[TERRAN_MED].Down() )
			data.terranRank = 2;
		else if ( toggles[TERRAN_HIGH].Down() )
			data.terranRank = 4;

		if ( toggles[SQUAD_4].Down() )
			data.nTerrans= 4;
		else if ( toggles[SQUAD_6].Down() )
			data.nTerrans = 6;

		data.alienRank = 0;
		if ( toggles[ALIEN_MED].Down() ) {
			data.alienRank = 2;
		}
		else if ( toggles[ALIEN_HIGH].Down() ) {
			data.alienRank = 4;
		}

		data.dayTime = toggles[TIME_DAY].Down() ? 1 : 0;
		game->PopScene( *((int*)&data) );
	}
}




