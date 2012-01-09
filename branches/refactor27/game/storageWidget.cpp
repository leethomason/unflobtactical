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
								const Storage* _storage,
								float w, float h,
								float _costMult )
	: 
		costMult( _costMult ),
		storage( _storage ),
		itemDefArr( _itemDefArr )
{
	buttonWidth = w;
	buttonHeight = h;
	fudgeFactor.Set( 0, 0 );
	info.Init( gamui );

	static const int decoID[NUM_SELECT_BUTTONS] = { DECO_PISTOL, DECO_RAYGUN, DECO_CHARACTER, DECO_ALIEN };

	for( int i=0; i<NUM_SELECT_BUTTONS; ++i ) {
		selectButton[i].Init( gamui, blue );
		if ( i > 0 ) 
			selectButton[0].AddToToggleGroup( &selectButton[i] );
		selectButton[i].SetSize( buttonWidth, buttonHeight );
		selectButton[i].SetDeco( UIRenderer::CalcDecoAtom( decoID[i], true ), UIRenderer::CalcDecoAtom( decoID[i], false ) );
		itemArr[i] = &selectButton[i];
	}
	for( int i=0; i<NUM_BOX_BUTTONS; ++i ) {
		boxButton[i].Init( gamui, green );
		boxButton[i].SetSize( buttonWidth, buttonHeight );
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
	
	gamui::UIItem* item = itemArr[TOTAL_BUTTONS-BOX_CX];
	info.SetPos( item->X(), item->Y() + item->Height() );
}


const ItemDef* StorageWidget::ConvertTap( const gamui::UIItem* item )
{
	if ( item >= &boxButton[0] && item < &boxButton[NUM_BOX_BUTTONS] ) {
		int offset = (const PushButton*)item - boxButton;
		GLASSERT( offset >= 0 && offset < NUM_BOX_BUTTONS );

		const ItemDef* itemDef = itemDefMap[offset];
		if ( itemDef ) {
			char buf[40];
			itemDef->PrintDesc( buf, 40 );
			info.SetText( buf );
		}
		return itemDef;
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

	RenderAtom nullAtom;

	for( int i=0; i<NUM_BOX_BUTTONS; ++i ) {
		itemDefMap[i] = 0;
		boxButton[i].SetDeco( nullAtom, nullAtom );
		boxButton[i].SetText( "" );
		boxButton[i].SetText2( "" );
		boxButton[i].SetEnabled( false );
	}

	for( int i=0; i<itemDefArr.Size(); ++i ) {
		const ItemDef* itemDef = itemDefArr.Query( i );
		if ( !itemDef || itemDef->Hide() )
			continue;

		int group = 3;
		int row = -1;

		const WeaponItemDef* wid = itemDef->IsWeapon();
		const ClipItemDef* cid = itemDef->IsClip();

		/*
		if ( wid ) group = wid->IsAlien() ? 1 : 0;
		if ( cid ) group = cid->IsAlien() ? 1 : 2;
		if ( itemDef->IsArmor() ) group = 0;
		if ( itemDef->deco == DECO_SHIELD ) group = 2;
		if ( 	StrEqual( itemDef->name, "Soldr" ) 
			 || StrEqual( itemDef->name, "Sctst" ) ) 
		{
			group = 2;
		}
		*/

		// Terran weapons and clips.
		if ( wid ) group = wid->IsAlien() ? 1 : 0;
		if ( cid ) group = cid->IsAlien() ? 1 : 0;

		// armor, bonus rockets.
		if ( itemDef->IsArmor() ) {
			group=2;
		}
		if ( itemDef->deco == DECO_SHIELD ) {
			group=2;
		}
		if (    StrEqual( itemDef->name, "Flare" ) 
			 || StrEqual( itemDef->name, "Smoke" ) 
			 || StrEqual( itemDef->name, "Soldr" ) 
			 || StrEqual( itemDef->name, "Sctst" ) ) 
		{
			group = 2;
		}
		itemsPerGroup[group] += storage->GetCount( itemDef );

		if ( group==groupSelected ) {
			GLASSERT( slot < 12 );
			if ( slot < 12 ) {
				if ( row >= 0 ) {
					for( int j=row*4; j<12; ++j ) {
						if ( itemDefMap[j] == 0 ) {
							slot = j;
							break;
						}
					}

				}
				itemDefMap[slot] = itemDef;
				int deco = itemDef->deco;
				boxButton[slot].SetDeco( UIRenderer::CalcDecoAtom( deco, false ), UIRenderer::CalcDecoAtom( deco, false ) );

				static const int SZ=16;
				char buffer[SZ];
				if ( storage->GetCount( itemDef ) ) {
					if ( costMult ) {
						// show the cost
						SNPrintf( buffer, SZ, "$%d", itemDef->BuyPrice( costMult ) );
					}
					else {
						SNPrintf( buffer, SZ, "%d", storage->GetCount( itemDef ) );
					}
					boxButton[slot].SetText( itemDef->displayName.c_str() );
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
