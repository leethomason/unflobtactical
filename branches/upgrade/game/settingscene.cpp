#include "settingscene.h"
#include "../engine/engine.h"
#include "game.h"
#include "cgame.h"
#include "gamesettings.h"
#include "../grinliz/glstringutil.h"

using namespace gamui;
using namespace grinliz;

SettingScene::SettingScene( Game* _game ) : Scene( _game )
{
	Engine* engine = GetEngine();
	const Screenport& port = engine->GetScreenport();
	GameSettingsManager* sm = GameSettingsManager::Instance();

	background.Init( &gamui2D, game->GetRenderAtom( Game::ATOM_TACTICAL_BACKGROUND ), false );
	background.SetSize( game->engine->GetScreenport().UIWidth(), game->engine->GetScreenport().UIHeight() );


	const ButtonLook& blue  = game->GetButtonLook( Game::BLUE_BUTTON );
	const ButtonLook& green = game->GetButtonLook( Game::GREEN_BUTTON );

	doneButton.Init( &gamui2D, blue );
	doneButton.SetSize( GAME_BUTTON_SIZE_B, GAME_BUTTON_SIZE_B );
	doneButton.SetDeco( Game::CalcDecoAtom( DECO_OKAY_CHECK, true ),
						Game::CalcDecoAtom( DECO_OKAY_CHECK, false ) );

	const float SIZE = gamui2D.GetTextHeight()*2.8f;
	float y = GAME_GUTTER / 4;
	float deltaY = SIZE + 2.0f;
	float x = 196.0f + GAME_GUTTER*2.f;
	float deltaX = SIZE + 5.0f;
	float boxWidth = 220.0f;

	modText.Init( &gamui2D );
	modText.SetSize( boxWidth, SIZE );
	modText.SetText( "Mod File to use." );

	modDown.Init( &gamui2D, green );
	modDown.SetSize( SIZE, SIZE );
	modDown.SetDeco( Game::CalcDecoAtom( DECO_BUTTON_PREV, true ),
					 Game::CalcDecoAtom( DECO_BUTTON_PREV, false ) );

	modCurrent.Init( &gamui2D );
	GLString modName = CalcMod( 0 );
	modCurrent.SetText( modName.size() ? modName.c_str() : "None" );

	modUp.Init( &gamui2D, green );
	modUp.SetSize( SIZE, SIZE );
	modUp.SetDeco( Game::CalcDecoAtom( DECO_BUTTON_NEXT, true ),
				   Game::CalcDecoAtom( DECO_BUTTON_NEXT, false ) );
	y += deltaY;

	moveText.Init( &gamui2D );
	moveText.SetSize( boxWidth, SIZE );
	moveText.SetText( "Confirm movement (for small screens.)" );
	static const char* move_TEXT[4] = { "Off", "On" };
	for( int i=0; i<2; ++i ) {
		moveButton[i].Init( &gamui2D, green );
		moveButton[i].SetText( move_TEXT[i] );
		moveButton[i].SetSize( SIZE, SIZE );
		moveButton[0].AddToToggleGroup( &moveButton[i] );
	}
	moveButton[ sm->GetConfirmMove() ? 1 : 0].SetDown();
	y += deltaY;

	dotText.Init( &gamui2D );
	dotText.SetSize( boxWidth, SIZE );
	dotText.SetText( "Overlay Dots shows movement path on top of world objects." );
	static const char* dot_TEXT[4] = { "Off", "On" };
	for( int i=0; i<2; ++i ) {
		dotButton[i].Init( &gamui2D, green );
		dotButton[i].SetText( dot_TEXT[i] );
		dotButton[i].SetSize( SIZE, SIZE );
		dotButton[0].AddToToggleGroup( &dotButton[i] );
	}
	dotButton[ sm->GetNumWalkingMaps()-1 ].SetDown();
	y += deltaY;

	debugText.Init( &gamui2D );
	debugText.SetSize( boxWidth, SIZE );
	debugText.SetText( "Debug Output\nLevel 1 displays framerate." );
	static const char* DEBUG_TEXT[4] = { "Off", "1", "2", "3" };
	for( int i=0; i<4; ++i ) {
		debugButton[i].Init( &gamui2D, green );
		debugButton[i].SetText( DEBUG_TEXT[i] );
		debugButton[i].SetSize( SIZE, SIZE );
		debugButton[0].AddToToggleGroup( &debugButton[i] );
	}
	debugButton[game->GetDebugLevel()].SetDown();
	y += deltaY;

	dragText.Init( &gamui2D );
	dragText.SetSize( boxWidth, SIZE );
	dragText.SetText( "Drag to move units." );
	for( int i=0; i<2; ++i ) {
		dragButton[i].Init( &gamui2D, green );
		dragButton[i].SetText( i==0 ? "Off" : "On" );
		dragButton[i].SetSize( SIZE, SIZE );
	}
	dragButton[0].AddToToggleGroup( &dragButton[1] );
	dragButton[ sm->GetAllowDrag() ? 1 : 0 ].SetDown();
	y += deltaY;

	audioButton.Init( &gamui2D, green );
	audioButton.SetSize( SIZE, SIZE );

	if ( sm->GetAudioOn() ) {
		audioButton.SetDown();
		audioButton.SetDeco( Game::CalcDecoAtom( DECO_AUDIO, true ),
							 Game::CalcDecoAtom( DECO_AUDIO, false ) );	
	}
	else {
		audioButton.SetUp();
		audioButton.SetDeco( Game::CalcDecoAtom( DECO_MUTE, true ),
							 Game::CalcDecoAtom( DECO_MUTE, false ) );	
	}
	Resize();
}


