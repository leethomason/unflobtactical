#include "saveloadscene.h"
#include "game.h"
#include "../engine/uirendering.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;
using namespace gamui;

SaveLoadScene::SaveLoadScene( Game* _game, const SaveLoadSceneData* _data ) : Scene( _game ), data( _data )
{
	Engine* engine = GetEngine();
	const Screenport& port = engine->GetScreenport();

	background.Init( &gamui2D, game->GetRenderAtom( Game::ATOM_TACTICAL_BACKGROUND ), false );
	background.SetSize( game->engine->GetScreenport().UIWidth(), game->engine->GetScreenport().UIHeight() );
	
	const ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );
	
	static const float deltaY = GAME_BUTTON_SIZE_F + 2.0f;

	backButton.Init( &gamui2D, blue );
	backButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	backButton.SetText( "Cancel" );
	backButton.SetPos( GAME_GUTTER, 
					   port.UIHeight() - GAME_BUTTON_SIZE_F - GAME_GUTTER );

	okayButton.Init( &gamui2D, blue );
	okayButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	okayButton.SetText( "Okay" );
	okayButton.SetPos( port.UIWidth() - GAME_BUTTON_SIZE_F - GAME_GUTTER, 
					   port.UIHeight() - GAME_BUTTON_SIZE_F - GAME_GUTTER );

	confirmText.Init( &gamui2D );
	confirmText.SetPos( okayButton.X() - 180, okayButton.Y()+30 );

	saveButton.Init( &gamui2D, blue );
	saveButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	saveButton.SetText( "Save" );
	saveButton.SetPos( GAME_GUTTER, GAME_GUTTER );

	loadButton.Init( &gamui2D, blue );
	loadButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	loadButton.SetText( "Load" );
	loadButton.SetPos( GAME_GUTTER, saveButton.Y() + deltaY );

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
		slotButton[i].SetPos( 120, GAME_GUTTER + deltaY*(float)i );
		if ( i > 0 ) {
			slotButton[0].AddToToggleGroup( &slotButton[i] );
		}
		slotText[i].Init( &gamui2D );
		slotText[i].SetPos( slotButton[i].X() + slotButton[i].Width() + GAME_GUTTER, slotButton[i].Y() );
		slotTime[i].Init( &gamui2D );
		slotTime[i].SetPos( slotText[i].X(), slotText[i].Y()+gamui2D.GetTextHeight() );
		
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
			game->Save( slot, true );
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
