#include "helpscene.h"
#include "game.h"
#include "../engine/engine.h"
#include "../engine/uirendering.h"


HelpScene::HelpScene( Game* _game ) : Scene( _game )
{
	Engine* engine = GetEngine();
	engine->EnableMap( false );
	currentScreen = 0;

	for( int i=0; i<NUM_SCREENS; ++i ) {
		screens[i] = new UIImage( engine->GetScreenport() );
		screens[i]->SetOrigin( 0, engine->GetScreenport().UIHeight() - 512 );
	}
	screens[0]->Init( TextureManager::Instance()->GetTexture( "help0" ), 512, 512 );
	screens[1]->Init( TextureManager::Instance()->GetTexture( "help1" ), 512, 512 );
	screens[2]->Init( TextureManager::Instance()->GetTexture( "help2" ), 512, 512 );
	screens[3]->Init( TextureManager::Instance()->GetTexture( "help3" ), 512, 512 );
	screens[4]->Init( TextureManager::Instance()->GetTexture( "help4" ), 512, 512 );

	buttons = new UIButtonBox( engine->GetScreenport() );
	buttons->SetColumns( 3 );

	int icons[NUM_BUTTONS] = { ICON_BLUE_BUTTON, ICON_BLUE_BUTTON, ICON_BLUE_BUTTON };

	const char* iconText[] = { "<", ">", "X" };
	buttons->InitButtons( icons, NUM_BUTTONS );
	buttons->SetOrigin( 300, 5 );
	buttons->SetText( iconText );
}



HelpScene::~HelpScene()
{
	for( int i=0; i<NUM_SCREENS; ++i ) {
		delete screens[i];
	}
	delete buttons;
}



void HelpScene::DrawHUD()
{
	screens[currentScreen]->Draw();
	buttons->Draw();
}


void HelpScene::Tap( int count, const grinliz::Vector2I& screen, const grinliz::Ray& world )
{
	int ux, uy;
	GetEngine()->GetScreenport().ViewToUI( screen.x, screen.y, &ux, &uy );

	int tap = buttons->QueryTap( ux, uy );

	// Want to keep re-using main texture. Do a ContextShift() if anything
	// will change on this screen.
	TextureManager* texman = TextureManager::Instance();

	switch ( tap ) {
		case 0:		--currentScreen;	texman->ContextShift();		break;
		case 1:		++currentScreen;	texman->ContextShift();		break;
		case 2:		game->PopScene();								break;

		default:
			break;
	}
	while( currentScreen < 0 )				currentScreen += NUM_SCREENS;
	while( currentScreen >= NUM_SCREENS )	currentScreen -= NUM_SCREENS;
}