SettingScene::~SettingScene()
{
	GameSettingsManager* sm = GameSettingsManager::Instance();

	for( int i=0; i<4; ++i ) {
		if ( debugButton[i].Down() ) {
			game->SetDebugLevel( i );
			break;
		}
	}
	for( int i=0; i<2; ++i ) {
		if ( dotButton[i].Down() ) {
			sm->SetNumWalkingMaps( i+1 );
			break;
		}
	}
	sm->SetConfirmMove( moveButton[1].Down() );
	sm->SetAudioOn( audioButton.Down() );
	sm->SetAllowDrag( dragButton[1].Down() );
}


void SettingScene::Resize()
{
	const Screenport& port = GetEngine()->GetScreenport();
	background.SetSize( port.UIWidth(), port.UIHeight() );

	gamui::LayoutCalculator layout( port.UIWidth(), port.UIHeight() );
	layout.SetSize( GAME_BUTTON_SIZE_B, GAME_BUTTON_SIZE_B );
	layout.SetSpacing( GAME_BUTTON_SPACING*0.25f );

	layout.PosAbs( &doneButton, 0, -1 );

	static const int COL = 4;
	layout.PosAbs( &modText, 0, 0 );
	layout.PosAbs( &modDown, COL, 0 );
	layout.PosAbs( &modCurrent, COL+1, 0 );
	layout.PosAbs( &modUp, COL+3, 0 );

	layout.PosAbs( &moveText, 0, 1 );
	for( int i=0; i<2; ++i ) {
		layout.PosAbs( &moveButton[i], COL+i, 1 );
	}

	layout.PosAbs( &dotText, 0, 2 );
	for( int i=0; i<2; ++i ) {
		layout.PosAbs( &dotButton[i], COL+i, 2 );
	}

	layout.PosAbs( &debugText, 0, 3 );
	for( int i=0; i<4; ++i ) {
		layout.PosAbs( &debugButton[i], COL+i, 3 );
	}

	layout.PosAbs( &dragText, 0, 4 );
	for( int i=0; i<2; ++i ) {
		layout.PosAbs( &dragButton[i], COL+i, 4 );
	}

	layout.PosAbs( &audioButton, COL, 5 );
}


void SettingScene::Activate()
{
	GetEngine()->SetMap( 0 );
}


GLString SettingScene::CalcMod( int delta )
{
	GameSettingsManager* sm = GameSettingsManager::Instance();

	const GLString* path = game->GetModDatabasePaths();
	int nPaths=0;
	int index = -1;
	while( nPaths<GAME_MAX_MOD_DATABASES && path[nPaths].size() ) {
		if ( path[nPaths] == sm->GetCurrentModName() ) {
			index = nPaths;
		}
		nPaths++;
	}
	index += delta;
	if ( index >= nPaths ) index = -1;
	if ( index < -1 ) index = nPaths-1;

	GLString name = "";
	if ( index >= 0 ) {
		StrSplitFilename( path[index], 0, &name, 0 );		
		sm->SetCurrentModName( path[index] );
	}
	else {
		sm->SetCurrentModName( name );
	}
	return name;
}


void SettingScene::Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )
{
	grinliz::Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( screen, &ui );
	GameSettingsManager* sm = GameSettingsManager::Instance();

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

	if ( audioButton.Down() ) {
		audioButton.SetDeco( Game::CalcDecoAtom( DECO_AUDIO, true ),
							 Game::CalcDecoAtom( DECO_AUDIO, false ) );	
	}
	else {
		audioButton.SetDeco( Game::CalcDecoAtom( DECO_MUTE, true ),
							 Game::CalcDecoAtom( DECO_MUTE, false ) );	
	}


	if ( item == &doneButton ) {
		game->PopScene();
	}
	else if ( item == &modDown ) {
		GLString name = CalcMod( -1 );
		game->LoadModDatabase( sm->GetCurrentModName().c_str(), false );
		modCurrent.SetText( name.size() ? name.c_str() : "None" );
	}
	else if ( item == &modUp ) {
		GLString name = CalcMod( +1 );
		game->LoadModDatabase( sm->GetCurrentModName().c_str(), false );
		modCurrent.SetText( name.size() ? name.c_str() : "None" );
	}
}
