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

	helpButton.Init( &gamui2D, green );
	helpButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	helpButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_HELP, true ),  UIRenderer::CalcDecoAtom( DECO_HELP, false ) );

	gamui::UIItem* controlArr[NUM_CONTROL+1] = { &helpButton };
	static const char* const controlLabel[NUM_CONTROL] = { "Inv", "Stats", "Comp" };
	for( int i=0; i<NUM_CONTROL; ++i ) {
		control[i].Init( &gamui2D, 2, green );
		control[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		control[i].SetText( controlLabel[i] );
		controlArr[i+1] = &control[i];
	}

	static const char* const rangeLabel[NUM_RANGE] = { "4m", "8m", "16m" };
	for( int i=0; i<NUM_RANGE; ++i ) {
		range[i].Init( &gamui2D, 3, blue );
		range[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		range[i].SetText( rangeLabel[i] );
		range[i].SetVisible( false );
		if ( i == 1 ) 
			range[i].SetDown();
		else 
			range[i].SetUp();
	}

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

	gamui::Gamui::Layout( itemArr, NUM_BASE_BUTTONS, 3, 2, 0, 0, charInvButton[0].Width()*3.f, charInvButton[0].Height()*2.f );
	charInvButton[NUM_BASE_BUTTONS+0].SetPos( charInvButton[0].X(), charInvButton[0].Y()+charInvButton[0].Height()*2.f );
	charInvButton[NUM_BASE_BUTTONS+1].SetPos( charInvButton[2].X(), charInvButton[2].Y()+charInvButton[2].Height()*2.f );
	
	this->unit = unit;

	Vector2I mapPos;
	unit->CalcMapPos( &mapPos, 0 );
	storage = engine->GetMap()->LockStorage( mapPos.x, mapPos.y );
	if ( !storage ) {
		storage = new Storage( game->GetItemDefArr() );
	}

	storageWidget = new StorageWidget( &gamui2D, green, blue, _game->GetItemDefArr(), storage );
	engine->EnableMap( false );
	storageWidget->SetOrigin( (float)port.UIWidth()-storageWidget->Width(), 0 );
	InitInvWidget();
	InitTextTable( &gamui2D );
	InitCompTable( &gamui2D );

	gamui::Gamui::Layout( controlArr, NUM_CONTROL+1, NUM_CONTROL+1, 1, storageWidget->X(), (float)(port.UIHeight()-GAME_BUTTON_SIZE), storageWidget->Width(), GAME_BUTTON_SIZE_F );
	for( int i=0; i<NUM_RANGE; ++i ) {
		range[i].SetPos( controlArr[i+1]->X(), controlArr[i+1]->Y()-GAME_BUTTON_SIZE_F );
	}
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


void CharacterScene::InitTextTable( gamui::Gamui* g )
{
	float y=0;
	float x=storageWidget->X();
	float dy = 20.0f;
	float dx = 100.0f;
	char buf[32];
	const Stats& stats = unit->GetStats();
	int count=0;

	nameRankUI.Init( g );
	nameRankUI.Set( x, y, unit, false );
	nameRankUI.SetVisible( false );

	for( int i=0; i<2*STATS_ROWS; ++i ) {
		textTable[i].Init( g );
		textTable[i].SetVisible( false );
	}
	for( int i=0; i<STATS_ROWS; ++i ) {
		textTable[i*2+0].SetPos( x, y + (float)(i+1) * dy );
		textTable[i*2+1].SetPos( x+dx, y + (float)(i+1) * dy );
	}

	int c = 0;
	textTable[c++].SetText( "STR" );
	SNPrintf( buf, 32, "%d", stats.STR() );
	textTable[c++].SetText( buf );

	textTable[c++].SetText( "DEX" );
	SNPrintf( buf, 32, "%d", stats.DEX() );
	textTable[c++].SetText( buf );

	textTable[c++].SetText( "PSY" );
	SNPrintf( buf, 32, "%d", stats.PSY() );
	textTable[c++].SetText( buf );

	textTable[c++].SetText( "HP" );
	SNPrintf( buf, 32, "%d/%d", unit->HP(), stats.TotalHP() );
	textTable[c++].SetText( buf );

	textTable[c++].SetText( "TU" );
	SNPrintf( buf, 32, "%0.1f/%0.1f", unit->TU(), stats.TotalTU() );
	textTable[c++].SetText( buf );

	textTable[c++].SetText( "Accuracy" );
	SNPrintf( buf, 32, "%0.2f", 1.0f - stats.Accuracy() );
	textTable[c++].SetText( buf );

	textTable[c++].SetText( "Reaction" );
	SNPrintf( buf, 32, "%0.2f", stats.Reaction() );
	textTable[c++].SetText( buf );

	textTable[c++].SetText( "Kills" );
	SNPrintf( buf, 32, "%d", unit->KillsCredited() );
	textTable[c++].SetText( buf );

	GLASSERT( c <= 2*STATS_ROWS );
}


void CharacterScene::InitCompTable( gamui::Gamui* g )
{
	float x=storageWidget->X();
	float dy = 20.0f;
	float y=0;		// skip nameRankUI

	float nameWidth = 70.0f;

	for( int i=0; i<COMP_COL*COMP_ROW; ++i ) {
		compTable[i].Init( g );
		compTable[i].SetVisible( false );
	}
	float delta = (storageWidget->Width() - nameWidth) / (COMP_COL-1);

	for( int j=0; j<COMP_ROW; ++j ) {
		for( int i=0; i<COMP_COL; ++i ) {
			float rx = x;

			if ( i>0 )
				rx = x + nameWidth + delta*(i-1);

			compTable[j*COMP_COL+i].SetPos( rx, y + (float)(j+1)*dy );
		}
	}

	compTable[0].SetText( "Name" );
	compTable[1].SetText( "TU" );
	compTable[2].SetText( "%" );
	compTable[3].SetText( "D" );
	compTable[4].SetText( "D/TU" );
}


void CharacterScene::SetCompText()
{
	ItemDef* const* itemDefArr = game->GetItemDefArr();
	const Inventory* inventory = unit->GetInventory();
	const Stats& stats = unit->GetStats();
	int index = 1;

	float r = 8.0f;
	if ( range[0].Down() )
		r = 4.0f;
	else if ( range[2].Down() )
		r = 16.0f;

	for( int i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( itemDefArr[i] && itemDefArr[i]->IsWeapon() ) {
			bool inInventory = false;
			bool onGround = false;

			if ( inventory->Contains( itemDefArr[i] ) )
				inInventory = true;
			if ( storage->Contains( itemDefArr[i] ) ) 
				onGround = true;

			if ( inInventory || onGround ) {
				const WeaponItemDef* wid = itemDefArr[i]->IsWeapon();
				WeaponMode mode = kAutoFireMode;
		
				DamageDesc dd;
				wid->DamageBase( mode, &dd );

				float fraction, fraction2, damage, dptu;
				wid->FireStatistics( mode, stats.Accuracy(), r, &fraction, &fraction2, &damage, &dptu );

				if ( fraction2 > 0.90f )
					fraction2 = 0.90f;

				char buf[16];

				SNPrintf( buf, 16, "%s%s", inInventory ? "+" : "-", wid->name );
				compTable[index*COMP_COL + 0].SetText( buf );

				SNPrintf( buf, 16, "%.1f", wid->TimeUnits( mode ) );
				compTable[index*COMP_COL + 1].SetText( buf );

				SNPrintf( buf, 16, "%d", LRintf( fraction2 * 100.f ) );
				compTable[index*COMP_COL + 2].SetText( buf );

				SNPrintf( buf, 16, "%d", LRintf( damage ) );
				compTable[index*COMP_COL + 3].SetText( buf );

				SNPrintf( buf, 16, "%.1f", dptu );
				compTable[index*COMP_COL + 4].SetText( buf );

				++index;
				if ( index == COMP_ROW )
					break;
			}
		}
	}
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


void CharacterScene::SwitchMode( int mode )
{
	storageWidget->SetVisible( mode == INVENTORY );

	nameRankUI.SetVisible( mode != INVENTORY );

	bool statsVisible = ( mode == STATS );
	for( int i=0; i<2*STATS_ROWS; ++i ) {
		textTable[i].SetVisible( statsVisible );
	}

	bool compVisible = ( mode == COMPARE );
	for( int i=0; i<COMP_COL*COMP_ROW; ++i ) {
		compTable[i].SetVisible( compVisible );
	}
	for( int i=0; i<NUM_RANGE; ++i )
		range[i].SetVisible( compVisible );

	if ( mode == COMPARE )
		SetCompText();
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

			if ( wid->HasWeapon(kAutoFireMode) )
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

	// control buttons
	if ( item == &control[0] )
		SwitchMode( INVENTORY );
	else if ( item == &control[1] )
		SwitchMode( STATS );
	else if ( item == &control[2] )
		SwitchMode( COMPARE );

	if ( item >= &range[0] && item < &range[NUM_RANGE] )
		SetCompText();

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
	Item item = inv->GetItem( slot );
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


