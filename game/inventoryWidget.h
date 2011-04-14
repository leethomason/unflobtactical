#ifndef UFO_ATTACK_INVENTORY_WIDGET
#define UFO_ATTACK_INVENTORY_WIDGET

#include "../grinliz/glvector.h"
#include "../gamui/gamui.h"
#include "inventory.h"
#include "scene.h"

class Unit;

class InventoryWidget
{
public:
	InventoryWidget( gamui::Gamui* g,
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
	gamui::TextLabel	text0, text1, description;
	
	gamui::Image		dragImage;
	int					dragIndex;

	Unit* unit;

};

#endif // UFO_ATTACK_INVENTORY_WIDGET