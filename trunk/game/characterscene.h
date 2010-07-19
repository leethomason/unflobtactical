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

#ifndef UFO_ATTACK_CHARACTER_SCENE_INCLUDED
#define UFO_ATTACK_CHARACTER_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "storageWidget.h"
#include "../engine/camera.h"
#include "gamelimits.h"


class UIButtonBox;
class Texture;

struct CharacterSceneInput
{
	Unit* unit;
	bool  canChangeArmor;
};

class CharacterScene : public Scene
{
public:
	CharacterScene( Game* _game, CharacterSceneInput* unit );
	virtual ~CharacterScene();

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );

//	virtual void Drag(	int action,
//						const grinliz::Vector2F& view )			{}

	virtual void Zoom( int action, int distance )				{}
	virtual void CancelInput()									{}

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	
	{ 
		clip3D->SetInvalid(); 
		clip2D->SetInvalid(); 
		return RENDER_2D;	// | RENDER_3D; FIXME: Would be nice to draw the current weapon. But don't want to render everything else in the engine.
	}
	virtual void DoTick( U32 currentTime, U32 deltaTime )		{}
	virtual void DrawHUD();

protected:
	enum {
		INV, ARMOR, WEAPON, UNLOAD
	};

	enum {
		INVENTORY_MODE,
		STATS_MODE,
		MODE_COUNT
	};
	int mode;
	void InitInvWidget();
	void InitTextTable( gamui::Gamui* );
	void InitCompTable( gamui::Gamui* );

	void SetAllButtonGraphics();
	void SetButtonGraphics( int index, const Item& item );
	void SetCompText();

	void InventoryToStorage( int slot );
	void StorageToInventory( const ItemDef* itemDef );

	enum { INVENTORY, STATS, COMPARE };
	void SwitchMode( int mode );

	Engine* engine;

	gamui::PushButton backButton;
	enum {	NUM_BASE_BUTTONS = 6,
			NUM_INV_BUTTONS = 8,
			ARMOR_BUTTON = 6,
			WEAPON_BUTTON = 7
	};

	// control buttons:
	enum { NUM_CONTROL = 3, NUM_RANGE=3 };
	gamui::PushButton helpButton;
	gamui::ToggleButton control[NUM_CONTROL];
	gamui::ToggleButton range[NUM_RANGE];

	// Left side buttons (unit inventory)
	gamui::PushButton charInvButton[NUM_INV_BUTTONS];

	// Right side option #1
	StorageWidget* storageWidget;  //< widget to show ground items
	// Right side option #2
	enum { STATS_ROWS = 10 };
	NameRankUI	nameRankUI;
	gamui::TextLabel textTable[2*STATS_ROWS];
	// Right side option #3
	// name tu % dam dptu
	enum { COMP_COL = 5, COMP_ROW = 5 };
	gamui::TextLabel compTable[COMP_COL*COMP_ROW];

	const char* description;
	Storage* storage;
	Unit* unit;
};


#endif // UFO_ATTACK_CHARACTER_SCENE_INCLUDED