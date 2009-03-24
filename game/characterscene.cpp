#include "characterscene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/uirendering.h"


CharacterScene::CharacterScene( Game* _game ) : Scene( _game )
{
	engine = &_game->engine;

	Texture* t = game->GetTexture( "icons" );	
	widgets = new UIButtonBox( t );

	int icons[] = { UIButtonBox::ICON_PLAIN };
	const char* iconText[] = { "back" };
	widgets->SetButtons( icons, iconText, 1 );
}


CharacterScene::~CharacterScene()
{
	delete widgets;
}


void CharacterScene::Activate()
{
	engine->EnableMap( false );
}


void CharacterScene::DrawHUD()
{
	int h = engine->Width();
	int w = engine->Height();
	int rotation = game->Rotation();
	widgets->Draw( w, h, rotation );
}

void CharacterScene::Tap(	int count, 
							const grinliz::Vector2I& screen,
							const grinliz::Ray& world )
{
	game->PopScene();
}