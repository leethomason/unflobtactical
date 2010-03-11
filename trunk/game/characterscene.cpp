#include "characterscene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/uirendering.h"
#include "../engine/text.h"
#include "battlestream.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

const int BUTX = 62;	// 61 is min
const int BUTY = 60;



CharacterScene::CharacterScene( Game* _game, CharacterSceneInput* input ) 
	: Scene( _game )
{
	unit = input->unit;
	canChangeArmor = input->canChangeArmor;
	delete input;
	input = 0;

	engine = &_game->engine;
	description = 0;

	backWidget = new UIButtonBox( engine->GetScreenport() );
	charInvWidget = new UIButtonGroup( engine->GetScreenport() );

	this->unit = unit;
	//engine->SoloRender( unit );

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

	savedCamera = engine->camera;
	engine->camera.SetPosWC( model->Pos() - eyeDir[0]*10.0f + offset );

	storageWidget->SetOrigin( 230, 70 );
	storageWidget->SetButtonSize( BUTX, BUTY );
	InitInvWidget();
}


CharacterScene::~CharacterScene()
{
	//engine->SoloRender( 0 );
	engine->EnableMap( true );
	engine->camera = savedCamera;

	delete backWidget;
	delete charInvWidget;
	delete storageWidget;

	Vector2I mapPos;
	unit->CalcMapPos( &mapPos, 0 );
	engine->GetMap()->ReleaseStorage( mapPos.x, mapPos.y, storage, game->GetItemDefArr() );
	storage = 0;

	//BattleSceneStream bss( game );
	//bss.Save( selectedUnit, units, &savedCamera, engine->GetMap() );
}




void CharacterScene::InitInvWidget()
{
	Inventory* inv = unit->GetInventory();

	int icons[20] = { ICON_GREEN_BUTTON };

	charInvWidget->InitButtons( icons, Inventory::NUM_SLOTS+1 );
	charInvWidget->SetOrigin( 5, 70 );
	charInvWidget->SetButtonSize( BUTX, BUTY );
	charInvWidget->SetPadding( 0, 0 );
	charInvWidget->SetAlpha( 0.8f );

	// swap
	charInvWidget->SetPos( 0, BUTX*3/2, 0 );
	charInvWidget->SetButton( 0, ICON_BLUE_BUTTON );
	charInvWidget->SetDeco( 0, DECO_SWAP );

	// armor
	charInvWidget->SetPos( 1, 0, BUTY );
	charInvWidget->SetButton( 1, ICON_RED_BUTTON );
	charInvWidget->SetDeco( 1, DECO_ARMOR );
	
	// weapons
	for( int i=0; i<2; ++i ) {
		charInvWidget->SetPos( 2+i, BUTX*(i+1), BUTY );
		charInvWidget->SetButton( 2+i, ICON_RED_BUTTON );
		charInvWidget->SetDeco( 2+i, DECO_PISTOL );
	}

	// general
	for( int i=0; i<6; ++i ) {
		charInvWidget->SetPos( 4+i, BUTX*(i%3), BUTY*(2+i/3) );
		charInvWidget->SetButton( 4+i, ICON_GREEN_BUTTON );
		charInvWidget->SetDeco( 4+i, DECO_NONE );
	}

	SetAllButtonGraphics();
}


void CharacterScene::SetAllButtonGraphics()
{
	Inventory* inventory = unit->GetInventory();

	SetButtonGraphics( 1, inventory->GetItem( Inventory::ARMOR_SLOT ) );
	SetButtonGraphics( 2, inventory->GetItem( Inventory::WEAPON_SLOT_SECONDARY ) );
	SetButtonGraphics( 3, inventory->GetItem( Inventory::WEAPON_SLOT_PRIMARY ) );
	for( int i=0; i<6; ++i ) {
		SetButtonGraphics( 4+i, inventory->GetItem( Inventory::GENERAL_SLOT+i ) );
	}
}


void CharacterScene::SetButtonGraphics( int index, const Item& item )
{
	char buffer[16];

	if ( item.IsSomething() ) {

		if ( item.IsWeapon() ) {
			const WeaponItemDef* wid = item.IsWeapon();
			Inventory* inventory = unit->GetInventory();

			int rounds[2] = { 0, 0 };
			rounds[0] = inventory->CalcClipRoundsTotal( wid->weapon[0].clipItemDef );
			rounds[1] = inventory->CalcClipRoundsTotal( wid->weapon[1].clipItemDef );

			if ( wid->HasWeapon(1) )
				SNPrintf( buffer, 16, "%d %d", rounds[0], rounds[1] );
			else
				SNPrintf( buffer, 16, "%d", rounds[0] );
			charInvWidget->SetText( index, item.Name(), buffer );
		}
		else if ( item.Rounds() > 1 ) {
			SNPrintf( buffer, 16, "%d", item.Rounds() );
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
}



void CharacterScene::DrawHUD()
{
	backWidget->Draw();
	charInvWidget->Draw();
	storageWidget->Draw();

	if ( description ) {
		UFOText::Draw( 235, 50, "%s", description );
	}
}


void CharacterScene::Tap(	int count, 
							const grinliz::Vector2I& screen,
							const grinliz::Ray& world )
{
	int ux, uy;
	engine->GetScreenport().ViewToUI( screen.x, screen.y, &ux, &uy );

	// Back button:
	int tap = backWidget->QueryTap( ux, uy );
	if ( tap >= 0 ) {
		game->PopScene();
		return;
	}

	// Inventory of the character:
	tap = charInvWidget->QueryTap( ux, uy );
	if ( tap == 0 ) {
		Inventory* inventory = unit->GetInventory();
		inventory->SwapWeapons();
	}
	else if ( tap > 0 ) {
		if ( tap == 1 )
			InventoryToStorage( Inventory::ARMOR_SLOT );
		else if ( tap == 2 )
			InventoryToStorage( Inventory::WEAPON_SLOT_SECONDARY );
		else if ( tap == 3 )
			InventoryToStorage( Inventory::WEAPON_SLOT_PRIMARY );
		else
			InventoryToStorage( Inventory::GENERAL_SLOT + (tap-4) );
	}

	const ItemDef* itemDef = storageWidget->Tap( ux, uy );
	if ( itemDef ) {
		StorageToInventory( itemDef );
	}

	if ( tap >= 0 || itemDef ) {
		storageWidget->Update();
		SetAllButtonGraphics();
		unit->UpdateInventory();
	}
}


void CharacterScene::InventoryToStorage( int slot )
{
	// Always succeeds in the inv->storage directory
	Inventory* inv = unit->GetInventory();
	const Item& item = inv->GetItem( slot );
	description = 0;

	if ( item.IsSomething() ) {
		description = item.GetItemDef()->desc;
		if ( item.IsArmor() && !canChangeArmor ) {
			// do nothing.
		}
		else {
			storage->AddItem( item );
			inv->RemoveItem( slot );
		}
	}
}


void CharacterScene::StorageToInventory( const ItemDef* itemDef )
{
	description = 0;

	if ( itemDef ) {
		description = itemDef->desc;

		Item item;
		storage->RemoveItem( itemDef, &item );

		if ( item.IsArmor() && !canChangeArmor ) {
			// put it back
			storage->AddItem( item );
		}
		else {
			Inventory* inv = unit->GetInventory();
			if ( inv->AddItem( item ) < 0 ) {
				// Couldn't add to inventory. Return to storage.
				storage->AddItem( item );
			}
		}
	}
}


