#include "helpscene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/engine.h"
#include "../engine/uirendering.h"
#include "../grinliz/glstringutil.h"

using namespace gamui;

HelpScene::HelpScene( Game* _game ) : Scene( _game )
{
	Engine* engine = GetEngine();
	engine->EnableMap( false );
	currentScreen = 0;

	for( int i=0; i<NUM_TEXT_LABELS; ++i ) {
		label[i].Init( &gamui2D );
		label[i].SetPos( 0, (float)(i*20) );
		labelPtr[i] = &label[i];
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
	Layout();
}



HelpScene::~HelpScene()
{
}


void HelpScene::Layout()
{
	const gamedb::Reader* reader = game->GetDatabase();
	const gamedb::Item* rootItem = reader->Root();
	const gamedb::Item* textItem = rootItem->Child( "text" );
	const gamedb::Item* helpItem = textItem->Child( "tacticalHelp" );

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
	if ( text ) {
		gamui2D.LayoutTextBlock( text, labelPtr, NUM_TEXT_LABELS, 0, 0, 500 );
	}
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
	TextureManager* texman = TextureManager::Instance();

	if ( item == &buttons[0] ) {
//		screens[currentScreen].SetVisible( false );
		--currentScreen;	
//		texman->ContextShift();
	}
	else if ( item == &buttons[1] ) {
//		screens[currentScreen].SetVisible( false );
		++currentScreen;	
//		texman->ContextShift();
	}
	else if ( item == &buttons[2] ) {
		game->PopScene();
	}
//	while( currentScreen < 0 )				currentScreen += NUM_SCREENS;
//	while( currentScreen >= NUM_SCREENS )	currentScreen -= NUM_SCREENS;
//	screens[currentScreen].SetVisible( true );
}
