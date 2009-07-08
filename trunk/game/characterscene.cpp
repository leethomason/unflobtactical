#include "characterscene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/uirendering.h"

using namespace grinliz;


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

	UFOStream* stream = game->OpenStream( "SingleUnit" );
	unit.Load( stream, &_game->engine, _game );


	Model* model = unit.GetModel();

	const Vector3F* eyeDir = engine->camera.EyeDir3();
	Vector3F offset = { 0.0f, model->AABB().SizeY()*0.5f, 0.0f };

	engine->camera.SetPosWC( model->Pos() - eyeDir[0]*10.0f + offset );
	//model->SetYRotation( -18.0f );
	unit.SetYRotation( -18.0f );
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