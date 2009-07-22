#ifndef UFO_ATTACK_CHARACTER_SCENE_INCLUDED
#define UFO_ATTACK_CHARACTER_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"

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
						const grinliz::Vector2I& view );
	virtual void Zoom( int action, int distance )				{}
	virtual void CancelInput()									{}

	// Rendering
	virtual void DoTick( U32 currentTime, U32 deltaTime )		{}
	virtual void DrawHUD();

protected:
	enum {
		INV, ARMOR, WEAPON, UNLOAD
	};

	void SetInvWidget();
	void SetStorageWidget();
	void SetButtonGraphics( int index, const Item& item );
	void IndexType( int uiIndex, int* type, int* inventorySlot );

	Engine* engine;
	UIButtonBox* backWidget;
	UIButtonBox* charInvWidget;
	UIButtonBox* selectWidget;
	UIButtonBox* storageWidget;
	const char* description;
	Unit unit;
	int groupSelected;

	UIButtonBox *startWidget;
	int startTap;

	bool dragging;
	grinliz::Vector2I dragPos;
	const Texture* decoTexture;
	int currentDeco;
};


#endif // UFO_ATTACK_CHARACTER_SCENE_INCLUDED