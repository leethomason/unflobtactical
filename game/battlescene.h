#ifndef UFOATTACK_BATTLE_SCENE_INCLUDED
#define UFOATTACK_BATTLE_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "../engine/ufoutil.h"
#include "gamelimits.h"
#include "targets.h"
#include "../grinliz/glbitarray.h"
#include "../grinliz/glvector.h"
#include "../sqlite3/sqlite3.h"

class Model;
class UIButtonBox;
class UIButtonGroup;
class UIBar;
class UIImage;
class Engine;
class Texture;
class AI;
struct TacticalEndSceneData;

// Needs to be a POD because it gets 'union'ed in a bunch of events.
// size is important for the same reason.
struct MotionPath
{
	int						pathLen;
	U8						pathData[MAX_TU*2];

	grinliz::Vector2<S16>	GetPathAt( unsigned i ) 
	{
		GLASSERT( (int)i < pathLen );
		grinliz::Vector2<S16> v = { pathData[i*2+0], pathData[i*2+1] };
		return v;
	}
	void Init( const std::vector< grinliz::Vector2<S16> >& pathCache );
	void CalcDelta( int i0, int i1, grinliz::Vector2I* vec, float* rot );
	void Travel( float* travelDistance, int* pathPos, float* fraction );
	void GetPos( int step, float fraction, float* x, float* z, float* rot );
private:
	float DeltaToRotation( int dx, int dy );
};


class BattleScene : public Scene
{
public:
	BattleScene( Game* );
	virtual ~BattleScene();

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

	virtual void Save( TiXmlElement* doc );
	virtual void Load( const TiXmlElement* doc );

	// debugging / MapMaker
	void MouseMove( int x, int y );
#ifdef MAPMAKER
	void RotateSelection( int delta );
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );
#endif

