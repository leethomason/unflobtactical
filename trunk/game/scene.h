#ifndef UFOATTACK_SCENE_INCLUDED
#define UFOATTACK_SCENE_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"

class Game;
class Engine;

class Scene
{
public:
	Scene( Game* _game ) : game( _game ) {}
	virtual ~Scene()								{}

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2I& screen,
						const grinliz::Ray& world )				{}

	virtual void Drag(	int action,
						const grinliz::Vector2I& screenRaw )	{}
	virtual void Zoom( int action, int distance )	{}
	virtual void CancelInput()						{}

	// Rendering
	virtual void DoTick( U32 currentTime, U32 deltaTime )	{}
	// Occurs after engine rendering, depth test off
	virtual void DrawHUD()	{}

protected:
	Game* game;
};

#endif // UFOATTACK_SCENE_INCLUDED