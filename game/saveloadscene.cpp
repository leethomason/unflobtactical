#include "saveloadscene.h"
#include "game.h"
#include "../engine/uirendering.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;
using namespace gamui;

SaveLoadScene::SaveLoadScene( Game* _game, const SaveLoadSceneData* _data ) : Scene( _game ), data( _data )
{
	//Engine* engine = GetEngine();
	//const Screenport& port = engine->GetScreenport();

	background.Init( &gamui2D, game->GetRenderAtom( Game::ATOM_TACTICAL_BACKGROUND ), false );
	background.SetSize( game->engine->GetScreenport().UIWidth(), game->engine->GetScreenport().UIHeight() );
	
	const ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );
	
	//static const float deltaY = GAME_BUTTON_SIZE_F + 2.0f;

	backButton.Init( &gamui2D, blue );
	backButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	backButton.SetDeco(	Game::CalcDecoAtom( DECO_CANCEL, true ),
						Game::CalcDecoAtom( DECO_CANCEL, false ) );	

	okayButton.Init( &gamui2D, blue );
	okayButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	okayButton.SetDeco(	Game::CalcDecoAtom( DECO_OKAY_CHECK, true ),
						Game::CalcDecoAtom( DECO_OKAY_CHECK, false ) );	

	confirmText.Init( &gamui2D );

	saveButton.Init( &gamui2D, blue );
	saveButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	saveButton.SetText( "Save" );

	loadButton.Init( &gamui2D, blue );
	loadButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	loadButton.SetText( "Load" );

	saveButton.AddToToggleGroup( &loadButton );
	loadButton.SetDown();		// safer default
	if ( !data->canSave ) {
		saveButton.SetEnabled( false );
	}

	for( int i=0; i<SAVE_SLOTS; ++i ) {
		slotButton[i].Init( &gamui2D, blue );
		slotButton[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		char buf[10];
		SNPrintf( buf, 10, "%d", i+1 );
		slotButton[i].SetText( buf );
		slotButton[0].AddToToggleGroup( &slotButton[i] );
		slotText[i].Init( &gamui2D );
		slotTime[i].Init( &gamui2D );
		
		GLString t;
		CStr<16> str = "<empty>";
		if ( game->HasSaveFile( SAVEPATH_GEO, i+1 )) {
			str = "Geo";
			game->SavePathTimeStamp( SAVEPATH_GEO, i+1, &t );
		}
		else if ( game->HasSaveFile( SAVEPATH_TACTICAL, i+1 )) {
			str = "Tactical";
			game->SavePathTimeStamp( SAVEPATH_TACTICAL, i+1, &t );
		}
		slotText[i].SetText( str.c_str() );
		slotTime[i].SetText( t.c_str() );
	}
	EnableSlots();
	Confirm( false );
}


SaveLoadScene::~SaveLoadScene()
{
}

void SaveLoadScene::Resize()
{
	const Screenport& port = GetEngine()->GetScreenport();
	background.SetSize( port.UIWidth(), port.UIHeight() );

	LayoutCalculator layout( port.UIWidth(), port.UIHeight() );

	layout.SetGutter( GAME_GUTTER );
	layout.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	//layout.SetSpacing( GAME_BUTTON_SPACING * 0.5f );

	layout.PosAbs( &backButton, 0, -1 );
	layout.PosAbs( &okayButton, -1, -1 );
	confirmText.SetPos( okayButton.X() - 150, okayButton.Y()+40 );

	layout.PosAbs( &saveButton, 0, 0 );
	layout.PosAbs( &loadButton, 0, 1 );

	for( int i=0; i<SAVE_SLOTS; ++i ) {
		layout.PosAbs( &slotButton[i], 2, i );
		layout.PosAbs( &slotText[i], 3, i );
		layout.PosAbs( &slotTime[i], 4, i );
	}
}


void SaveLoadScene::Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )
{
	grinliz::Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( screen, &ui );

	const UIItem* item = 0;
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

	if ( item ) {
		EnableSlots();
		Confirm( false );
	}

	if ( item == &backButton ) {
		game->PopScene();
	}
	else if ( item == &okayButton ) {
		EnableSlots();
		Confirm( true );
	}
	else if ( item == &saveButton || item == &loadButton ) {
		EnableSlots();
	}
}


void SaveLoadScene::EnableSlots()
{
	if ( saveButton.Down() ) {
		for( int i=0; i<SAVE_SLOTS; ++i ) {
			slotButton[i].SetEnabled( true );
		}
	}
	else {
		for( int i=0; i<SAVE_SLOTS; ++i ) {
			if ( StrEqual( slotText[i].GetText(), "<empty>" ) ) {
				slotButton[i].SetEnabled( false );
			}
			else {
				slotButton[i].SetEnabled( true );
			}
		}
	}
}

void SaveLoadScene::Confirm( bool action )
{
	int toggleDown = -1;
	for( int i=0; i<SAVE_SLOTS; ++i ) {
		if ( slotButton[i].Enabled() && slotButton[i].Down() ) {
			toggleDown = i;
			break;
		}
	}
	int slot = toggleDown+1;

	char buf[32] = { 0 };
	if ( saveButton.Down() ) {
		SNPrintf( buf, 32, "Save game to slot %d?", slot );
		if ( action ) {
			game->DeleteSaveFile( SAVEPATH_GEO, slot );
			game->DeleteSaveFile( SAVEPATH_TACTICAL, slot );
			game->Save( slot, true, true );
			game->PopScene();
		}
	}
	else if ( loadButton.Down() && toggleDown >= 0 ) {
		SNPrintf( buf, 32, "Load game in slot %d?", slot );
		if ( action ) {
			game->PopAllAndLoad( slot );
		}
	}
	confirmText.SetText( buf );
}
