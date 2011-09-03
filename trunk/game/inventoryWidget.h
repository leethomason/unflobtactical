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

#ifndef UFO_ATTACK_INVENTORY_WIDGET
#define UFO_ATTACK_INVENTORY_WIDGET

#include "../grinliz/glvector.h"
#include "../gamui/gamui.h"
#include "inventory.h"
#include "scene.h"

class Unit;
class Game;

class InventoryWidget
{
public:
	InventoryWidget( Game* game,
					 gamui::Gamui* g,
					 const gamui::ButtonLook& carriedLook,
					 const gamui::ButtonLook& packLook,
					 Unit* unit );

	void SetOrigin( float x, float y );
	void Update( Unit* unit = 0 );
	void Tap( const gamui::UIItem* item, int* move );
	void TapMove( const grinliz::Vector2F& screen );

private:
	void DoLayout();
	int UIItemToIndex( const gamui::UIItem* item );

	enum {
		NUM_BUTTONS = Inventory::NUM_SLOTS,
		PACK_START = Inventory::GENERAL_SLOT,
		PACK_END   = Inventory::NUM_SLOTS,
		ARMOR = Inventory::ARMOR_SLOT,
		WEAPON = Inventory::WEAPON_SLOT
	};

	grinliz::Vector2F pos;

	gamui::Gamui*		gamui;
	gamui::PushButton	button[NUM_BUTTONS];
	NameRankUI			nameRankUI;
	gamui::TextLabel	description, ammo;
	
	gamui::Image		dragImage;
	int					dragIndex;

	Unit* unit;

};

#endif // UFO_ATTACK_INVENTORY_WIDGET