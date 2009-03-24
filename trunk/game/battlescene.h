#ifndef UFOATTACK_BATTLE_SCENE_INCLUDED
#define UFOATTACK_BATTLE_SCENE_INCLUDED

#include "scene.h"

class Model;
class UIButtonBox;
class Engine;
class Texture;


class BattleScene : public Scene
{
public:
	BattleScene( Game* );
	virtual ~BattleScene();

	virtual void Activate();
	virtual void DeActivate();

	virtual void Tap(	int count, 
						const grinliz::Vector2I& screen,
						const grinliz::Ray& world );

	virtual void Drag(	int action,
						const grinliz::Vector2I& screenRaw );

	virtual void Zoom( int action, int distance );
	virtual void CancelInput() {}

	virtual void DoTick( U32 currentTime, U32 deltaTime );
	virtual void DrawHUD();

#ifdef MAPMAKER
	void MouseMove( int x, int y );
	void RotateSelection();
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );
#endif

private:

	grinliz::Vector3F dragStart;
	grinliz::Vector3F dragStartCameraWC;
	grinliz::Matrix4  dragMVPI;
	grinliz::Vector3F draggingModelOrigin;

	int		initZoomDistance;
	float	initZoom;

	Model* draggingModel;
	UIButtonBox* widgets;
	Engine* engine;

	enum { NUM_TEST_MODEL = 256 };
	Model* testModel[NUM_TEST_MODEL];

#ifdef MAPMAKER
	// Mapmaker:
	Model* selection;
	int currentMapItem;
#endif
};


#endif // UFOATTACK_BATTLE_SCENE_INCLUDED