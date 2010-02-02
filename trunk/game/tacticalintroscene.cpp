#include "tacticalintroscene.h"
#include "../engine/uirendering.h"
#include "../engine/engine.h"
#include "game.h"

#include <string>

TacticalIntroScene::TacticalIntroScene( Game* _game ) : Scene( _game )
{
	showNewChoices = false;

	Engine* engine = GetEngine();
	background = new UIImage( engine->GetScreenport() );

	// -- Background -- //
	const Texture* bg = TextureManager::Instance()->GetTexture( "intro" );
	GLASSERT( bg );
	background->Init( bg, 480, 320 );

	// -- Buttons -- //
	{
		buttons = new UIButtonBox( engine->GetScreenport() );

		int icons[] = { ICON_GREEN_BUTTON, ICON_GREEN_BUTTON, ICON_GREEN_BUTTON };
		const char* iconText[] = { "test", "new", "continue" };
		buttons->InitButtons( icons, 3 );
		buttons->SetOrigin( 42, 40 );
		buttons->SetButtonSize( 120, 50 );
		buttons->SetText( iconText );
	}

	// -- New Game choices -- //
	{
		choices = new UIButtonGroup( engine->GetScreenport() );
		
		const int XSIZE = 55;
		const int YSIZE = 55;
		int icons[] = { ICON_GREEN_BUTTON, ICON_GREEN_BUTTON_DOWN,
						ICON_GREEN_BUTTON, ICON_GREEN_BUTTON_DOWN, ICON_GREEN_BUTTON,
						ICON_GREEN_BUTTON, ICON_GREEN_BUTTON_DOWN,
						ICON_GREEN_BUTTON, ICON_GREEN_BUTTON_DOWN, ICON_GREEN_BUTTON,
						ICON_GREEN_BUTTON_DOWN, ICON_GREEN_BUTTON,
						ICON_BLUE_BUTTON
		};
		const char* iconText[] = {
						"4", "8",
						"Low", "Med", "Hi",
						"8", "16",
						"Low", "Med", "Hi",
						"Day", "Ngt",
						"GO!"
		};

		choices->SetOrigin( 280, 40 );

		choices->InitButtons( icons, 13 );
		choices->SetText( iconText );

		choices->SetPos( SQUAD_4, XSIZE*0, YSIZE*4 );
		choices->SetPos( SQUAD_8, XSIZE*1, YSIZE*4 );

		choices->SetPos( TERRAN_LOW, XSIZE*0, YSIZE*3 );
		choices->SetPos( TERRAN_MED, XSIZE*1, YSIZE*3 );
		choices->SetPos( TERRAN_HIGH, XSIZE*2, YSIZE*3 );

		choices->SetPos( ALIEN_8, XSIZE*0, YSIZE*2 );
		choices->SetPos( ALIEN_16, XSIZE*1, YSIZE*2 );

		choices->SetPos( ALIEN_LOW, XSIZE*0, YSIZE*1 );
		choices->SetPos( ALIEN_MED, XSIZE*1, YSIZE*1 );
		choices->SetPos( ALIEN_HIGH, XSIZE*2, YSIZE*1 );

		choices->SetPos( TIME_DAY, XSIZE*0, YSIZE*0 );
		choices->SetPos( TIME_NIGHT, XSIZE*1, YSIZE*0 );

		choices->SetPos( GO_NEW_GAME, XSIZE*25/10, -YSIZE*5/10 );
	}

	// Is there a current game?
	const std::string& savePath = game->GameSavePath();
	buttons->SetEnabled( CONTINUE_GAME, false );
	FILE* fp = fopen( savePath.c_str(), "r" );
	if ( fp ) {
		fseek( fp, 0, SEEK_END );
		unsigned long len = ftell( fp );
		if ( len > 100 ) {
			// 20 ignores empty XML noise (hopefully)
			buttons->SetEnabled( CONTINUE_GAME, true );
		}
		fclose( fp );
	}
}


TacticalIntroScene::~TacticalIntroScene()
{
	delete background;
	delete buttons;
	delete choices;
}


void TacticalIntroScene::DrawHUD()
{
	background->Draw();
	buttons->Draw();
	if ( showNewChoices ) {
		choices->Draw();
	}
}


void TacticalIntroScene::Tap(	int count, 
								const grinliz::Vector2I& screen,
								const grinliz::Ray& world )
{
	int ux, uy;
	GetEngine()->GetScreenport().ViewToUI( screen.x, screen.y, &ux, &uy );
	int tap = buttons->QueryTap( ux, uy );

	switch ( tap ) {
		case TEST_GAME:			game->loadRequested = 2;			break;
		case NEW_GAME:			game->loadRequested = 1;			break;
		case CONTINUE_GAME:		game->loadRequested = 0;			break;

		default:
			break;
	}

	if ( tap >= 0 ) {
		game->PopScene();
		game->PushScene( Game::BATTLE_SCENE, 0 );
	}
}