private:
	enum {
		BTN_TAKE_OFF,
		BTN_END_TURN,
		BTN_NEXT,
		//BTN_NEXT_DONE,	// Consider turning back on?
		BTN_TARGET,
		//BTN_LEFT,			// Don't need if you can see 360
		//BTN_RIGHT,
		BTN_CHAR_SCREEN
	};

	enum {
		ACTION_NONE,
		ACTION_MOVE,
		ACTION_ROTATE,
		ACTION_SHOOT,
		ACTION_DELAY,
		ACTION_HIT,
		ACTION_CAMERA,
	};

	struct MoveAction	{
		int			pathStep;
		float		pathFraction;
		MotionPath	path;
	};

	struct RotateAction {
		float rotation;
	};

	struct ShootAction {
		grinliz::Vector3F target;
		int select;	// primary(0) or secondary(1)
	};

	struct DelayAction {
		U32 delay;
	};

	struct HitAction {
		DamageDesc			damageDesc;		// hit with what??
		bool				explosive;
		grinliz::Vector3F	p;				// point of impact
		grinliz::Vector3F	n;				// normal from shooter to target
		Model*				m;				// model impacted - may be 0
	};

	struct CameraAction {
		grinliz::Vector3F	start;
		grinliz::Vector3F	end;
		int					time;
		int					timeLeft;
	};

	struct Action
	{
		int actionID;
		Unit* unit;			// unit performing the action (sometimes null)

		union {
			MoveAction		move;
			RotateAction	rotate;
			ShootAction		shoot;
			DelayAction		delay;
			HitAction		hit;
			CameraAction	camera;
		} type;

		void Clear()							{ actionID = ACTION_NONE; memset( &type, 0, sizeof( type ) ); }
		void Init( int id, Unit* unit )			{ Clear(); actionID = id; this->unit = unit; }
		bool NoAction()							{ return actionID == ACTION_NONE; }
	};

	void InitAction( Action* a, int actionID ) {
		memset( a, 0, sizeof(Action) );
		a->actionID = actionID;
	}

	CStack< Action > actionStack;

	void PushRotateAction( Unit* src, const grinliz::Vector3F& dst, bool quantize );
	
	// Try to shoot. Return true if success.
	bool PushShootAction(	Unit* src, const grinliz::Vector3F& dst, 
							int select, int type, bool useError, bool clearMoveIfShoot );

	// Process the current action. Returns flags that describe what happened.
	enum { 
		STEP_COMPLETE			= 0x01,		// the step of a unit on a path completed. The unit is centered on the map grid
		UNIT_ACTION_COMPLETE	= 0x02,		// shooting, rotating, some other unit action finished (resulted in Pop() )
		OTHER_ACTION_COMPLETE	= 0x04,		// a non unit action (camera motion) finised (resulted in a Pop() )
	};		
	int ProcessAction( U32 deltaTime );
	int ProcessActionShoot( Action* action, Unit* unit, Model* model );
	int ProcessActionHit( Action* action );	

	bool EndCondition( TacticalEndSceneData* data );

	void ScrollOnScreen( const grinliz::Vector3F& v );

	void StopForNewTeamTarget();
	void DoReactionFire();
	void DrawFireWidget();
	void DrawHPBars();

	std::vector< grinliz::Vector2<S16> >	pathCache;

	// Show the UI zones arount the selected unit
	enum {
		NEAR_PATH_VALID,
		NEAR_PATH_INVALID,
		//NEAR_PATH_OFF
	};
	int nearPathState;
	void ShowNearPath( const Unit* unit );
	// set the fire widget to the primary and secondary weapon
	void SetFireWidget();
	float Travel( U32 timeMSec, float speed ) { return speed * (float)timeMSec / 1000.0f; }

	void InitUnits();
	void TestHitTesting();
	void SetPathBlocks( const Unit* exclude );

	Unit* UnitFromModel( Model* m );
	Unit* GetUnitFromTile( int x, int z );
	bool HandleIconTap( int screenX, int screenY );
	void SetFogOfWar();

	struct Selection
	{
		Selection()	{ Clear(); }
		void Clear()		{ soldierUnit = 0; targetUnit = 0; targetPos.Set( -1, -1 ); }
		void ClearTarget()	{ targetUnit = 0; targetPos.Set( -1, -1 ); }
		Unit*	soldierUnit;
		
		Unit*				targetUnit;
		grinliz::Vector2I	targetPos;
	};
	Selection selection;

	bool	SelectedSoldier()		{ return selection.soldierUnit != 0; }
	Unit*	SelectedSoldierUnit()	{ return selection.soldierUnit; }
	Model*	SelectedSoldierModel()	{ if ( selection.soldierUnit ) return selection.soldierUnit->GetModel(); return 0; }

	bool	HasTarget()				{ return selection.targetUnit || selection.targetPos.x >= 0; }
	bool	AlienTargeted()			{ return selection.targetUnit != 0; }
	Unit*	AlienUnit()				{ return selection.targetUnit; }
	Model*	AlienModel()			{ if ( selection.targetUnit ) return selection.targetUnit->GetModel(); return 0; }

	void	SetSelection( Unit* unit );

	void NextTurn();

	grinliz::Vector3F dragStart;
	grinliz::Vector3F dragStartCameraWC;
	grinliz::Matrix4  dragMVPI;

	int		initZoomDistance;
	float	initZoom;
	
	grinliz::Vector3F orbitPole;
	float	orbitStart;

	enum {
		UIM_NORMAL,			// normal click and move
		UIM_TARGET_TILE,	// special target abitrary tile mode
		UIM_FIRE_MENU		// fire menu is up
	};

	int				uiMode;
	UIButtonGroup*	widgets;
	UIButtonGroup*	fireWidget;
	UIImage*		alienImage;

	Engine*			engine;
	grinliz::Random random;	// "the" random number generator for the battle
	int				currentTeamTurn;
	AI*				aiArr[3];

	struct TargetEvent
	{
		U8 team;		// 1: team, 0: unit
		U8 gain;		// 1: gain, 0: loss
		U8 viewerID;	// unit id of viewer, or teamID if team event
		U8 targetID;	// unit id of target
	};

	Targets m_targets;
	CDynArray< TargetEvent > targetEvents;
	CDynArray< grinliz::Vector2I > doors;
	void ProcessDoors();
	bool ProcessAI();			// return true if turn over.

	// Updates what units can and can not see. Sets the 'Targets' structure above,
	// and generates targetEvents.
	void CalcTeamTargets();
	void DumpTargetEvents();

	// Groups all the visibility code together. In the battlescene itself, visibility quickly
	// becomes difficult to track. 'Visibility' groups it all together and does the minimum
	// amount of computation.
	class Visibility {
	public:
		Visibility();
		~Visibility()					{}

		void Init( BattleScene* bs, const Unit* u, Map* m )	{ battleScene = bs; this->units = u; map = m; }

		void InvalidateAll();
		void InvalidateAll( const grinliz::Rectangle2I& bounds );
		void InvalidateUnit( int i );

		int TeamCanSee( int team, int x, int y ); //< Can anyone on the 'team' see the location (x,y)
		int UnitCanSee( int unit, int x, int y );

		// returs the current state of the FoW bit - and clears it!
		bool FogCheckAndClear()	{ bool result = fogInvalid; fogInvalid = false; return result; }

	private:
		void CalcUnitVisibility( int unitID );
		void CalcVisibilityRay( int unitID, const grinliz::Vector2I& pos, const grinliz::Vector2I& origin );

		BattleScene*	battleScene;
		const Unit*		units;
		Map*			map;
		bool			fogInvalid;

		bool current[MAX_UNITS];	//< Is the visibility current? Triggers CalcUnitVisibility if not.

		grinliz::BitArray< MAP_SIZE, MAP_SIZE, MAX_UNITS >	visibilityMap;
		grinliz::BitArray< MAP_SIZE, MAP_SIZE, 1 >			visibilityProcessed;		// temporary - used in vis calc.
	};
	Visibility visibility;

	//void InvalidateAllVisibility();
	//void InvalidateAllVisibility( 
	//void CalcAllVisibility();

	// before using 'visibilityMap', bring current with CalcAllVisibility()
	//void CalcUnitVisibility( const Unit* unit );

	Unit	units[MAX_UNITS];
	UIBar*	hpBars[MAX_UNITS];
	int		hpBarsFadeTime[MAX_UNITS];

#ifdef MAPMAKER
	// Mapmaker:
	void UpdatePreview();
	Model* mapSelection;
	Model* preview;
	int currentMapItem;
	//sqlite3* mapDatabase;
#endif
};


#endif // UFOATTACK_BATTLE_SCENE_INCLUDED