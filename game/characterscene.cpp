#include "characterscene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/uirendering.h"
#include "../engine/text.h"

using namespace grinliz;


CharacterScene::CharacterScene( Game* _game ) : Scene( _game )
{
	engine = &_game->engine;
	dragging = false;
	description = 0;

	Texture* t = game->GetTexture( "icons" );	
	decoTexture = game->GetTexture( "iconDeco" );	

	backWidget = new UIButtonBox( t, decoTexture, engine->GetScreenport() );
	charInvWidget = new UIButtonBox( t, decoTexture, engine->GetScreenport() );

	const CDynArray<ItemDef*>& itemDefs = game->GetItemDefArray();
	CDynArray<ItemDefInfo> info( itemDefs.Size() );
	for( unsigned i=0; i<info.Size(); ++i ) {
		info[i].tech = 1;
		info[i].count = 1;
	}

	storageWidget = new StorageWidget( t, decoTexture, engine->GetScreenport(), itemDefs, info );

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

	const int BUTX = 62;	// 61 is min
	const int BUTY = 60;
	{
		int icons[20] = { ICON_GREEN_BUTTON };
		charInvWidget->InitButtons( icons, Inventory::NUM_SLOTS+1 );
		charInvWidget->SetColumns( Inventory::DX );
		charInvWidget->SetOrigin( 5, 70 );
		// 5 letters...
		//charInvWidget->SetButtonSize( 10*4+16+5, 45 );
		charInvWidget->SetButtonSize( BUTX, BUTY );
		charInvWidget->SetPadding( 0, 0 );
		charInvWidget->SetAlpha( 0.8f );
	}
	storageWidget->SetOrigin( 230, 70 );
	storageWidget->SetButtonSize( BUTX, BUTY );
	//storageWidget->SetPadding( 0, 0 );
	SetInvWidget();
}


CharacterScene::~CharacterScene()
{
	delete backWidget;
	delete charInvWidget;
	delete storageWidget;
}




void CharacterScene::SetInvWidget()
{
	Inventory* inv = unit.GetInventory();

	// Top slots are general items.
	for( int i=2; i<Inventory::NUM_SLOTS; ++i ) 
	{
		int index = i+1;	//charInvWidget->TopIndex( i-2 );
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

	if ( item.IsSomething() ) {
		if ( item.IsWeapon() ) {
			const WeaponItemDef* wid = item.IsWeapon();
			if ( wid->weapon[1].shell )
				sprintf( buffer, "%d %d", item.Rounds(1), item.Rounds(2) );
			else
				sprintf( buffer, "%d", item.Rounds(1) );
			charInvWidget->SetText( index, item.Name(), buffer );
		}
		else if ( item.IsClip() ) {
			sprintf( buffer, "%d", item.Rounds() );
			charInvWidget->SetText( index, item.Name(), buffer );
		}
		else {
			charInvWidget->SetText( index, item.Name() );
		}
		charInvWidget->SetDeco( index, item.Deco() );
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
	storageWidget->Draw();

	if ( description ) {
		UFOText::Draw( 235, 50, "%s", description );
	}

	if ( dragging ) {
		const int SIZE = 30;

		Rectangle2F uv;
		UIRendering::GetDecoUV( currentDeco, &uv );
		Rectangle2I pos;
		pos.Set( dragPos.x-SIZE, dragPos.y-SIZE, dragPos.x+SIZE, dragPos.y+SIZE );
		UIRendering::DrawQuad( engine->GetScreenport(), pos, uv, decoTexture  );
	}
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

	storageWidget->Tap( ux, uy );

	description = "";
	if ( storageWidget->Tap( ux, uy ) ) {
		description = storageWidget->TappedItemDef()->desc;
	}
}


void CharacterScene::IndexType( int uiIndex, int* type, int* inventorySlot )
{
	if ( uiIndex >= 3 ) {
		*type = INV;
		*inventorySlot = uiIndex-1;
	}
	else if ( uiIndex == 0 ) {
		*type = ARMOR;
		*inventorySlot = Inventory::ARMOR_SLOT;
	}
	else if ( uiIndex == 1 ) {
		*type = UNLOAD;
	}
	else if ( uiIndex == 2 ) {
		*type = WEAPON;
		*inventorySlot = Inventory::WEAPON_SLOT;
	}
}


void CharacterScene::Drag( int action, const grinliz::Vector2I& screen )
{
	int ux, uy;
	engine->GetScreenport().ViewToUI( screen.x, screen.y, &ux, &uy );

	if ( action == GAME_DRAG_MOVE ) {
		dragPos.Set( ux, uy );
		return;
	}

	UIButtonBox* widget = 0;

	int tap = charInvWidget->QueryTap( ux, uy );
	if ( tap >= 0 ) {
		widget = charInvWidget;
	}

	Inventory* inv = unit.GetInventory();

	if ( action == GAME_DRAG_START && tap >= 0 ) {
		startWidget = widget;
		startTap = tap;
		
		int type, slot;
		IndexType( tap, &type, &slot );
		if ( type != UNLOAD ) {
			currentDeco = inv->GetDeco( slot );
			dragPos.Set( ux, uy );
			dragging = true;
		}
	}
	if ( action == GAME_DRAG_END ) {
		dragging = false;
		if ( tap >= 0 ) {
			if ( startWidget == widget && widget == charInvWidget ) {
				if ( startTap != tap ) {

					int startType, endType, startSlot, endSlot;

					IndexType( startTap, &startType, &startSlot );
					IndexType( tap, &endType, &endSlot );

					switch ( startType | (endType<<8) ) 
					{
						case ( INV    | (INV<<8 ) ):
						case ( INV    | (ARMOR<<8 ) ):
						case ( INV    | (WEAPON<<8 ) ):
						case ( ARMOR  | (INV<<8 ) ):
						case ( WEAPON | (INV<<8 ) ):
							{
								
								Item* startItem = inv->AccessItem( startSlot );
								Item* endItem = inv->AccessItem( endSlot );

								bool consumed = false;

								if (    startItem->IsSomething() 
									 && endItem->IsSomething() 
									 && endItem->Combine( startItem, &consumed ) ) 
								{
									if ( consumed == true )
										startItem->Clear();
								}
								else {
									inv->Swap( startSlot, endSlot );
								}
							}
							break;

						case ( WEAPON | (UNLOAD<<8) ):
						case ( INV    | (UNLOAD<<8) ):
							{
								Item* item = inv->AccessItem( startSlot );
								const WeaponItemDef* weaponDef = item->IsWeapon();
								if ( weaponDef ) {
									for( int i=2; i>=1; --i ) {
										if ( item->Rounds(i) > 0 && inv->IsGeneralSlotFree() ) {
											Item clip;
											item->RemovePart( i, &clip );
											inv->AddItem( Inventory::ANY_SLOT, clip );
										}
									}
								}
							}
							break;


						default:
							break;
					}
					SetInvWidget();
				}
			}
		}
	}
	unit.UpdateInventory();
}

