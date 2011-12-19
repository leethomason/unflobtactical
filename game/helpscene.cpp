#include "helpscene.h"
#include "game.h"
#include "cgame.h"
#include "saveloadscene.h"
#include "../engine/engine.h"
#include "../engine/uirendering.h"
#include "../grinliz/glstringutil.h"

using namespace gamui;

HelpScene::HelpScene( Game* _game, const HelpSceneData* data ) : Scene( _game ), helpName( data->name )
{
	Engine* engine = GetEngine();
	const Screenport& port = engine->GetScreenport();

	// 248, 228, 8
	const float INV = 1.f/255.f;
	uiRenderer.SetTextColor( 248.f*INV, 228.f*INV, 8.f*INV );

	background.Init( &gamui2D, game->GetRenderAtom( Game::ATOM_TACTICAL_BACKGROUND ), false );
	background.SetSize( game->engine->GetScreenport().UIWidth(), game->engine->GetScreenport().UIHeight() );
	
	RenderAtom nullAtom;
	image.Init( &gamui2D, nullAtom, true );

	currentScreen = 0;

	textBox.Init( &gamui2D );

	textBox.SetPos( GAME_GUTTER, GAME_GUTTER );
	textBox.SetSize( port.UIWidth()-GAME_GUTTER*2.0f, port.UIHeight()-GAME_GUTTER*2.0f );

	const ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );
	//static const char* const text[NUM_BUTTONS] = { "<", ">", "X", "" };
	static const int deco[NUM_BUTTONS] = { DECO_BUTTON_PREV, DECO_BUTTON_NEXT, DECO_OKAY_CHECK, DECO_NONE };
	UIItem* items[NUM_BUTTONS] = { 0 };

	for( int i=0; i<NUM_BUTTONS; ++i ) {
		buttons[i].Init( &gamui2D, blue );
		buttons[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		//buttons[i].SetText( text[i] );
		buttons[i].SetDeco( Game::CalcDecoAtom( deco[i], true ), 
							Game::CalcDecoAtom( deco[i], false ) );
		items[i] = &buttons[i];
	}
	buttons[PREV_BUTTON].SetPos( port.UIWidth() - GAME_BUTTON_SIZE_F*2.0f, port.UIHeight() - GAME_BUTTON_SIZE_F );
	buttons[NEXT_BUTTON].SetPos( port.UIWidth() - GAME_BUTTON_SIZE_F,		 port.UIHeight() - GAME_BUTTON_SIZE_F );
	buttons[DONE_BUTTON].SetPos( 0, port.UIHeight() - GAME_BUTTON_SIZE_F );
	buttons[SETTINGS_BUTTON].SetPos( port.UIWidth() - GAME_BUTTON_SIZE_F*3.0f, port.UIHeight() - GAME_BUTTON_SIZE_F );
	buttons[SAVE_LOAD_BUTTON].SetPos( port.UIWidth() - GAME_BUTTON_SIZE_F*4.f, port.UIHeight() - GAME_BUTTON_SIZE_F );

	buttons[SETTINGS_BUTTON].SetDeco( Game::CalcDecoAtom( DECO_SETTINGS, true ), Game::CalcDecoAtom( DECO_SETTINGS, false ) );
	buttons[SETTINGS_BUTTON].SetVisible( data->settings );

	buttons[SAVE_LOAD_BUTTON].SetDeco( Game::CalcDecoAtom( DECO_SAVE_LOAD, true ), Game::CalcDecoAtom( DECO_SAVE_LOAD, false ) );
	buttons[SAVE_LOAD_BUTTON].SetVisible( data->settings );

	Layout();
}



HelpScene::~HelpScene()
{
	uiRenderer.SetTextColor( 1.0f, 1.0, 1.0 );
}


void HelpScene::Activate()
{
	GetEngine()->SetMap( 0 );
}



void HelpScene::Layout()
{
	const gamedb::Reader* reader = game->GetDatabase();
	const gamedb::Item* rootItem = reader->Root();
	GLASSERT( rootItem );
	const gamedb::Item* textItem = rootItem->Child( "text" );
	GLASSERT( textItem );
	const gamedb::Item* helpItem = textItem->Child( helpName );
	GLASSERT( helpItem );

	int nPages = helpItem->NumChildren();
	while( currentScreen < 0 ) currentScreen += nPages;
	while( currentScreen >= nPages ) currentScreen -= nPages;

	const gamedb::Item* pageItem = helpItem->Child( currentScreen );
	const char* text = 0;

	grinliz::GLString full = "text_";
	full += PlatformName();

	if ( pageItem->HasAttribute( full.c_str() ) ) {
		text = (const char*)reader->AccessData( pageItem, full.c_str(), 0 );
	}
	else {
		text = (const char*)reader->AccessData( pageItem, "text", 0 );
	}
	const Screenport& port = game->engine->GetScreenport();
	textBox.SetText( text ? text : "" );
	textBox.SetPos( GAME_GUTTER, GAME_GUTTER );
	float tw = port.UIWidth() - GAME_GUTTER*2.0f;
	float th = buttons[PREV_BUTTON].Y() - GAME_GUTTER*2.0f;
	image.SetVisible( false );

	if ( pageItem->HasAttribute( "image" ) ) {
		const char* imageName = pageItem->GetString( "image" );
		const Texture* texture = TextureManager::Instance()->GetTexture( imageName );
		GLASSERT( texture );
		
		float width = 100.0f;
		float height = width / texture->AspectRatio();
		if ( height > 320.f - GAME_BUTTON_SIZE_F - GAME_GUTTER ) {
			height = 320.f - GAME_BUTTON_SIZE_F - GAME_GUTTER;
			width = height * texture->AspectRatio();
		}

		RenderAtom atom(	(const char*) UIRenderer::RENDERSTATE_UI_NORMAL,
							(const char*) texture,
							0, 0, 1, 1 );
		image.SetAtom( atom );
		image.SetPos( port.UIWidth()-width, 0 );
		image.SetSize( width, height );
		tw = image.X()-GAME_GUTTER;
		image.SetVisible( true );
	}
	textBox.SetSize( tw, th );

	buttons[PREV_BUTTON].SetEnabled( currentScreen > 0 );
	buttons[NEXT_BUTTON].SetEnabled( currentScreen < nPages - 1 );
}


void HelpScene::DrawHUD()
{
}


void HelpScene::Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )
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

	// Want to keep re-using main texture. Do a ContextShift() if anything
	// will change on this screen.
	//TextureManager* texman = TextureManager::Instance();

	if ( item == &buttons[PREV_BUTTON] ) {
		--currentScreen;	
	}
	else if ( item == &buttons[NEXT_BUTTON] ) {
		++currentScreen;	
	}
	else if ( item == &buttons[DONE_BUTTON] ) {
		game->PopScene();
	}
	else if ( item == &buttons[SETTINGS_BUTTON] ) {
		game->PushScene( Game::SETTING_SCENE, 0 );
	}
	else if ( item == &buttons[SAVE_LOAD_BUTTON] ) {
		game->PushScene( Game::SAVE_LOAD_SCENE, new SaveLoadSceneData( true ) ); 
	}
	Layout();
}


void HelpScene::HandleHotKeyMask( int mask )
{
	if ( mask == GAME_HK_NEXT_UNIT  ) {
		++currentScreen;
	}
	else if ( mask == GAME_HK_PREV_UNIT ) {
		--currentScreen;
	}
	Layout();
}
