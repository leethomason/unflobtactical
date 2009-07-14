#include "characterscene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/uirendering.h"

using namespace grinliz;


CharacterScene::CharacterScene( Game* _game ) : Scene( _game )
{
	engine = &_game->engine;

	Texture* t = game->GetTexture( "icons" );	
	Texture* t2 = game->GetTexture( "iconDeco" );	
	widgets = new UIButtonBox( t, t2, engine->GetScreenport() );
	charInvWidget = new UIButtonBox( t, t2, engine->GetScreenport() );

	{
		int icons[] = { ICON_GREEN_BUTTON };
		const char* iconText[] = { "back" };
		widgets->InitButtons( icons, 1 );
		widgets->SetOrigin( 5, 5 );
		widgets->SetText( iconText );
	}
	engine->EnableMap( false );

	UFOStream* stream = game->OpenStream( "SingleUnit" );
	unit.Load( stream, &_game->engine, _game );


	Model* model = unit.GetModel();

	const Vector3F* eyeDir = engine->camera.EyeDir3();
	Vector3F offset = { 0.0f, model->AABB().SizeY()*0.5f, 0.0f };

	engine->camera.SetPosWC( model->Pos() - eyeDir[0]*10.0f + offset );
	//model->SetYRotation( -18.0f );
	unit.SetYRotation( -18.0f );

	{
		int icons[20] = { ICON_GREEN_BUTTON };
		charInvWidget->InitButtons( icons, Inventory::NUM_SLOTS );
		charInvWidget->SetColumns( Inventory::DX );
		charInvWidget->SetOrigin( 5, 70 );
		// 5 letters...
		charInvWidget->SetButtonSize( 10*4+16+5, 45 );
		charInvWidget->SetPadding( 0, 0 );
		charInvWidget->SetAlpha( 0.8f );

	}
	SetInvWidgetText();
}


CharacterScene::~CharacterScene()
{
	delete widgets;
	delete charInvWidget;
}


void CharacterScene::SetInvWidgetText()
{
	Inventory* inv = unit.GetInventory();
	for( int i=0; i<Inventory::NUM_SLOTS; ++i ) {

		int index = charInvWidget->TopIndex( (i+1)%Inventory::NUM_SLOTS );

		const Item& item = inv->GetItem( index );

		if ( !item.None() ) {
			charInvWidget->SetText( i, item.itemDef->name );
			charInvWidget->SetDeco( i, item.itemDef->deco );
		}
		else {
			charInvWidget->SetText( i, 0 );
			charInvWidget->SetDeco( i, DECO_NONE );
		}
		int id = ICON_BLUE_BUTTON;	// size1
		int size = inv->GetSlotSize(i);
		if ( size == 2 )
			id = ICON_GREEN_BUTTON;
		else if ( size == 3 )
			id = ICON_RED_BUTTON;
		charInvWidget->SetButton( i, id );
	}
}

void CharacterScene::DrawHUD()
{
	widgets->Draw();
	charInvWidget->Draw();
}

void CharacterScene::Tap(	int count, 
							const grinliz::Vector2I& screen,
							const grinliz::Ray& world )
{
	game->PopScene();
}