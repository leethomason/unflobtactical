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
								ItemDef* const* _itemDefArr,
								const Storage* _storage )
	: storage( _storage ),
	  itemDefArr( _itemDefArr )
{
	static const int decoID[NUM_SELECT_BUTTONS] = { DECO_PISTOL, DECO_RAYGUN, DECO_ARMOR, DECO_ALIEN };
	for( int i=0; i<NUM_SELECT_BUTTONS; ++i ) {
		selectButton[i].Init( gamui, 1, blue );
		selectButton[i].SetSize( (float)GAME_BUTTON_SIZE, (float)GAME_BUTTON_SIZE );
		selectButton[i].SetDeco( UIRenderer::CalcDecoAtom( decoID[i], true ), UIRenderer::CalcDecoAtom( decoID[i], false ) );
		itemArr[i*BOX_CX] = &selectButton[i];
	}
	for( int i=0; i<NUM_BOX_BUTTONS; ++i ) {
		boxButton[i].Init( gamui, green );
		boxButton[i].SetSize( (float)GAME_BUTTON_SIZE, (float)GAME_BUTTON_SIZE );
		int y = i / (BOX_CX-1);
		int x = i - y*(BOX_CX-1);
		itemArr[y*BOX_CX+1+x] = &boxButton[i];
	}
	SetOrigin( 0, 0 );
	SetButtons();
}


StorageWidget::~StorageWidget()
{
}


void StorageWidget::SetOrigin( float x, float y )
{
	Gamui::Layout( itemArr, TOTAL_BUTTONS, BOX_CX, BOX_CY, x, y, (float)(GAME_BUTTON_SIZE*BOX_CX), (float)(GAME_BUTTON_SIZE*BOX_CY), 0 );
}


const ItemDef* StorageWidget::ConvertTap( const gamui::UIItem* item )
{
	if ( item >= &boxButton[0] && item < &boxButton[NUM_BOX_BUTTONS] ) {
		int offset = item - boxButton;
		GLASSERT( offset >= 0 && offset < NUM_BOX_BUTTONS );
		return itemDefMap[offset];
	}
	return 0;
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

	for( unsigned i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( !itemDefArr[i] )
			continue;

		int group = 3;

		const WeaponItemDef* wid = itemDefArr[i]->IsWeapon();
		const ClipItemDef* cid = itemDefArr[i]->IsClip();

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
		if ( itemDefArr[i]->IsArmor() || itemDefArr[i]->deco == DECO_SHIELD )
			group=2;

		itemsPerGroup[group] += storage->GetCount( itemDefArr[i] );

		if ( group==groupSelected ) {
			GLASSERT( slot < 12 );
			if ( slot < 12 ) {
				itemDefMap[slot] = itemDefArr[i];
				int deco = itemDefArr[i]->deco;
				boxButton[slot].SetDeco( UIRenderer::CalcDecoAtom( deco, true ), UIRenderer::CalcDecoAtom( deco, false ) );

				char buffer[16];
				if ( storage->GetCount( itemDefArr[i] ) ) {
					SNPrintf( buffer, 16, "(%d)", storage->GetCount( itemDefArr[i] ) );
					boxButton[slot].SetText( itemDefArr[i]->name );
					boxButton[slot].SetText2( buffer );
					boxButton[slot].SetEnabled( true );
				}
				else {
					boxButton[slot].SetText( itemDefArr[i]->name );
					boxButton[slot].SetText2( " " );
					boxButton[slot].SetEnabled( false );
				}
			}
			++slot;
		}
	}
	for( ; slot<12; ++slot ) {
		itemDefMap[slot] = 0;
		RenderAtom nullAtom;
		boxButton[slot].SetDeco( nullAtom, nullAtom );
		boxButton[slot].SetText( "" );
		boxButton[slot].SetText2( "" );
		boxButton[slot].SetEnabled( false );
	}

	for( int i=0; i<4; ++i ) {
		selectButton[i].SetEnabled( itemsPerGroup[i]>0 );
	}
}
