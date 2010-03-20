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
	StorageWidget(	const Screenport& port,
					ItemDef* const* itemDefArr,
					const Storage* storage );

	~StorageWidget();

	const ItemDef* Tap( int x, int y );	
	void Draw();

	void SetOrigin( int x, int y );
	void SetButtonSize( int dx, int dy );
	void SetPadding( int dx, int dy );

	void CalcBounds( grinliz::Rectangle2I* _bounds );
	void Update()	{ valid = false; }

private:
	void SetButtons();

	bool valid;
	UIButtonBox* selectWidget;
	UIButtonBox* boxWidget;

	int groupSelected;
	const Storage* storage;
	const ItemDef* itemDefMap[12];
	ItemDef* const* itemDefArr;
};


#endif // UFOATTACK_STORAGE_WIDGET_INCLUDED
