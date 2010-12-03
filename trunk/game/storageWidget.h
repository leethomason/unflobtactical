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

#ifndef UFOATTACK_STORAGE_WIDGET_INCLUDED
#define UFOATTACK_STORAGE_WIDGET_INCLUDED

#include "../engine/uirendering.h"
#include "item.h"
#include "../engine/ufoutil.h"

class StorageWidget
{
public:
	StorageWidget(	gamui::Gamui* container,
					const gamui::ButtonLook& green,
					const gamui::ButtonLook& blue,
					const ItemDefArr& itemDefArr,
					const Storage* storage );

	~StorageWidget();

	const ItemDef* ConvertTap( const gamui::UIItem* item );	
	void SetOrigin( float x, float y );
	void SetButtons();
	
	float X() const			{ return selectButton[0].X(); }
	float Y() const			{ return selectButton[0].Y(); }
	float Width() const		{ return (float)(GAME_BUTTON_SIZE*BOX_CX); }
	float Height() const	{ return (float)(GAME_BUTTON_SIZE*BOX_CY); }
	void SetVisible( bool visible );

private:

	enum {
		NUM_SELECT_BUTTONS = 4,
		BOX_CX = 4,
		BOX_CY = 4,
		NUM_BOX_BUTTONS = (BOX_CX)*(BOX_CY-1),
		TOTAL_BUTTONS = NUM_SELECT_BUTTONS + NUM_BOX_BUTTONS
	};
	gamui::ToggleButton selectButton[ NUM_SELECT_BUTTONS ];
	gamui::PushButton	boxButton[ NUM_BOX_BUTTONS ];
	gamui::UIItem*		itemArr[TOTAL_BUTTONS];
	
	const Storage* storage;
	const ItemDef* itemDefMap[NUM_BOX_BUTTONS];
	const ItemDefArr& itemDefArr;
};


#endif // UFOATTACK_STORAGE_WIDGET_INCLUDED
