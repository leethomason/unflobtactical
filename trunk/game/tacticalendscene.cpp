#include "tacticalendscene.h"
#include "../engine/uirendering.h"
#include "../engine/engine.h"
#include "../engine/text.h"
#include "game.h"

#include <string>

TacticalEndScene::TacticalEndScene( Game* _game, const TacticalEndSceneData* d ) : Scene( _game )
{
	Engine* engine = GetEngine();
	data = d;

	engine->EnableMap( false );
	background = new UIImage( engine->GetScreenport() );

	// -- Background -- //
	const Texture* bg = TextureManager::Instance()->GetTexture( "intro" );
	GLASSERT( bg );
	background->Init( bg, 480, 320 );

	/*
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
	*/
}


TacticalEndScene::~TacticalEndScene()
{
	GetEngine()->EnableMap( true );
	delete background;
}


void TacticalEndScene::DrawHUD()
{
	background->Draw();
	UFOText::Draw( 50, 200, "%s", data->nTerransAlive ? "Victory" : "Defeat" );
	UFOText::Draw( 50, 180, "Soldiers survived        %2d", data->nTerransAlive );
	UFOText::Draw( 50, 160, "Soldiers killed          %2d", data->nTerrans - data->nTerransAlive );
	UFOText::Draw( 50, 140, "Aliens survived          %2d", data->nAliensAlive );
	UFOText::Draw( 50, 120, "Aliens killed            %2d", data->nAliens - data->nAliensAlive );

	UFOText::Draw( 50, 80,  "For a new game, close and select 'new'" );
}
