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
#include "battlestream.h"
#include "inventoryWidget.h"
#include "helpscene.h"
#include "tacmap.h"

#include "../grinliz/glstringutil.h"

#include "../engine/uirendering.h"
#include "../engine/text.h"

using namespace grinliz;
using namespace gamui;

CharacterScene::CharacterScene( Game* _game, CharacterSceneData* _input ) 
	: Scene( _game ),
	  input( _input )
{
	unit = input->unit;
	storage = _input->storage;
	currentUnit = 0;

	engine = _game->engine;
	const Screenport& port = _game->engine->GetScreenport();

	const gamui::ButtonLook& red		= game->GetButtonLook( Game::RED_BUTTON );
	const gamui::ButtonLook& green		= game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& blue		= game->GetButtonLook( Game::BLUE_BUTTON );
	const gamui::ButtonLook& blueTab	= game->GetButtonLook( Game::BLUE_TAB_BUTTON );

	backButton.Init( &gamui2D, blue );
	backButton.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	backButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	backButton.SetText( "Back" );

	prevButton.Init( &gamui2D, blue );
	prevButton.SetPos( GAME_BUTTON_SIZE_F, port.UIHeight()-GAME_BUTTON_SIZE_F );
	prevButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	prevButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_PREV, true ),  UIRenderer::CalcDecoAtom( DECO_PREV, false ) );
	prevButton.SetVisible( input->nUnits > 1 );

	nextButton.Init( &gamui2D, blue );
	nextButton.SetPos( GAME_BUTTON_SIZE_F*2.0f, port.UIHeight()-GAME_BUTTON_SIZE_F );
	nextButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	nextButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_NEXT, true ),  UIRenderer::CalcDecoAtom( DECO_NEXT, false ) );
	nextButton.SetVisible( input->nUnits > 1 );

	helpButton.Init( &gamui2D, blue );
	helpButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	helpButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_HELP, true ),  UIRenderer::CalcDecoAtom( DECO_HELP, false ) );

	gamui::UIItem* controlArr[NUM_CONTROL+1] = { &helpButton };
	static const char* const controlLabel[NUM_CONTROL] = { "Inv", "Stats", "Wpns" };
	for( int i=0; i<NUM_CONTROL; ++i ) {
		control[i].Init( &gamui2D, blue );
		if ( i > 0 ) 
			control[0].AddToToggleGroup( &control[i] );
		control[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		control[i].SetText( controlLabel[i] );
		controlArr[i+1] = &control[i];
	}
	
	this->unit = unit;

	inventoryWidget = new InventoryWidget( game, &gamui2D, red, green, unit );

	storageWidget = new StorageWidget( &gamui2D, green, blueTab, _game->GetItemDefArr(), _input->storage );
	storageWidget->SetOrigin( (float)port.UIWidth()-storageWidget->Width(), 0 );
	statWidget.Init( &gamui2D, unit, storageWidget->X(), 0, input->nUnits > 1 );
	compWidget.Init( &game->GetItemDefArr(), storage, unit, &gamui2D, blue, storageWidget->X(), 0, storageWidget->Width() );

	unitCounter.Init( &gamui2D );
	unitCounter.SetPos( 180, 0 );
	unitCounter.SetVisible( false );
	SetCounter( 0 );

	gamui::Gamui::Layout( controlArr, NUM_CONTROL+1, NUM_CONTROL+1, 1, storageWidget->X(), (float)(port.UIHeight()-GAME_BUTTON_SIZE), storageWidget->Width(), GAME_BUTTON_SIZE_F );
}


CharacterScene::~CharacterScene()
{
	delete storageWidget;
	delete inventoryWidget;
}


void CharacterScene::SetCounter( int delta )
{
	if ( input->nUnits <= 1 )
		return;

	inventoryWidget->SetInfoText( 0 );
	currentUnit += delta;
	if ( currentUnit >= input->nUnits )
		currentUnit = 0;
	if ( currentUnit < 0 )
		currentUnit = input->nUnits-1;

	unit = &input->unit[ currentUnit ];

	char buf[8];
	SNPrintf( buf, 8, "[%d/%d]", currentUnit+1, input->nUnits );
	unitCounter.SetText( buf );
	unitCounter.SetVisible( true );
}


void CharacterScene::Activate()
{
	GetEngine()->SetMap( 0 );
}


