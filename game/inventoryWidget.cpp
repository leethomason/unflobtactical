#include "inventoryWidget.h"
#include "../engine/uirendering.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;
using namespace gamui;


InventoryWidget::InventoryWidget(	gamui::Gamui* g,
									const gamui::ButtonLook& carriedLook,
									const gamui::ButtonLook& packLook,
									Inventory* inventory )
{
	gamui = g;
	pos.Set( 0, 0 );
	this->inventory = inventory;

	for( int i=PACK_START; i<PACK_END; ++i ) {
		button[i].Init( g, packLook );
	}
	button[WEAPON].Init( g, carriedLook );
	button[ARMOR].Init( g, carriedLook );
	text0.Init( g );
	text1.Init( g );
	text0.SetText( "In use" );
	text1.SetText( "Pack" );

	for( int i=0; i<NUM_BUTTONS; ++i ) {
		button[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	}

	RenderAtom nullAtom;
	dragImage.Init( g, nullAtom );
	dragImage.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
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


void InventoryWidget::Update()
{
	// armor
	const gamui::RenderAtom& atomEmptyArmor  = UIRenderer::CalcDecoAtom( DECO_ARMOR, false );
	const gamui::RenderAtom& atomEmptyWeapon = UIRenderer::CalcDecoAtom( DECO_PISTOL, false );

	for( int i=0; i<NUM_BUTTONS; ++i ) {
		const Item& item = inventory->GetItem( i );

		if ( item.IsSomething() ) {
			char buffer[16] = { 0 };

			if ( item.IsWeapon() ) {
				const WeaponItemDef* wid = item.IsWeapon();

				int rounds[2] = { 0, 0 };
				rounds[0] = inventory->CalcClipRoundsTotal( wid->weapon[0].clipItemDef );
				rounds[1] = inventory->CalcClipRoundsTotal( wid->weapon[1].clipItemDef );

				if ( wid->HasWeapon( kAltFireMode ) )
					SNPrintf( buffer, 16, "R%d %d", rounds[0], rounds[1] );
				else
					SNPrintf( buffer, 16, "R%d", rounds[0] );

			}
			else if ( item.IsClip() ) {
				SNPrintf( buffer, 16, "%d", item.Rounds() );
			}
			button[i].SetDeco( UIRenderer::CalcDecoAtom( item.Deco(), false ), UIRenderer::CalcDecoAtom( item.Deco() , false ) );
			button[i].SetText( item.Name() );
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
}


void InventoryWidget::Tap( const gamui::UIItem* item, int* move )
{
	*move = UIItemToIndex( item );

	const UIItem* g0;
	const UIItem* g1;
	gamui->GetDragPair( &g0, &g1 );

	int start = UIItemToIndex( g0 );
	int end = UIItemToIndex( g1 );

	if ( start >= 0 && end >= 0 && start != end ) {

		// Can the items be swapped?
		int low  = start;
		int high = end;
		if ( low > high ) Swap( &high, &low );

		Item lowItem = inventory->GetItem( low );
		Item highItem = inventory->GetItem( high );

		bool doSwap = false;
		if ( low == ARMOR && ( highItem.IsArmor() || highItem.IsNothing() ) ) {
			doSwap = true;
		}
		else if ( low == WEAPON && ( highItem.IsWeapon() || highItem.IsNothing() ) ) {
			doSwap = true;
		}
		else if ( low >= PACK_START ) {
			doSwap = true;
		}

		if ( doSwap ) {
			inventory->RemoveItem( low );
			inventory->RemoveItem( high );
			inventory->AddItem( highItem );
			inventory->AddItem( lowItem );
		}
	}

	dragIndex = -1;
	dragImage.SetVisible( false );
}


void InventoryWidget::TapMove( const grinliz::Vector2F& ui )
{
	const UIItem* capture = gamui->TapCaptured();
	int index = UIItemToIndex( capture );

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
