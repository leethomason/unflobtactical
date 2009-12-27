#include "characterscene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/uirendering.h"
#include "../engine/text.h"
#include "battlestream.h"

using namespace grinliz;


CharacterScene::CharacterScene( Game* _game ) 
	: Scene( _game )
{
	engine = &_game->engine;
	dragging = false;
	description = 0;
	memset( units, 0, sizeof(Unit)*MAX_UNITS );

	backWidget = new UIButtonBox( engine->GetScreenport() );
	charInvWidget = new UIButtonBox( engine->GetScreenport() );

	BattleSceneStream bss( game );
	bss.Load( &selectedUnit, units, false, &savedCamera, game->engine.GetMap() );

	// Hide all the models but selected.
	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( i != selectedUnit ) {
			if ( units[i].GetModel() )
				units[i].GetModel()->SetFlag( Model::MODEL_INVISIBLE );
			if ( units[i].GetWeaponModel() )
				units[i].GetWeaponModel()->SetFlag( Model::MODEL_INVISIBLE );
		}
	}

	GLASSERT( selectedUnit < MAX_UNITS );	// else how did we get here?
	unit = &units[selectedUnit];

	Vector2I mapPos;
	unit->CalcMapPos( &mapPos, 0 );
	storage = engine->GetMap()->LockStorage( mapPos.x, mapPos.y );
	if ( !storage ) {
		storage = new Storage();
	}
	storageWidget = new StorageWidget( engine->GetScreenport(), _game->GetItemDefArr(), storage );

	{
		int icons[] = { ICON_GREEN_BUTTON };
		const char* iconText[] = { "back" };
		backWidget->InitButtons( icons, 1 );
		backWidget->SetOrigin( 5, 5 );
		backWidget->SetText( iconText );
	}
	engine->EnableMap( false );


	Model* model = unit->GetModel();
	const Vector3F* eyeDir = engine->camera.EyeDir3();
	Vector3F offset = { 0.0f, model->AABB().SizeY()*0.5f, 0.0f };
	engine->camera.SetPosWC( model->Pos() - eyeDir[0]*10.0f + offset );

	//unit.SetYRotation( -18.0f );

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

	Vector2I mapPos;
	unit->CalcMapPos( &mapPos, 0 );
	engine->GetMap()->ReleaseStorage( mapPos.x, mapPos.y, storage );
	storage = 0;

	BattleSceneStream bss( game );
	bss.Save( selectedUnit, units, &savedCamera, engine->GetMap() );
}




void CharacterScene::SetInvWidget()
{
	Inventory* inv = unit->GetInventory();

	// Top slots are general items.
	// 9, 10, 11: general (slow)
	// 6, 7, 8: general (medium)
	// 3, 4, 5: general (fast)
	// 0: armor		1: drop		2: weapon
	//
	for( int i=2; i<Inventory::NUM_SLOTS; ++i ) 
	{
		int index = i+1;
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
		/*if ( item.IsWeapon() && item.GetItemDef(1) == 0 ) {
			// melee weapon.
			charInvWidget->SetText( index, item.Name(), " " );		
		}
		else*/
		if ( item.IsWeapon() ) {
			const WeaponItemDef* wid = item.IsWeapon();
			if ( wid->HasWeapon(1) )
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

		const Texture* decoTexture = TextureManager::Instance()->GetTexture( "iconDeco" );
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

	const ItemDef* itemDef = storageWidget->Tap( ux, uy );
	description = itemDef ? itemDef->desc : "";
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


void CharacterScene::StartDragging( const ItemDef* itemDef )
{
	currentDeco = 0;
	if ( itemDef ) {
		currentDeco = itemDef->deco;
		dragging = true;
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
	Vector2I ui = { ux, uy };

	int tap = charInvWidget->QueryTap( ux, uy );
	const ItemDef* tappedItemDef = storageWidget->Tap( ux, uy );
	Rectangle2I storageBounds;
	storageWidget->CalcBounds( &storageBounds );

	Inventory* inv = unit->GetInventory();

	if ( action == GAME_DRAG_START ) {
		if ( tappedItemDef ) {
			
			startInvTap = -1;
			startItemDef = tappedItemDef;

			StartDragging( tappedItemDef );
			dragPos.Set( ux, uy );
		}
		else if ( tap >= 0 ) {
			startInvTap = tap;
			startItemDef = 0;
			
			int type, slot;
			IndexType( tap, &type, &slot );
			if ( type != UNLOAD ) {
				StartDragging( inv->GetItemDef( slot ) );
				dragPos.Set( ux, uy );
			}
		}
		else {
			startInvTap = -1;
			startItemDef = 0;
		}
	}
	if ( action == GAME_DRAG_END ) {
		dragging = false;
		if ( tap >= 0 && startInvTap >= 0) {
			DragInvToInv( startInvTap, tap );
		}
		else if ( tap >=0 && startItemDef ) {
			DragStorageToInv( startItemDef, tap );
		}
		else if ( startInvTap >= 0 &&  storageBounds.Contains( ui ) ) {
			DragInvToStorage( startInvTap );
		}
		startInvTap = -1;
		startItemDef = 0;
	}
	unit->UpdateInventory();
}


void CharacterScene::DragStorageToInv( const ItemDef* startItemDef, int endTap )
{
	Inventory* inv = unit->GetInventory();
	if ( !inv->IsSlotFree( startItemDef ) ) {
		// can't move.
		return;
	}

	int endType, endSlot;
	IndexType( endTap, &endType, &endSlot );
	if ( endType == UNLOAD ) {
		return;
	}

	Item item;
	storage->RemoveItem(	startItemDef, &item );
	GLASSERT( item.IsSomething() );
	GLASSERT( item.Rounds() );

	bool added = inv->AddItem( endSlot, item );
	if ( !added ) {
		added = inv->AddItem( -1, item );
	}
	GLASSERT( added );

	SetInvWidget();
	storageWidget->Update();
}


void CharacterScene::DragInvToStorage( int startTap )
{
	Inventory* inv = unit->GetInventory();
	int startType, startSlot=-1;
	IndexType( startTap, &startType, &startSlot );
	if ( startType == UNLOAD ) {
		return;
	}

	Item* item = inv->AccessItem( startSlot );

	if ( item->IsNothing() )
		return;

	storage->AddItem( *item );
	item->Clear();

	SetInvWidget();
	storageWidget->Update();
}


void CharacterScene::DragInvToInv( int startTap, int endTap )
{
	if ( startTap == endTap )
		return;

	int startType, endType, startSlot, endSlot;
	Inventory* inv = unit->GetInventory();

	IndexType( startTap, &startType, &startSlot );
	IndexType( endTap, &endType, &endSlot );

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