void CharacterScene::StatWidget::Init( gamui::Gamui* g, const Unit* unit, float x, float y, bool baseScreen )
{
	float dy = 20.0f;
	float dx = 100.0f;
	char buf[32];
	const Stats& stats = unit->GetStats();

	for( int i=0; i<2*STATS_ROWS; ++i ) {
		if ( g ) textTable[i].Init( g );
		if ( g ) textTable[i].SetVisible( false );
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
	SNPrintf( buf, 32, "%d", stats.AccuracyRating() );
	textTable[c++].SetText( buf );

	textTable[c++].SetText( "Reaction" );
	SNPrintf( buf, 32, "%d", stats.ReactionRating() );
	textTable[c++].SetText( buf );

	textTable[c++].SetText( "Kills" );
	if ( baseScreen ) {
		SNPrintf( buf, 32, "%d", unit->AllMissionKills() );
		textTable[c++].SetText( buf );

		textTable[c++].SetText( "Missions" );
		SNPrintf( buf, 32, "%d", unit->Missions() );
		textTable[c++].SetText( buf );

		textTable[c++].SetText( "XP" );
		SNPrintf( buf, 32, "%d", unit->XP() );
		textTable[c++].SetText( buf );
	}
	else {
		SNPrintf( buf, 32, "%d", unit->KillsCredited() );
		textTable[c++].SetText( buf );
	}

	GLASSERT( c <= 2*STATS_ROWS );
}


void CharacterScene::StatWidget::SetVisible( bool visible )
{
	for( int i=0; i<2*STATS_ROWS; ++i ) {
		textTable[i].SetVisible( visible );
	}
}



void CharacterScene::CompWidget::Init( const ItemDefArr* arr, const Storage* storage, const Unit* unit, gamui::Gamui* g, const gamui::ButtonLook& look, float x, float y, float width )
{
	this->itemDefArr = arr;
	this->unit = unit;
	this->storage = storage;

	float NAME_WIDTH = 70.0f;
	float DY = 16.0f;

	for( int i=0; i<COMP_COL*COMP_ROW; ++i ) {
		if ( g ) compTable[i].Init( g );
		if ( g ) compTable[i].SetVisible( false );
	}
	float delta = (width - NAME_WIDTH) / (COMP_COL-1);

	for( int j=0; j<COMP_ROW; ++j ) {
		for( int i=0; i<COMP_COL; ++i ) {
			float rx = x;

			if ( i>0 )
				rx = x + NAME_WIDTH + delta*(i-1);

			compTable[j*COMP_COL+i].SetPos( rx, y + (float)(j+1)*DY );
		}
	}

	compTable[0].SetText( "Name" );
	compTable[1].SetText( "TU" );
	compTable[2].SetText( "%" );
	compTable[3].SetText( "D" );
	compTable[4].SetText( "D/TU" );
}


void CharacterScene::CompWidget::SetCompText()
{
	const Inventory* inventory = unit->GetInventory();
	const Stats& stats = unit->GetStats();
	int index = 1;

	float r = 8.0f;

	for( int i=0; i<itemDefArr->Size() && index < COMP_ROW; ++i ) {
		const ItemDef* itemDef = itemDefArr->Query( i );
		if ( itemDef && itemDef->IsWeapon() ) {
			bool inInventory = false;
			bool onGround = false;

			if ( inventory->Contains( itemDef ) )
				inInventory = true;
			if ( storage->Contains( itemDef ) ) 
				onGround = true;

			if ( inInventory || onGround ) {
				const WeaponItemDef* wid = itemDef->IsWeapon();
				WeaponMode mode = kAutoFireMode;
		
				DamageDesc dd;
				wid->DamageBase( mode, &dd );

				float fraction, fraction2, damage, dptu;
				wid->FireStatistics( mode, stats.AccuracyArea(), BulletTarget( r ), &fraction, &fraction2, &damage, &dptu );

				fraction = Clamp( fraction, 0.0f, 0.95f );
				fraction2 = Clamp( fraction2, 0.0f, 0.95f );

				char buf[16];

				SNPrintf( buf, 16, "%s%s", inInventory ? "+" : "-", wid->displayName.c_str() );
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
			}
		}
	}
	for( ; index<COMP_ROW; ++index ) {
		for( int k=0; k<COMP_COL; ++k )
			compTable[index*COMP_COL+k].SetText( "" );
	}
}


void CharacterScene::CompWidget::SetVisible( bool visible )
{
	for( int i=0; i<COMP_COL*COMP_ROW; ++i ) {
		compTable[i].SetVisible( visible );
	}
	if ( visible )
		SetCompText();
}


void CharacterScene::CompWidget::Tap( const gamui::UIItem* item )
{
//	for( int i=0; i<NUM_RANGE; ++i )
//		if ( item == &range[i] )
//			SetCompText();
}



void CharacterScene::SwitchMode( int mode )
{
	storageWidget->SetVisible( mode == INVENTORY );
	statWidget.SetVisible( mode == STATS );
	compWidget.SetVisible( mode == COMPARE );
}


void CharacterScene::DrawHUD()
{
}


void CharacterScene::HandleHotKeyMask( int mask )
{
	if ( mask == GAME_HK_NEXT_UNIT && input->nUnits > 1  ) {
		SetCounter( 1 );
	}
	else if ( mask == GAME_HK_PREV_UNIT && input->nUnits > 1 ) {
		SetCounter( -1 );
	}
	inventoryWidget->Update( unit );
	statWidget.Init( 0, unit, storageWidget->X(), 0, input->nUnits>1 );

	const gamui::ButtonLook& blue		= game->GetButtonLook( Game::BLUE_BUTTON );
	compWidget.Init( &game->GetItemDefArr(), storage, unit, 0, blue, storageWidget->X(), 0, storageWidget->Width() );		
	compWidget.SetCompText();
}


void CharacterScene::Tap(	int action, 
							const grinliz::Vector2F& screen,
							const grinliz::Ray& world )
{
	Vector2F ui;
	engine->GetScreenport().ViewToUI( screen, &ui );

	const UIItem* item = 0;
	if ( action == GAME_TAP_DOWN ) {
		gamui2D.TapDown( ui.x, ui.y );
		return;
	}
	else if ( action == GAME_TAP_MOVE ) {
		inventoryWidget->TapMove( ui );
		return;
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		inventoryWidget->Tap( 0, 0 );
		return;
	}
	else if ( action == GAME_TAP_UP ) {
		item = gamui2D.TapUp( ui.x, ui.y );
	}

	if ( item == &backButton ) {
		game->PopScene( 0 );
		return;
	}

	if ( item == &helpButton ) {
		game->PushScene( Game::HELP_SCENE, new HelpSceneData( "characterHelp", false ));
	}

	// control buttons
	if ( item == &control[0] )
		SwitchMode( INVENTORY );
	else if ( item == &control[1] )
		SwitchMode( STATS );
	else if ( item == &control[2] )
		SwitchMode( COMPARE );
	
	if ( ( item == &prevButton || item == &nextButton) && input->nUnits > 1 ) {
		if ( item == &prevButton ) {
			SetCounter( -1 );
		}
		else {
			SetCounter( 1 );
		}
		inventoryWidget->Update( unit );
		statWidget.Init( 0, unit, storageWidget->X(), 0, input->nUnits>1 );

		const gamui::ButtonLook& blue		= game->GetButtonLook( Game::BLUE_BUTTON );
		compWidget.Init( &game->GetItemDefArr(), storage, unit, 0, blue, storageWidget->X(), 0, storageWidget->Width() );		
		compWidget.SetCompText();
	}

	compWidget.Tap( item );
	
	// Inventory of the character:
	int index=-1;
	inventoryWidget->Tap( item, &index );
	if ( index >= 0 ) {
		InventoryToStorage( index );
	}

	const ItemDef* itemDef = storageWidget->ConvertTap( item );
	if ( itemDef ) {
		StorageToInventory( itemDef );
		inventoryWidget->SetInfoText( itemDef );	
	}

	inventoryWidget->Update();
	storageWidget->SetButtons();
}


void CharacterScene::InventoryToStorage( int slot )
{
	// Always succeeds in the inv->storage directory
	Inventory* inv = unit->GetInventory();
	Item item = inv->GetItem( slot );

	if ( item.IsSomething() ) {
		inventoryWidget->SetInfoText( item.GetItemDef() );	
		storage->AddItem( item );
		inv->RemoveItem( slot );
	}
}


void CharacterScene::StorageToInventory( const ItemDef* itemDef )
{
	if ( itemDef ) {
		Item item;
		storage->RemoveItem( itemDef, &item );

		if ( item.IsSomething() ) {
			Inventory* inv = unit->GetInventory();
			if ( inv->AddItem( item ) < 0 ) {
				// Couldn't add to inventory. Return to storage.
				storage->AddItem( item );
			}
		}
	}
}


