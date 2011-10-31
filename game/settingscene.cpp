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

	const float SIZE = gamui2D.GetTextHeight()*2.8f;
	float y = GAME_GUTTER / 4;
	float deltaY = SIZE + 2.0f;
	float x = 196.0f + GAME_GUTTER*2.f;
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
	GLString modName = CalcMod( 0 );
	modCurrent.SetText( modName.size() ? modName.c_str() : "None" );

	modUp.Init( &gamui2D, green );
	modUp.SetSize( SIZE, SIZE );
	modUp.SetPos( x+deltaX*3.f, y );
	modUp.SetDeco( UIRenderer::CalcDecoAtom( DECO_NEXT, true ),
				   UIRenderer::CalcDecoAtom( DECO_NEXT, false ) );
	y += deltaY;

	moveText.Init( &gamui2D );
	moveText.SetSize( boxWidth, SIZE );
	moveText.SetText( "Confirm movement (for small screens.)" );
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
	debugText.SetText( "Debug Output\nLevel 1 displays framerate." );
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

	dragText.Init( &gamui2D );
	dragText.SetSize( boxWidth, SIZE );
	dragText.SetText( "Drag to move units." );
	dragText.SetPos( GAME_GUTTER, y );
	for( int i=0; i<2; ++i ) {
		dragButton[i].Init( &gamui2D, green );
		dragButton[i].SetText( i==0 ? "Off" : "On" );
		dragButton[i].SetPos( x + deltaX*(float)i, y );
		dragButton[i].SetSize( SIZE, SIZE );
	}
	dragButton[0].AddToToggleGroup( &dragButton[1] );
	dragButton[ sm->GetAllowDrag() ? 1 : 0 ].SetDown();
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
	sm->SetAllowDrag( dragButton[1].Down() );
}


void SettingScene::Activate()
{
	GetEngine()->SetMap( 0 );
}


GLString SettingScene::CalcMod( int delta )
{
	SettingsManager* sm = SettingsManager::Instance();

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
	SettingsManager* sm = SettingsManager::Instance();

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
