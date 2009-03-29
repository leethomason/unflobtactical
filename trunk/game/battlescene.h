#ifndef UFOATTACK_BATTLE_SCENE_INCLUDED
#define UFOATTACK_BATTLE_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"

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
	void InitUnits();

	grinliz::Vector3F dragStart;
	grinliz::Vector3F dragStartCameraWC;
	grinliz::Matrix4  dragMVPI;
	grinliz::Vector3F draggingModelOrigin;

	int		initZoomDistance;
	float	initZoom;

	Model* draggingModel;
	UIButtonBox* widgets;
	Engine* engine;

	enum {
		MAX_TERRANS = 8,
		MAX_CIVS = 16,
		MAX_ALIENS = 16,

		TERRAN_UNITS_START	= 0,
		TERRAN_UNITS_END	= 8,
		CIV_UNITS_START		= TERRAN_UNITS_END,
		CIV_UNITS_END		= CIV_UNITS_START+MAX_CIVS,
		ALIEN_UNITS_START	= CIV_UNITS_END,
		ALIEN_UNITS_END		= ALIEN_UNITS_START+MAX_ALIENS,
		MAX_UNITS			= ALIEN_UNITS_END
	};
	Unit units[MAX_UNITS];
	Model* crateTest;

#ifdef MAPMAKER
	// Mapmaker:
	Model* selection;
	int currentMapItem;
#endif
};


#endif // UFOATTACK_BATTLE_SCENE_INCLUDED