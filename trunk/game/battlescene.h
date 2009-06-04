#ifndef UFOATTACK_BATTLE_SCENE_INCLUDED
#define UFOATTACK_BATTLE_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "../engine/ufoutil.h"

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
						const grinliz::Vector2I& view );

	virtual void Zoom( int action, int distance );
	virtual void Rotate( int aciton, float degreesFromStart );
	virtual void CancelInput() {}

	virtual void DoTick( U32 currentTime, U32 deltaTime );
	virtual void DrawHUD();

	void Save( UFOStream* s );
	void Load( UFOStream* s );

	// debugging / MapMaker
	void MouseMove( int x, int y );
#ifdef MAPMAKER
	void RotateSelection( int delta );
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );
#endif

private:
	enum {
		ACTION_NONE,
		ACTION_MOVE,
		ACTION_ROTATE,
		ACTION_SHOOT,
	};

	struct Action
	{
		int action;
		Unit* unit;				// unit performing action
		int pathStep;			// move
		float pathFraction;		// move
		float rotation;			// rotate
		Unit* target;			// shoot

		void Clear()							{ action = ACTION_NONE; }
		void Move( Unit* unit )					{ GLASSERT( unit ); this->unit = unit; action = ACTION_MOVE; pathStep = 0; pathFraction = 0; }
		void Rotate( Unit* unit, float r )		{ GLASSERT( unit ); this->unit = unit; action = ACTION_ROTATE; rotation = r; }
		void Shoot( Unit* unit, Unit* target )	{ GLASSERT( unit ); GLASSERT( target ); this->unit = unit; this->target = target; action = ACTION_SHOOT; }
		bool NoAction()							{ return action == ACTION_NONE; }
	};
	CStack< Action > actionStack;
	void RotateAction( Unit* src, const Unit* dst, bool quantize );
	void ShootAction( Unit* src, Unit* dst );
	void ProcessAction( U32 deltaTime );

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

	float Travel( U32 timeMSec, float speed ) { return speed * (float)timeMSec / 1000.0f; }

	void InitUnits();
	void SetUnitsDraggable();
	void TestHitTesting();

	Unit* UnitFromModel( Model* m );
	Unit* GetUnitFromTile( int x, int z );
	bool HandleIconTap( int screenX, int screenY );

	struct Selection
	{
		Selection()	: soldierUnit( 0 ), targetUnit( 0 ), pathEndModel( 0 ) {}
		Unit*	soldierUnit;
		Unit*	targetUnit;
		Model*	pathEndModel;
	};
	Selection selection;

	bool	SelectedSoldier()		{ return selection.soldierUnit != 0; }
	Unit*	SelectedSoldierUnit()	{ return selection.soldierUnit; }
	Model*	SelectedSoldierModel()	{ if ( selection.soldierUnit ) return selection.soldierUnit->GetModel(); return 0; }

	bool	AlienTargeted()			{ return selection.targetUnit != 0; }
	Unit*	AlienUnit()				{ return selection.targetUnit; }
	Model*	AlienModel()			{ if ( selection.targetUnit ) return selection.targetUnit->GetModel(); return 0; }

	Model*	PathEndModel()			{ return selection.pathEndModel; }
	void	SetSelection( Unit* unit );
	void	FreePathEndModel();

	grinliz::Vector3F dragStart;
	grinliz::Vector3F dragStartCameraWC;
	grinliz::Matrix4  dragMVPI;

	int		initZoomDistance;
	float	initZoom;
	
	grinliz::Vector3F orbitPole;
	float	orbitStart;

	UIButtonBox*	widgets;
	UIButtonBox*	fireWidget;
	Engine*			engine;

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