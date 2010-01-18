#ifndef UFO_ATTACK_CHARACTER_SCENE_INCLUDED
#define UFO_ATTACK_CHARACTER_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "storageWidget.h"
#include "../engine/camera.h"
#include "gamelimits.h"


class UIButtonBox;
class Texture;

class CharacterScene : public Scene
{
public:
	CharacterScene( Game* _game );
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
	int selectedUnit;
	Unit* unit;
	Camera savedCamera;

	Unit units[MAX_UNITS];
};


#endif // UFO_ATTACK_CHARACTER_SCENE_INCLUDED