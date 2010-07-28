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
	const Screenport& port = engine->GetScreenport();

	// 248, 228, 8
	const float INV = 1.f/255.f;
	uiRenderer.SetTextColor( 248.f*INV, 228.f*INV, 8.f*INV );

	background.Init( &gamui2D, game->GetRenderAtom( Game::ATOM_TACTICAL_BACKGROUND ) );
	background.SetSize( game->engine->GetScreenport().UIWidth(), game->engine->GetScreenport().UIHeight() );
	
	RenderAtom nullAtom;
	image.Init( &gamui2D, nullAtom );
	image.SetForeground( true );

	currentScreen = 0;

	textBox.Init( &gamui2D );

	const float GUTTER = 20.0f;
	textBox.SetPos( GUTTER, GUTTER );
	textBox.SetSize( port.UIWidth()-GUTTER*2.0f, port.UIHeight()-GUTTER*2.0f );

	const ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );
	static const char* const text[3] = { "<", ">", "X" };
	UIItem* items[3] = { 0 };

	for( int i=0; i<3; ++i ) {
		buttons[i].Init( &gamui2D, blue );
		buttons[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		buttons[i].SetText( text[i] );
		items[i] = &buttons[i];
	}
	buttons[0].SetPos( port.UIWidth() - GAME_BUTTON_SIZE_F*2.0f, port.UIHeight() - GAME_BUTTON_SIZE_F );
	buttons[1].SetPos( port.UIWidth() - GAME_BUTTON_SIZE_F,		 port.UIHeight() - GAME_BUTTON_SIZE_F );
	buttons[2].SetPos( 0, port.UIHeight() - GAME_BUTTON_SIZE_F );
	/*
	Gamui::Layout( items, 3, 3, 1, 
				   (float)engine->GetScreenport().UIWidth()-GAME_BUTTON_SIZE_F*3.0f, 
				   (float)engine->GetScreenport().UIHeight()-GAME_BUTTON_SIZE_F, 
				   GAME_BUTTON_SIZE_F*3.0f, 
				   GAME_BUTTON_SIZE_F );
	*/
	Layout();
}



HelpScene::~HelpScene()
{
	uiRenderer.SetTextColor( 1.0f, 1.0, 1.0 );
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

	const float GUTTER = 20.0f;
	
	if ( pageItem->HasAttribute( full.c_str() ) ) {
		text = (const char*)reader->AccessData( pageItem, full.c_str(), 0 );
	}
	else {
		text = (const char*)reader->AccessData( pageItem, "text", 0 );
	}
	const Screenport& port = game->engine->GetScreenport();
	textBox.SetText( text ? text : "" );
	textBox.SetPos( GUTTER, GUTTER );
	float tw = port.UIWidth() - GUTTER*2.0f;
	float th = buttons[0].Y() - GUTTER*2.0f;
	image.SetVisible( false );

	if ( pageItem->HasAttribute( "image" ) ) {
		const char* imageName = pageItem->GetString( "image" );
		const Texture* texture = TextureManager::Instance()->GetTexture( imageName );
		GLASSERT( texture );
		RenderAtom atom(	(const char*) UIRenderer::RENDERSTATE_UI_NORMAL,
							(const char*) texture,
							0, 0, 1, 1, (float)texture->Width(), (float)texture->Height() );
		image.SetAtom( atom );
		image.SetPos( port.UIWidth()-texture->Width(), 0 );
		image.SetSize( atom.srcWidth, atom.srcHeight );
		tw = image.X()-GUTTER;
		image.SetVisible( true );
	}
	textBox.SetSize( tw, th );
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
		--currentScreen;	
	}
	else if ( item == &buttons[1] ) {
		++currentScreen;	
	}
	else if ( item == &buttons[2] ) {
		game->PopScene();
	}
	Layout();
}
