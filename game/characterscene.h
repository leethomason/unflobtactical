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
						const grinliz::Vector2I& screen,
						const grinliz::Ray& world );

	virtual void Drag(	int action,
						const grinliz::Vector2I& view )			{}

	virtual void Zoom( int action, int distance )				{}
	virtual void CancelInput()									{}

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	
	{ 
		clip3D->SetInvalid(); 
		clip2D->SetInvalid(); 
		return RENDER_2D | RENDER_3D; 
	}
	virtual void DoTick( U32 currentTime, U32 deltaTime )		{}
	virtual void DrawHUD();

protected:
	enum {
		INV, ARMOR, WEAPON, UNLOAD
	};

	
	// M M M	7 8 9
	// M M M	4 5 6
	// A 2 1	1 2 3
	//   S		  0

	void InitInvWidget();

	void SetAllButtonGraphics();
	void SetButtonGraphics( int index, const Item& item );

	void InventoryToStorage( int slot );
	void StorageToInventory( const ItemDef* itemDef );


	Engine* engine;
	UIButtonBox* backWidget;
	UIButtonGroup* charInvWidget;
	StorageWidget* storageWidget;

	const char* description;
	Storage* storage;
	Unit* unit;
	Camera savedCamera;
	bool canChangeArmor;
};


#endif // UFO_ATTACK_CHARACTER_SCENE_INCLUDED