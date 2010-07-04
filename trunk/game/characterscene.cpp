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
using namespace gamui;

CharacterScene::CharacterScene( Game* _game, CharacterSceneInput* input ) 
	: Scene( _game )
{
	unit = input->unit;
	delete input;
	input = 0;
	mode = INVENTORY_MODE;

	engine = _game->engine;
	const Screenport& port = _game->engine->GetScreenport();
	description = 0;

	//controlButtons = new UIButtonGroup( engine->GetScreenport() );
	const gamui::ButtonLook& green = game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& blue  = game->GetButtonLook( Game::BLUE_BUTTON );
	const gamui::ButtonLook& red   = game->GetButtonLook( Game::RED_BUTTON );

	backButton.Init( &gamui2D, green );
	backButton.SetPos( 0, port.UIHeight()-backButton.Height() );
	backButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	backButton.SetText( "Back" );

	gamui::UIItem* itemArr[NUM_BASE_BUTTONS];
	for( int i=0; i<NUM_BASE_BUTTONS; ++i ) {
		charInvButton[i].Init( &gamui2D, green );
		charInvButton[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		itemArr[i] = &charInvButton[i];
	}
	for( int i=NUM_BASE_BUTTONS; i<NUM_INV_BUTTONS; ++i ) {
		charInvButton[i].Init( &gamui2D, red );
		charInvButton[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	}
	gamui::Gamui::Layout( itemArr, NUM_BASE_BUTTONS, 3, 2, 0, 0, charInvButton[0].Width()*3.f, charInvButton[0].Height()*2.f, 0 );
	charInvButton[NUM_BASE_BUTTONS+0].SetPos( charInvButton[0].X(), charInvButton[0].Y()+charInvButton[0].Height()*2.f );
	charInvButton[NUM_BASE_BUTTONS+1].SetPos( charInvButton[2].X(), charInvButton[2].Y()+charInvButton[2].Height()*2.f );
	
	this->unit = unit;

	Vector2I mapPos;
	unit->CalcMapPos( &mapPos, 0 );
	storage = engine->GetMap()->LockStorage( mapPos.x, mapPos.y );
	if ( !storage ) {
		storage = new Storage();
	}
	storageWidget = new StorageWidget( &gamui2D, green, blue, _game->GetItemDefArr(), storage );

	engine->EnableMap( false );
	storageWidget->SetOrigin( 230, 0 );
	InitInvWidget();
	InitTextTable();
}


CharacterScene::~CharacterScene()
{
	unit->UpdateInventory();
	engine->EnableMap( true );
	delete storageWidget;

	Vector2I mapPos;
	unit->CalcMapPos( &mapPos, 0 );
	engine->GetMap()->ReleaseStorage( mapPos.x, mapPos.y, storage, game->GetItemDefArr() );
	storage = 0;
}


void CharacterScene::InitTextTable()
{
/*	textTable->SetOrigin( GAME_BUTTON_SIZE*4, GAME_BUTTON_SIZE*5-5 );

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
*/
}


void CharacterScene::InitInvWidget()
{
	// armor
	const gamui::RenderAtom& atomArmor = uiRenderer.CalcDecoAtom( DECO_ARMOR, false );
	charInvButton[ARMOR_BUTTON].SetDeco( atomArmor, atomArmor );

	const gamui::RenderAtom& atomWeapon = uiRenderer.CalcDecoAtom( DECO_PISTOL, false );
	charInvButton[WEAPON_BUTTON].SetDeco( atomWeapon, atomWeapon );

	SetAllButtonGraphics();
}


void CharacterScene::SetAllButtonGraphics()
{
	Inventory* inventory = unit->GetInventory();

	SetButtonGraphics( ARMOR_BUTTON, inventory->GetItem( Inventory::ARMOR_SLOT ) );
	SetButtonGraphics( WEAPON_BUTTON, inventory->GetItem( Inventory::WEAPON_SLOT ) );
	for( int i=0; i<NUM_BASE_BUTTONS; ++i ) {
		SetButtonGraphics( i, inventory->GetItem( Inventory::GENERAL_SLOT+i ) );
	}
}


void CharacterScene::SetButtonGraphics( int index, const Item& item )
{
	if ( item.IsSomething() ) {
		char buffer[16] = { 0 };

		if ( item.IsWeapon() ) {
			const WeaponItemDef* wid = item.IsWeapon();
			Inventory* inventory = unit->GetInventory();

			int rounds[2] = { 0, 0 };
			rounds[0] = inventory->CalcClipRoundsTotal( wid->weapon[0].clipItemDef );
			rounds[1] = inventory->CalcClipRoundsTotal( wid->weapon[1].clipItemDef );

			if ( wid->HasWeapon(kModeAlt) )
				SNPrintf( buffer, 16, "R%d %d", rounds[0], rounds[1] );
			else
				SNPrintf( buffer, 16, "R%d", rounds[0] );

		}
		else if ( item.IsClip() ) {
			SNPrintf( buffer, 16, "%d", item.Rounds() );
		}
		charInvButton[index].SetDeco( UIRenderer::CalcDecoAtom( item.Deco(), false ), UIRenderer::CalcDecoAtom( item.Deco() , false ) );
		charInvButton[index].SetText( item.Name() );
		charInvButton[index].SetText2( buffer );
	}
	else {
		charInvButton[index].SetText( " " );
		charInvButton[index].SetText2( " " );
		gamui::RenderAtom nullAtom;
		charInvButton[index].SetDeco( nullAtom, nullAtom );
	}
}



void CharacterScene::DrawHUD()
{
}


void CharacterScene::Tap(	int count, 
							const grinliz::Vector2I& screen,
							const grinliz::Ray& world )
{
	float ux, uy;
	engine->GetScreenport().ViewToGUI( screen.x, screen.y, &ux, &uy );

	const UIItem* item = gamui2D.Tap( ux, uy );

	if ( item == &backButton ) {
		game->PopScene();
		return;
	}

	// Inventory of the character:
	if ( item >= &charInvButton[0] && item < &charInvButton[NUM_BASE_BUTTONS] ) {
		int offset = (const PushButton*) item - charInvButton;
		GLASSERT( offset >= 0 && offset < NUM_BASE_BUTTONS );

		InventoryToStorage( Inventory::GENERAL_SLOT + offset );
	}
	else if ( item == &charInvButton[WEAPON_BUTTON] ) {
		InventoryToStorage( Inventory::WEAPON_SLOT );
	}
	else if ( item == &charInvButton[ARMOR_BUTTON] ) {
		InventoryToStorage( Inventory::ARMOR_SLOT );
	}


	const ItemDef* itemDef = storageWidget->ConvertTap( item );
	if ( itemDef ) {
		StorageToInventory( itemDef );
	}
	SetAllButtonGraphics();
	storageWidget->SetButtons();
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


