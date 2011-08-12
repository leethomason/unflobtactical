#include "settingscene.h"
#include "../engine/engine.h"
#include "game.h"
#include "cgame.h"
#include "settings.h"
#include "../grinliz/glstringutil.h"

using namespace gamui;
using namespace grinliz;

SettingScene::SettingScene( Game* _game ) : Scene( _game )
{
	Engine* engine = GetEngine();
	const Screenport& port = engine->GetScreenport();
	SettingsManager* sm = SettingsManager::Instance();

	background.Init( &gamui2D, game->GetRenderAtom( Game::ATOM_TACTICAL_BACKGROUND ), false );
	background.SetSize( game->engine->GetScreenport().UIWidth(), game->engine->GetScreenport().UIHeight() );


	const ButtonLook& blue  = game->GetButtonLook( Game::BLUE_BUTTON );
	const ButtonLook& green = game->GetButtonLook( Game::GREEN_BUTTON );

	doneButton.Init( &gamui2D, blue );
	doneButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	doneButton.SetText( "X" );
	doneButton.SetPos( 0, port.UIHeight() - GAME_BUTTON_SIZE_F);

	static const float SIZE = 50.0f;
	float y = GAME_GUTTER;
	float deltaY = SIZE + 5.0f;
	float x = 220.0f + GAME_GUTTER*2.f;
	float deltaX = SIZE + 5.0f;
	float boxWidth = 220.0f;

	modText.Init( &gamui2D );
	modText.SetSize( boxWidth, SIZE );
	modText.SetText( "Mod File to use." );
	modText.SetPos( GAME_GUTTER,  y );

	modDown.Init( &gamui2D, green );
	modDown.SetSize( SIZE, SIZE );
	modDown.SetPos( x, y );
	modDown.SetDeco( UIRenderer::CalcDecoAtom( DECO_PREV, true ),
					 UIRenderer::CalcDecoAtom( DECO_PREV, false ) );

	modCurrent.Init( &gamui2D );
	modCurrent.SetPos( x+deltaX, y );
	GLString modName = sm->GetCurrentModName();
	modCurrent.SetText( modName.size() ? modName.c_str() : "None" );

	modUp.Init( &gamui2D, green );
	modUp.SetSize( SIZE, SIZE );
	modUp.SetPos( x+deltaX*3.f, y );
	modUp.SetDeco( UIRenderer::CalcDecoAtom( DECO_NEXT, true ),
				   UIRenderer::CalcDecoAtom( DECO_NEXT, false ) );
	y += deltaY;

	moveText.Init( &gamui2D );
	moveText.SetSize( boxWidth, SIZE );
	moveText.SetText( "Confirm movement. Recommended for smaller touch screens." );
	moveText.SetPos( GAME_GUTTER, y );
	static const char* move_TEXT[4] = { "Off", "On" };
	for( int i=0; i<2; ++i ) {
		moveButton[i].Init( &gamui2D, green );
		moveButton[i].SetText( move_TEXT[i] );
		moveButton[i].SetPos( x + deltaX*(float)i, y );
		moveButton[i].SetSize( SIZE, SIZE );
		moveButton[0].AddToToggleGroup( &moveButton[i] );
	}
	moveButton[ sm->GetConfirmMove() ? 1 : 0].SetDown();
	y += deltaY;

	dotText.Init( &gamui2D );
	dotText.SetSize( boxWidth, SIZE );
	dotText.SetText( "Overlay Dots shows movement path on top of world objects." );
	dotText.SetPos( GAME_GUTTER, y );
	static const char* dot_TEXT[4] = { "Off", "On" };
	for( int i=0; i<2; ++i ) {
		dotButton[i].Init( &gamui2D, green );
		dotButton[i].SetText( dot_TEXT[i] );
		dotButton[i].SetPos( x + deltaX*(float)i, y );
		dotButton[i].SetSize( SIZE, SIZE );
		dotButton[0].AddToToggleGroup( &dotButton[i] );
	}
	dotButton[ sm->GetNumWalkingMaps()-1 ].SetDown();
	y += deltaY;

	debugText.Init( &gamui2D );
	debugText.SetSize( boxWidth, SIZE );
	debugText.SetText( "Debug Output\nLevel 1 displays framerate.\n(Setting doesn't save.)" );
	debugText.SetPos( GAME_GUTTER, y );
	static const char* DEBUG_TEXT[4] = { "Off", "1", "2", "3" };
	for( int i=0; i<4; ++i ) {
		debugButton[i].Init( &gamui2D, green );
		debugButton[i].SetText( DEBUG_TEXT[i] );
		debugButton[i].SetPos( x + deltaX*(float)i, y );
		debugButton[i].SetSize( SIZE, SIZE );
		debugButton[0].AddToToggleGroup( &debugButton[i] );
	}
	debugButton[game->GetDebugLevel()].SetDown();
	y += deltaY;

	audioButton.Init( &gamui2D, green );
	audioButton.SetSize( SIZE, SIZE );
	audioButton.SetPos( x, y );

	if ( sm->GetAudioOn() ) {
		audioButton.SetDown();
		audioButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_AUDIO, true ),
							 UIRenderer::CalcDecoAtom( DECO_AUDIO, false ) );	
	}
	else {
		audioButton.SetUp();
		audioButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_MUTE, true ),
							 UIRenderer::CalcDecoAtom( DECO_MUTE, false ) );	
	}
}


SettingScene::~SettingScene()
{
	SettingsManager* sm = SettingsManager::Instance();

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
}


void SettingScene::Activate()
{
	GetEngine()->SetMap( 0 );
}


GLString SettingScene::CalcMod( int delta )
{
	// Make sure the current mod is okay.
	const Game::ModDatabase* dbArr = game->GetModDatabases();
	GLString dbName = SettingsManager::Instance()->GetCurrentModName();

	int index = -1;
	int nDatabase = 0;
	for( nDatabase=0; nDatabase<GAME_MAX_MOD_DATABASES; ++nDatabase ) {
		if ( dbArr[nDatabase].id == 0 )
			break;
		GLString name;
		StrSplitFilename( dbArr[nDatabase].path, 0, &name, 0 );		
		if ( name == dbName ) {
			index = nDatabase;
		}
	}
	index += delta;
	if ( index >= nDatabase ) index = -1;
	if ( index < -1 ) index = nDatabase-1;

	GLString name = "";
	if ( index >= 0 ) {
		StrSplitFilename( dbArr[index].path, 0, &name, 0 );		
	}
	SettingsManager::Instance()->SetCurrentModName( name );
	//if ( name != dbName ) {
	//	game->LoadModDatabase( name.c_str() );
	//}
	return name;
}


void SettingScene::Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )
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

	if ( audioButton.Down() ) {
		audioButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_AUDIO, true ),
							 UIRenderer::CalcDecoAtom( DECO_AUDIO, false ) );	
	}
	else {
		audioButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_MUTE, true ),
							 UIRenderer::CalcDecoAtom( DECO_MUTE, false ) );	
	}

	GLString dbName = modCurrent.GetText();
	if ( item == &doneButton ) {
		game->PopScene();
	}
	else if ( item == &modDown ) {
		dbName = CalcMod( -1 );
	}
	else if ( item == &modUp ) {
		dbName = CalcMod( +1 );
	}

	if ( dbName == "" ) {
		dbName = "None";
	}
	modCurrent.SetText( dbName.c_str() );
}
