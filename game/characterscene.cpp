#include "characterscene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/uirendering.h"


CharacterScene::CharacterScene( Game* _game ) : Scene( _game )
{
	engine = &_game->engine;

	Texture* t = game->GetTexture( "icons" );	
	widgets = new UIButtonBox( t, engine->GetScreenport() );

	int icons[] = { UIButtonBox::ICON_PLAIN };
	const char* iconText[] = { "back" };
	widgets->SetButtons( icons, 1 );
	widgets->SetText( iconText );
	engine->EnableMap( false );
}


CharacterScene::~CharacterScene()
{
	delete widgets;
}


void CharacterScene::DrawHUD()
{
	widgets->Draw();
}

void CharacterScene::Tap(	int count, 
							const grinliz::Vector2I& screen,
							const grinliz::Ray& world )
{
	game->PopScene();
}