#ifndef UFOATTACK_SCENE_INCLUDED
#define UFOATTACK_SCENE_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"

class Game;
class Engine;
class TiXmlElement;

#pragma warning ( push )
#pragma warning ( disable : 4100 )	// un-referenced formal parameter

class Scene
{
public:
	Scene( Game* _game ) : game( _game )						{}
	virtual ~Scene()											{}

	virtual void Activate()											{}
	virtual void DeActivate()										{}

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2I& screen,
						const grinliz::Ray& world )				{}

	virtual void Drag(	int action,
						const grinliz::Vector2I& view )			{}
	virtual void Zoom( int action, int distance )				{}
	virtual void Rotate( int action, float degrees )			{}
	virtual void CancelInput()									{}

	virtual void Save( TiXmlElement* doc )						{}
	virtual void Load( const TiXmlElement* doc )				{}

	// Rendering
	virtual void DoTick( U32 currentTime, U32 deltaTime )		{}

	// Occurs after engine rendering.
	virtual void DrawHUD()										{}

protected:
	Engine* GetEngine();

	Game* game;
};

#pragma warning ( pop )

#endif // UFOATTACK_SCENE_INCLUDED