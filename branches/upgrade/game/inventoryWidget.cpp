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

#include "inventoryWidget.h"
#include "../engine/uirendering.h"
#include "../grinliz/glstringutil.h"
#include "unit.h"
#include "game.h"

using namespace grinliz;
using namespace gamui;


InventoryWidget::InventoryWidget(	Game* game,
									gamui::Gamui* g,
									const gamui::ButtonLook& carriedLook,
									const gamui::ButtonLook& packLook,
									Unit* unit )
{
	gamui = g;
	pos.Set( 0, 0 );
	//this->inventory = inventory;
	this->unit = unit;

	for( int i=PACK_START; i<PACK_END; ++i ) {
		button[i].Init( g, packLook );
	}
	button[WEAPON].Init( g, carriedLook );
	button[ARMOR].Init( g, carriedLook );
	description.Init( g );
	ammo.Init( g );
	info.Init( g );

	nameRankUI.Init( g, game );

	for( int i=0; i<NUM_BUTTONS; ++i ) {
		button[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	}

	RenderAtom nullAtom;
	dragImage.Init( g, nullAtom, true );
	dragImage.SetSize( GAME_BUTTON_SIZE_F*2.0f, GAME_BUTTON_SIZE_F*2.0f );
	dragImage.SetVisible( false );

	dragIndex = -1;

	DoLayout();
	Update();
}


void InventoryWidget::DoLayout()
{
	static const float TEXTHEIGHT = 18.f;
	static const float SPACE = 4.0f;

	float y = pos.y;

	nameRankUI.Set( pos.x, y+SPACE, unit, 
		            NameRankUI::DISPLAY_FACE | NameRankUI::DISPLAY_RANK );

	y += TEXTHEIGHT;
	description.SetPos( nameRankUI.name.X(), y+SPACE );
	y += TEXTHEIGHT;

	ammo.SetPos( nameRankUI.name.X(), y + SPACE );
	//y += TEXTHEIGHT;
	y = nameRankUI.name.Y() + NameRankUI::FACE_SIZE;


	y = GAME_BUTTON_SIZE_F;

	button[ARMOR].SetPos( pos.x, y );
	button[WEAPON].SetPos( pos.x+GAME_BUTTON_SIZE_F, y );
	y += GAME_BUTTON_SIZE_F;

	//y += SPACE;

	//text1.SetVisible( false );
	//text1.SetPos( pos.x, y );
	//y += TEXTHEIGHT;

	gamui::UIItem* itemArr[NUM_BUTTONS];
	for( int i=PACK_START; i<PACK_END; ++i )
		itemArr[i] = &button[i];

	gamui::Gamui::Layout( &itemArr[PACK_START], PACK_END-PACK_START, 3, pos.x, y );
	
	//y = itemArr[PACK_END-1]->Y() + itemArr[PACK_END-1]->Height();
	//descriptionLabel.SetPos( pos.x, y );
	//y += TEXTHEIGHT;

	info.SetPos( pos.x, y+GAME_BUTTON_SIZE_F*2.f ); 
}


int InventoryWidget::UIItemToIndex( const UIItem* item )
{
	if ( item >= (button+0) && item < (button+NUM_BUTTONS) ) {
		int index = (const gamui::PushButton*)item - button;
		GLASSERT( index >= 0 && index < NUM_BUTTONS );
		return index;
	}
	return -1;
}


void InventoryWidget::Update( Unit* _unit )
{
	if ( _unit ) {
		this->unit = _unit;
	}

	GLASSERT( unit );
	char buffer[32] = { 0 };

	// armor
	const gamui::RenderAtom& atomEmptyArmor  = Game::CalcDecoAtom( DECO_ARMOR, false );
	const gamui::RenderAtom& atomEmptyWeapon = Game::CalcDecoAtom( DECO_PISTOL, false );
	Inventory* inventory = unit->GetInventory();

	// weapon
	const WeaponItemDef* wid = unit->GetWeaponDef();
	if ( wid ) {
		int types = wid->NumClipTypes();

		if ( types == 1 ) {
			int rounds = inventory->CalcClipRoundsTotal( wid->clipItemDef[0] );
			SNPrintf( buffer, 32, "%s:%d", wid->clipItemDef[0]->name, rounds );
		}
		else if ( types == 2 ) {
			int rounds[2] = { 0, 0 };
			rounds[0] = inventory->CalcClipRoundsTotal( wid->clipItemDef[0] );
			
			int next=1;
			while( wid->clipItemDef[0] == wid->clipItemDef[next] )
				++next;

			rounds[1] = inventory->CalcClipRoundsTotal( wid->clipItemDef[next] );


			const ClipItemDef* clipDef0 = wid->clipItemDef[0];
			const ClipItemDef* clipDef1 = wid->clipItemDef[next];

			SNPrintf( buffer, 32, "%s:%d %s:%d", clipDef0->name, rounds[0],
											     clipDef1->name, rounds[1] );
		} 
		else if ( types > 2 ) {
			int rounds[WeaponItemDef::MAX_MODE];
			int count=0;
			for( int i=0; i<WeaponItemDef::MAX_MODE; ++i ) {
				if ( i == 0 || (wid->clipItemDef[i] != wid->clipItemDef[i-1] )) {
					rounds[count++] = inventory->CalcClipRoundsTotal( wid->clipItemDef[i] );
				}
			}
			switch ( types ) {
			case 3:	SNPrintf( buffer, 32, "%d/%d/%d", rounds[0], rounds[1], rounds[2] );	break;
			case 4: SNPrintf( buffer, 32, "%d/%d/%d/%d", rounds[0], rounds[1], rounds[2], rounds[3] );	break;
			default: GLASSERT( 0 );	//need to implement
			}
		}
		ammo.SetText( buffer );
	}
	else {
		ammo.SetText( "" );
	}
	for( int i=0; i<NUM_BUTTONS; ++i ) {
		const Item& item = inventory->GetItem( i );
		buffer[0] = 0;

		if ( item.IsSomething() ) {

			if ( item.IsWeapon() ) {
				buffer[0] = 0;
			}
			else if ( item.IsClip() ) {
				SNPrintf( buffer, 16, "%d", item.Rounds() );
			}
			button[i].SetDeco( Game::CalcDecoAtom( item.Deco(), false ), Game::CalcDecoAtom( item.Deco() , false ) );
			button[i].SetText( item.DisplayName() );
			button[i].SetText2( buffer );
		}
		else {
			button[i].SetText( " " );
			button[i].SetText2( " " );
			gamui::RenderAtom atom;
			
			if ( i == ARMOR )
				atom = atomEmptyArmor;
			else if ( i == WEAPON )
				atom = atomEmptyWeapon;

			button[i].SetDeco( atom, atom );
		}	
	}

	const Item& weaponItem = inventory->GetItem( Inventory::WEAPON_SLOT );
	if ( weaponItem.IsWeapon() ) {
		const WeaponItemDef* wid = weaponItem.IsWeapon();

		CStr<40> cstr;
		cstr += wid->displayName.c_str();
		cstr += " ";
		cstr += wid->desc;
		cstr += " ";

		/* Nice display, but too much space on a 4:3 screen.
		cstr += "[";
		if ( wid->HasWeapon( kAltFireMode ) ) {
			cstr += wid->GetClipItemDef( kSnapFireMode )->displayName.c_str();
			cstr += "/";
			cstr += wid->GetClipItemDef( kAltFireMode )->displayName.c_str();
		}
		else {
			cstr += wid->GetClipItemDef( kSnapFireMode )->displayName.c_str();
		}
		cstr += "]";
		*/

		description.SetText( cstr.c_str() );
	}
	else {
		description.SetText( "None" );
	}
	DoLayout();
}


void InventoryWidget::Tap( const gamui::UIItem* item, int* move )
{
	if ( item && move ) {
		// Issue #9
		*move = UIItemToIndex( item );
	}

	const UIItem* g0;
	const UIItem* g1;
	gamui->GetDragPair( &g0, &g1 );
	Inventory* inventory = unit->GetInventory();

	int start = UIItemToIndex( g0 );
	int end = UIItemToIndex( g1 );

	if ( start >= 0 && end >= 0 && start != end ) {

		Item startItem = inventory->GetItem( start );
		Item endItem = inventory->GetItem( end );

		inventory->RemoveItem( start );
		inventory->RemoveItem( end );

		int r0 = inventory->AddItem( start, endItem );
		int r1 = inventory->AddItem( end, startItem );

		if ( r0 < 0 || r1 < 0 ) {
			// swap didn't work. Restore.
			inventory->RemoveItem( start );
			inventory->RemoveItem( end );
			inventory->AddItem( start, startItem );
			inventory->AddItem( end, endItem );
		}
	}

	dragIndex = -1;
	dragImage.SetVisible( false );
}


void InventoryWidget::TapMove( const grinliz::Vector2F& ui )
{
	const UIItem* capture = gamui->TapCaptured();
	int index = UIItemToIndex( capture );
	Inventory* inventory = unit->GetInventory();

	if ( index >= 0 ) {
		dragIndex = index;
		Item item = inventory->GetItem( dragIndex );	

		if ( item.IsSomething() ) {
			RenderAtom atom = Game::CalcDecoAtom( item.Deco(), false );
			atom.renderState = (const void*) UIRenderer::RENDERSTATE_UI_NORMAL;

			dragImage.SetAtom( atom );
			dragImage.SetVisible( true );
			dragImage.SetCenterPos( ui.x, ui.y );

			SetInfoText( item.GetItemDef() );
			return;
		}
	}
	dragImage.SetVisible( false );
	dragIndex = -1;
}


void InventoryWidget::SetInfoText( const ItemDef* itemDef )
{
	if ( itemDef ) {
		char buffer[40];
		itemDef->PrintDesc( buffer, 40 );
		info.SetText( buffer );
	}
	else {
		info.SetText( "" );
	}
}
