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
	backWidget = new UIButtonBox( t, t2, engine->GetScreenport() );
	charInvWidget = new UIButtonBox( t, t2, engine->GetScreenport() );

	{
		int icons[] = { ICON_GREEN_BUTTON };
		const char* iconText[] = { "back" };
		backWidget->InitButtons( icons, 1 );
		backWidget->SetOrigin( 5, 5 );
		backWidget->SetText( iconText );
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
	delete backWidget;
	delete charInvWidget;
}


void CharacterScene::SetInvWidgetText()
{
	Inventory* inv = unit.GetInventory();

	// Top slots are general items.
	for( int i=2; i<Inventory::NUM_SLOTS; ++i ) {

		int index = charInvWidget->TopIndex( i-2 );
		const Item& item = inv->GetItem( i );
		SetButtonGraphics( index, item );
	}

	// Botton slots are special.
	// Armor, Drop, Weapon
	SetButtonGraphics( 0, inv->GetItem( Inventory::ARMOR_SLOT ) );
	charInvWidget->SetDeco( 0, DECO_ARMOR );
	charInvWidget->SetButton( 0, ICON_RED_BUTTON );

	charInvWidget->SetButton( 1, ICON_RED_BUTTON );
	charInvWidget->SetDeco( 1, DECO_UNLOAD );

	SetButtonGraphics( 2, inv->GetItem( Inventory::WEAPON_SLOT ) );
	charInvWidget->SetButton( 2, ICON_RED_BUTTON );
}


void CharacterScene::SetButtonGraphics( int index, const Item& item )
{
	char buffer[16];

	if ( !item.None() ) {
		if ( item.itemDef->IsWeapon() || item.itemDef->IsClip() ) {
			sprintf( buffer, "%d", item.rounds[0] );
			charInvWidget->SetText( index, item.itemDef->name, buffer );
		}
		else {
			charInvWidget->SetText( index, item.itemDef->name );
		}
		charInvWidget->SetDeco( index, item.itemDef->deco );
	}
	else {
		charInvWidget->SetText( index, 0 );
		charInvWidget->SetDeco( index, DECO_NONE );
	}
	charInvWidget->SetButton( index, ICON_GREEN_BUTTON );
}


void CharacterScene::DrawHUD()
{
	backWidget->Draw();
	charInvWidget->Draw();
}

void CharacterScene::Tap(	int count, 
							const grinliz::Vector2I& screen,
							const grinliz::Ray& world )
{
	int ux, uy;
	engine->GetScreenport().ViewToUI( screen.x, screen.y, &ux, &uy );

	int tap = backWidget->QueryTap( ux, uy );
	if ( tap >= 0 ) {
		game->PopScene();
	}
}


void CharacterScene::Drag( int action, const grinliz::Vector2I& screen )
{
	if ( action == GAME_DRAG_MOVE )
		return;

	int ux, uy;
	engine->GetScreenport().ViewToUI( screen.x, screen.y, &ux, &uy );

	UIButtonBox* widget = 0;

	int tap = charInvWidget->QueryTap( ux, uy );
	if ( tap >= 0 ) {
		widget = charInvWidget;
	}

	if ( action == GAME_DRAG_START ) {
		startWidget = widget;
		startTap = tap;
	}
	else {
		/*
		if ( startWidget == widget && widget == charInvWidget ) {
			if ( startTap != tap ) {
				int i0

				// Swap items:
				Inventory* inv = unit.GetInventory();
				grinliz::Swap( &inv->
			}
		}
		*/
	}
}

