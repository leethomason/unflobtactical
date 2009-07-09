#ifndef UFO_ATTACK_CHARACTER_SCENE_INCLUDED
#define UFO_ATTACK_CHARACTER_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"

class UIButtonBox;

class CharacterScene : public Scene
{
public:
	CharacterScene( Game* _game );
	virtual ~CharacterScene();

	//virtual void Activate();
	//virtual void DeActivate()									{}

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2I& screen,
						const grinliz::Ray& world );

	virtual void Drag(	int action,
						const grinliz::Vector2I& view )	{}
	virtual void Zoom( int action, int distance )				{}
	virtual void CancelInput()									{}

	// Rendering
	virtual void DoTick( U32 currentTime, U32 deltaTime )		{}
	virtual void DrawHUD();

protected:
	void SetInvWidgetText();

	Engine* engine;
	UIButtonBox* widgets;
	UIButtonBox* charInvWidget;
	Unit unit;
};


#endif // UFO_ATTACK_CHARACTER_SCENE_INCLUDED