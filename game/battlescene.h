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
	virtual void Rotate( int aciton, float degreesFromStart );
	virtual void CancelInput() {}

	virtual void DoTick( U32 currentTime, U32 deltaTime );
	virtual void DrawHUD();

	void Save( UFOStream* s );
	void Load( UFOStream* s );

#ifdef MAPMAKER
	void MouseMove( int x, int y );
	void RotateSelection( int delta );
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );
#endif

private:
	enum {
		ACTION_NONE,
		ACTION_MOVE
	};
	struct Action
	{
		int action;
		int pathStep;
		float pathFraction;

		void Clear() { action = ACTION_NONE; }
		void Move()  { action = ACTION_MOVE; pathStep = 0; pathFraction = 0; }
		bool NoAction() { return action == ACTION_NONE; }
	};
	Action action;

	struct Path
	{
		enum { MAX_PATH			= 100 };
		grinliz::Vector2<S16>	start, end;
		int						len;
		grinliz::Vector2<S16>	path[MAX_PATH];

		void Clear() { len = 0; start.Set( -1, -1 ); end.Set( -1, -1 ); }
		void CalcDelta( int i0, int i1, grinliz::Vector2I* vec, float* rot );
		void Travel( float* travelDistance, int* pathPos, float* fraction );
		void GetPos( int step, float fraction, float* x, float* z, float* rot );
	private:
		float DeltaToRotation( int dx, int dy );
	};
	Path path;

	void InitUnits();
	void SetUnitsDraggable();
	int UnitFromModel( Model* m );

	grinliz::Vector3F dragStart;
	grinliz::Vector3F dragStartCameraWC;
	grinliz::Matrix4  dragMVPI;
	//grinliz::Vector3F draggingModelOrigin;

	int		initZoomDistance;
	float	initZoom;
	
	grinliz::Vector3F orbitPole;
	float	orbitStart;

	//Model* draggingModel;
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
		MAX_UNITS			= ALIEN_UNITS_END,
	};
	Model*					crateTest;
	int						selectedUnit;
	Model*					pathEndModel;

	Unit units[MAX_UNITS];

#ifdef MAPMAKER
	// Mapmaker:
	void UpdatePreview();
	Model* selection;
	Model* preview;
	int currentMapItem;
#endif
};


#endif // UFOATTACK_BATTLE_SCENE_INCLUDED