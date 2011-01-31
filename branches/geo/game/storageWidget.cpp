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

#include "storageWidget.h"
#include "../grinliz/glstringutil.h"
#include "../engine/uirendering.h"

using namespace grinliz;
using namespace gamui;

StorageWidget::StorageWidget(	gamui::Gamui* gamui,
								const gamui::ButtonLook& green,
								const gamui::ButtonLook& blue,
								const ItemDefArr& _itemDefArr,
								const Storage* _storage )
	: storage( _storage ),
	  itemDefArr( _itemDefArr )
{
	static const int decoID[NUM_SELECT_BUTTONS] = { DECO_PISTOL, DECO_RAYGUN, DECO_ARMOR, DECO_ALIEN };

	for( int i=0; i<NUM_SELECT_BUTTONS; ++i ) {
		selectButton[i].Init( gamui, blue );
		if ( i > 0 ) 
			selectButton[0].AddToToggleGroup( &selectButton[i] );
		selectButton[i].SetSize( (float)GAME_BUTTON_SIZE, (float)GAME_BUTTON_SIZE );
		selectButton[i].SetDeco( UIRenderer::CalcDecoAtom( decoID[i], true ), UIRenderer::CalcDecoAtom( decoID[i], false ) );
		itemArr[i] = &selectButton[i];
	}
	for( int i=0; i<NUM_BOX_BUTTONS; ++i ) {
		boxButton[i].Init( gamui, green );
		boxButton[i].SetSize( (float)GAME_BUTTON_SIZE, (float)GAME_BUTTON_SIZE );
		itemArr[i+4] = &boxButton[i];
	}
	SetOrigin( 0, 0 );
	SetButtons();
}


StorageWidget::~StorageWidget()
{
}


void StorageWidget::SetOrigin( float x, float y )
{
	Gamui::Layout( itemArr, TOTAL_BUTTONS, BOX_CX, BOX_CY, x, y, Width(), Height() );
}


const ItemDef* StorageWidget::ConvertTap( const gamui::UIItem* item )
{
	if ( item >= &boxButton[0] && item < &boxButton[NUM_BOX_BUTTONS] ) {
		int offset = (const PushButton*)item - boxButton;
		GLASSERT( offset >= 0 && offset < NUM_BOX_BUTTONS );
		return itemDefMap[offset];
	}
	return 0;
}


void StorageWidget::SetVisible( bool visible )
{
	for( int i=0; i<TOTAL_BUTTONS; ++i )
		itemArr[i]->SetVisible( visible );
}

	
void StorageWidget::SetButtons()
{
	int itemsPerGroup[4] = { 0, 0, 0, 0 };

	// Then the storage.
	int slot = 0;
	int groupSelected = 0;
	for( int i=0; i<NUM_SELECT_BUTTONS; ++i ) {
		if ( selectButton[i].Down() ) {
			groupSelected = i;
			break;
		}
	}

	for( int i=0; i<itemDefArr.Size(); ++i ) {
		const ItemDef* itemDef = itemDefArr.Query( i );
		if ( !itemDef )
			continue;

		int group = 3;

		const WeaponItemDef* wid = itemDef->IsWeapon();
		const ClipItemDef* cid = itemDef->IsClip();

		// Terran non-melee weapons and clips.
		if ( wid && !wid->IsAlien() )
			group=0;
		if ( cid && !cid->IsAlien() )
			group=0;

		// alien non-melee weapons and clips
		if ( wid && wid->IsAlien() )
			group=1;
		if ( cid && cid->IsAlien() )
			group=1;

		// armor, melee
		if ( itemDef->IsArmor() || itemDef->deco == DECO_SHIELD )
			group=2;

		itemsPerGroup[group] += storage->GetCount( itemDef );

		if ( group==groupSelected ) {
			GLASSERT( slot < 12 );
			if ( slot < 12 ) {
				itemDefMap[slot] = itemDef;
				int deco = itemDef->deco;
				boxButton[slot].SetDeco( UIRenderer::CalcDecoAtom( deco, false ), UIRenderer::CalcDecoAtom( deco, false ) );

				char buffer[16];
				if ( storage->GetCount( itemDef ) ) {
					SNPrintf( buffer, 16, "%d", storage->GetCount( itemDef ) );
					boxButton[slot].SetText( itemDef->name );
					boxButton[slot].SetText2( buffer );
					boxButton[slot].SetEnabled( true );
				}
				else {
					boxButton[slot].SetText( " " );	//itemDefArr[i]->name );
					boxButton[slot].SetText2( " " );
					boxButton[slot].SetEnabled( false );
				}
			}
			++slot;
		}
	}
	RenderAtom nullAtom;

	for( ; slot<NUM_BOX_BUTTONS; ++slot ) {
		itemDefMap[slot] = 0;
		boxButton[slot].SetDeco( nullAtom, nullAtom );
		boxButton[slot].SetText( "" );
		boxButton[slot].SetText2( "" );
		boxButton[slot].SetEnabled( false );
	}

	//bool selectionOkay = false;
	//int canSelect = -1;

#if 1
	static const int decoID[NUM_SELECT_BUTTONS] = { DECO_PISTOL, DECO_RAYGUN, DECO_ARMOR, DECO_ALIEN };
	for( int i=0; i<NUM_SELECT_BUTTONS; ++i ) {
		selectButton[i].SetEnabled( true );
		selectButton[i].SetDeco( UIRenderer::CalcDecoAtom( decoID[i], (itemsPerGroup[i]>0) ), 
								 UIRenderer::CalcDecoAtom( decoID[i], (itemsPerGroup[i]>0) ) );

	}
#else
	for( int i=0; i<NUM_SELECT_BUTTONS; ++i ) {
		selectButton[i].SetEnabled( itemsPerGroup[i]>0 );
		if ( selectButton[i].Down() && selectButton[i].Enabled() ) {
			selectionOkay = true;
		}
		if ( selectButton[i].Enabled() && canSelect < 0 ) {
			canSelect = i;
		}
	}
	if ( !selectionOkay && canSelect >= 0 ) {
		for ( int i=0; i<NUM_SELECT_BUTTONS; ++i ) {
			if ( i == canSelect )
				selectButton[canSelect].SetDown();
			else
				selectButton[canSelect].SetUp();
		}
	}
#endif
}
