/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "characterscene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/uirendering.h"
#include "../engine/text.h"
#include "battlestream.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;


CharacterScene::CharacterScene( Game* _game, CharacterSceneInput* input ) 
	: Scene( _game )
{
	unit = input->unit;
	delete input;
	input = 0;
	mode = INVENTORY_MODE;

	engine = _game->engine;
	description = 0;

	controlButtons = new UIButtonGroup( engine->GetScreenport() );
	charInvWidget = new UIButtonGroup( engine->GetScreenport() );

	int widths[2] = { 16, 16 };
	textTable = new UITextTable( engine->GetScreenport(), 2, 16, widths );

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
		int icons[] = { ICON_GREEN_BUTTON, ICON_GREEN_BUTTON };
		const char* iconText[] = { "Back", "Stats" };
		controlButtons->InitButtons( icons, 2 );
		controlButtons->SetButtonSize( GAME_BUTTON_SIZE, GAME_BUTTON_SIZE );
		controlButtons->SetText( iconText );

		Vector2I size = controlButtons->GetButtonSize();
		controlButtons->SetPos( 0, 5, 5 );
		controlButtons->SetPos( 1, engine->GetScreenport().UIWidth() - 5 - size.x, 5 );
	}
	engine->EnableMap( false );


	Model* model = unit->GetModel();
	const Vector3F* eyeDir = engine->camera.EyeDir3();
	Vector3F offset = { 0.0f, model->AABB().SizeY()*0.5f, 0.0f };

	savedCamera = engine->camera;
	engine->camera.SetPosWC( model->Pos() - eyeDir[0]*10.0f + offset );

	storageWidget->SetOrigin( 230, 70 );
	storageWidget->SetButtonSize( GAME_BUTTON_SIZE, GAME_BUTTON_SIZE );

	InitInvWidget();
	InitTextTable();
}


CharacterScene::~CharacterScene()
{
	//engine->SoloRender( 0 );
	engine->EnableMap( true );
	engine->camera = savedCamera;

	delete controlButtons;
	delete charInvWidget;
	delete storageWidget;
	delete textTable;

	Vector2I mapPos;
	unit->CalcMapPos( &mapPos, 0 );
	engine->GetMap()->ReleaseStorage( mapPos.x, mapPos.y, storage, game->GetItemDefArr() );
	storage = 0;
}


void CharacterScene::InitTextTable()
{
	textTable->SetOrigin( GAME_BUTTON_SIZE*4, GAME_BUTTON_SIZE*5-5 );

	textTable->SetText( 0, 0, unit->FirstName() );
	textTable->SetText( 1, 0, unit->LastName() );

	textTable->SetText( 0, 1, "Rank" );
	textTable->SetText( 1, 1, unit->Rank() );

	int row=2;
	char buf[32];

	const Stats& stats = unit->GetStats();
	textTable->SetText( 0, row, "STR" );
	textTable->SetInt( 1, row++, stats.STR() );

	textTable->SetText( 0, row, "DEX" );
	textTable->SetInt( 1, row++, stats.DEX() );

	textTable->SetText( 0, row, "PSY" );
	textTable->SetInt( 1, row++, stats.PSY() );

	SNPrintf( buf, 32, "%d/%d", unit->HP(), stats.TotalHP() );
	textTable->SetText( 0, row, "HP" );
	textTable->SetText( 1, row++, buf );

	SNPrintf( buf, 32, "%.1f/%.1f", unit->TU(), stats.TotalTU() );
	textTable->SetText( 0, row, "TU" );
	textTable->SetText( 1, row++, buf );

	textTable->SetText( 0, row, "Accuracy" );
	textTable->SetFloat( 1, row++, 1.0f - stats.Accuracy() );	// keep consistent: show as higher is better.

	textTable->SetText( 0, row, "Reaction" );
	textTable->SetFloat( 1, row++, stats.Reaction() );

	textTable->SetText( 0, row, "Kills" );
	textTable->SetInt( 1, row, unit->KillsCredited() );
}


void CharacterScene::InitInvWidget()
{
	//Inventory* inv = unit->GetInventory();

	int icons[20] = { ICON_GREEN_BUTTON };

	charInvWidget->InitButtons( icons, Inventory::NUM_SLOTS+1 );
	charInvWidget->SetOrigin( 5, 70 );
	charInvWidget->SetButtonSize( GAME_BUTTON_SIZE, GAME_BUTTON_SIZE );
	charInvWidget->SetPadding( 0, 0 );
	charInvWidget->SetAlpha( 0.8f );

	// swap
	charInvWidget->SetPos( 0, GAME_BUTTON_SIZE*3/2, 0 );
	charInvWidget->SetButton( 0, ICON_BLUE_BUTTON );
	charInvWidget->SetDeco( 0, DECO_SWAP );

	// armor
	charInvWidget->SetPos( 1, 0, GAME_BUTTON_SIZE );
	charInvWidget->SetButton( 1, ICON_RED_BUTTON );
	charInvWidget->SetDeco( 1, DECO_ARMOR );
	
	// weapons
	for( int i=0; i<2; ++i ) {
		charInvWidget->SetPos( 2+i, GAME_BUTTON_SIZE*(i+1), GAME_BUTTON_SIZE );
		charInvWidget->SetButton( 2+i, ICON_RED_BUTTON );
		charInvWidget->SetDeco( 2+i, DECO_PISTOL );
	}

	// general
	for( int i=0; i<6; ++i ) {
		charInvWidget->SetPos( 4+i, GAME_BUTTON_SIZE*(i%3), GAME_BUTTON_SIZE*(2+i/3) );
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
	controlButtons->Draw();
	charInvWidget->Draw();
	if ( mode == INVENTORY_MODE )
		storageWidget->Draw();
	else
		textTable->Draw();

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
	int tap = controlButtons->QueryTap( ux, uy );
	if ( tap == 0 ) {
		game->PopScene();
		return;
	}
	else if ( tap ==1 ) {
		mode++;
		if ( mode == MODE_COUNT ) 
			mode = 0;
		if ( mode == INVENTORY_MODE )
			controlButtons->SetText( 1, "Stats" );
		if ( mode == STATS_MODE )
			controlButtons->SetText( 1, "Inv" );
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
		storage->AddItem( item );
		inv->RemoveItem( slot );
	}
}


void CharacterScene::StorageToInventory( const ItemDef* itemDef )
{
	description = 0;

	if ( itemDef ) {
		description = itemDef->desc;

		Item item;
		storage->RemoveItem( itemDef, &item );

		Inventory* inv = unit->GetInventory();
		if ( inv->AddItem( item ) < 0 ) {
			// Couldn't add to inventory. Return to storage.
			storage->AddItem( item );
		}
	}
}


