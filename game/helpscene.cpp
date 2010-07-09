#include "helpscene.h"
#include "game.h"
#include "../engine/engine.h"
#include "../engine/uirendering.h"

using namespace gamui;

HelpScene::HelpScene( Game* _game ) : Scene( _game )
{
	Engine* engine = GetEngine();
	engine->EnableMap( false );
	currentScreen = 0;

	static const char* const textures[NUM_SCREENS] = { "help0", "help1", "help2", "help3", "help4" };

	for( int i=0; i<NUM_SCREENS; ++i ) {
		RenderAtom atom;

		atom.Init( UIRenderer::RENDERSTATE_NORMAL, TextureManager::Instance()->GetTexture( textures[i] ), 0, 0, 1, 1, 480, 320 );
		UIRenderer::SetAtomCoordFromPixel( 0, 0, 480, 320, 512, 512, &atom );

		screens[i].Init( &gamui2D, atom );
		screens[i].SetVisible( i==0 );
	}

	const ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );
	static const char* const text[3] = { "<", ">", "X" };
	UIItem* items[3] = { 0 };

	for( int i=0; i<3; ++i ) {
		buttons[i].Init( &gamui2D, blue );
		buttons[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		buttons[i].SetText( text[i] );
		items[i] = &buttons[i];
	}
	Gamui::Layout( items, 3, 3, 1, 
				   (float)engine->GetScreenport().UIWidth()-GAME_BUTTON_SIZE_F*3.0f, 
				   (float)engine->GetScreenport().UIHeight()-GAME_BUTTON_SIZE_F, 
				   GAME_BUTTON_SIZE_F*3.0f, 
				   GAME_BUTTON_SIZE_F );
}



HelpScene::~HelpScene()
{
}



void HelpScene::DrawHUD()
{
}


void HelpScene::Tap( int count, const grinliz::Vector2F& screen, const grinliz::Ray& world )
{
	grinliz::Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( screen, &ui );

	const UIItem* tap = gamui2D.Tap( ui.x, ui.y );

	// Want to keep re-using main texture. Do a ContextShift() if anything
	// will change on this screen.
	TextureManager* texman = TextureManager::Instance();

	if ( tap == &buttons[0] ) {
		screens[currentScreen].SetVisible( false );
		--currentScreen;	
		texman->ContextShift();
	}
	else if ( tap == &buttons[1] ) {
		screens[currentScreen].SetVisible( false );
		++currentScreen;	
		texman->ContextShift();
	}
	else if ( tap == &buttons[2] ) {
		game->PopScene();
	}
	while( currentScreen < 0 )				currentScreen += NUM_SCREENS;
	while( currentScreen >= NUM_SCREENS )	currentScreen -= NUM_SCREENS;
	screens[currentScreen].SetVisible( true );
}
