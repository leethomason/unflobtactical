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
class InventoryWidget;
class TacMap;

class CharacterSceneData : public SceneData
{
public:
	Unit* unit;	// array of units
	int nUnits;
	Storage* storage;
};


class CharacterScene : public Scene
{
public:
	CharacterScene( Game* _game, CharacterSceneData* data );
	virtual ~CharacterScene();

	virtual void Activate();

	// UI
	virtual void Tap(	int action, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	
	{ 
		clip3D->SetInvalid(); 
		clip2D->SetInvalid(); 
		return RENDER_2D;	// | RENDER_3D; FIXME: Would be nice to draw the current weapon. But don't want to render everything else in the engine.
	}
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
	void InventoryToStorage( int slot );
	void StorageToInventory( const ItemDef* itemDef );
	void SetCounter( int delta );

	enum { INVENTORY, STATS, COMPARE };
	void SwitchMode( int mode );

	Engine* engine;

	gamui::PushButton backButton;
	gamui::PushButton nextButton;
	gamui::PushButton prevButton;
	gamui::TextLabel  unitCounter;

	// control buttons:
	enum { NUM_CONTROL = 3, NUM_RANGE=3 };
	gamui::PushButton helpButton;
	gamui::ToggleButton control[NUM_CONTROL];

	class StatWidget {
	public:
		StatWidget()		{}
		void Init( gamui::Gamui* g, const Unit* unit, float x, float y );
		void SetVisible( bool visible );
		void Update( Unit* unit );
	private:
		enum { STATS_ROWS = 10 };
		//NameRankUI	nameRankUI;
		gamui::TextLabel textTable[2*STATS_ROWS];
	};

	class CompWidget {
	public:
		CompWidget()		{}
		void Init(	const ItemDefArr*, const Storage* storage, const Unit* unit,
					gamui::Gamui* g, const gamui::ButtonLook& look, float x, float y, float w );
		
		void SetVisible( bool visible );
		void Tap( const gamui::UIItem* item );
		void SetCompText();
	private:
		gamui::ToggleButton range[NUM_RANGE];

		const ItemDefArr* itemDefArr;
		const Storage* storage;
		const Unit* unit;

		//NameRankUI	nameRankUI;
		// name tu % dam dptu
		enum { COMP_COL = 5, COMP_ROW = 5 };
		gamui::TextLabel compTable[COMP_COL*COMP_ROW];
	};

	CharacterSceneData* input;
	InventoryWidget* inventoryWidget;

	// Right side option #1
	StorageWidget* storageWidget;  //< widget to show ground items

	// Right side option #2
	StatWidget statWidget;

	// Right side option #3
	CompWidget compWidget;

	Storage* storage;
	Unit* unit;
	int currentUnit;
};


#endif // UFO_ATTACK_CHARACTER_SCENE_INCLUDED