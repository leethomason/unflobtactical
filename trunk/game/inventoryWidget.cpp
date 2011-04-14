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

using namespace grinliz;
using namespace gamui;


InventoryWidget::InventoryWidget(	gamui::Gamui* g,
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
	text0.Init( g );
	text1.Init( g );
	//descriptionLabel.Init( g );
	description.Init( g );

	nameRankUI.Init( g );
	nameRankUI.Set( pos.x, pos.y, unit, false );

	//text0.SetText( "In use:" );
	text1.SetText( "Pack:" );
	//descriptionLabel.SetText( "Armed Weapon:" );

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
	static const float SPACE = 5.0f;

	float y = pos.y;

	// Carried:
	text0.SetPos( pos.x, y );
	y += TEXTHEIGHT;
	description.SetPos( pos.x, y );
	y += TEXTHEIGHT;

	button[ARMOR].SetPos( pos.x, y );
	button[WEAPON].SetPos( pos.x+GAME_BUTTON_SIZE_F, y );
	y += GAME_BUTTON_SIZE_F;

	y += SPACE;
	text1.SetPos( pos.x, y );
	y += TEXTHEIGHT;

	gamui::UIItem* itemArr[NUM_BUTTONS];
	for( int i=PACK_START; i<PACK_END; ++i )
		itemArr[i] = &button[i];

	gamui::Gamui::Layout( &itemArr[PACK_START], PACK_END-PACK_START, 3, pos.x, y );
	
	//y = itemArr[PACK_END-1]->Y() + itemArr[PACK_END-1]->Height();
	//descriptionLabel.SetPos( pos.x, y );
	//y += TEXTHEIGHT;
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
		nameRankUI.Set( pos.x, pos.y, _unit, false );
	}

	GLASSERT( unit );

	// armor
	const gamui::RenderAtom& atomEmptyArmor  = UIRenderer::CalcDecoAtom( DECO_ARMOR, false );
	const gamui::RenderAtom& atomEmptyWeapon = UIRenderer::CalcDecoAtom( DECO_PISTOL, false );
	Inventory* inventory = unit->GetInventory();

	for( int i=0; i<NUM_BUTTONS; ++i ) {
		const Item& item = inventory->GetItem( i );

		if ( item.IsSomething() ) {
			char buffer[16] = { 0 };

			if ( item.IsWeapon() ) {
				const WeaponItemDef* wid = item.IsWeapon();

				int rounds[2] = { 0, 0 };
				rounds[0] = inventory->CalcClipRoundsTotal( wid->weapon[0].clipItemDef );
				rounds[1] = inventory->CalcClipRoundsTotal( wid->weapon[1].clipItemDef );

				const ClipItemDef* clipDef0 = wid->weapon[0].clipItemDef;
				const ClipItemDef* clipDef1 = wid->weapon[1].clipItemDef;

				if ( clipDef1 )
					SNPrintf( buffer, 16, "%c%d%c%d", clipDef0->abbreviation, rounds[0], clipDef1->abbreviation, rounds[1] );
				else
					SNPrintf( buffer, 16, "%c%d", clipDef0->abbreviation, rounds[0] );

			}
			else if ( item.IsClip() ) {
				SNPrintf( buffer, 16, "%d", item.Rounds() );
			}
			button[i].SetDeco( UIRenderer::CalcDecoAtom( item.Deco(), false ), UIRenderer::CalcDecoAtom( item.Deco() , false ) );
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
			RenderAtom atom = UIRenderer::CalcDecoAtom( item.Deco(), false );
			atom.renderState = (const void*) UIRenderer::RENDERSTATE_UI_NORMAL;

			dragImage.SetAtom( atom );
			dragImage.SetVisible( true );
			dragImage.SetCenterPos( ui.x, ui.y );
			return;
		}
	}
	dragImage.SetVisible( false );
	dragIndex = -1;
}
