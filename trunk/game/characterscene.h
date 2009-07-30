#ifndef UFO_ATTACK_CHARACTER_SCENE_INCLUDED
#define UFO_ATTACK_CHARACTER_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "storageWidget.h"


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
	void SetButtonGraphics( int index, const Item& item );
	void IndexType( int uiIndex, int* type, int* inventorySlot );
	void StartDragging( const ItemDef* itemDef );

	void DragInvToInv( int startTapIndex, int endTapIndex );
	void DragStorageToInv( const ItemDef* startItemDef, int endTapIndex );
	void DragInvToStorage( int startTap );

	Engine* engine;
	UIButtonBox* backWidget;
	UIButtonBox* charInvWidget;
	StorageWidget* storageWidget;

	const char* description;
	Unit unit;

	int startInvTap;				// start tap on inventory
	const ItemDef* startItemDef;	// start tap on storage

	bool dragging;
	grinliz::Vector2I dragPos;
	const Texture* decoTexture;
	int currentDeco;

	Storage storage;
};


#endif // UFO_ATTACK_CHARACTER_SCENE_INCLUDED