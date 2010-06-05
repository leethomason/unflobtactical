/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UFOATTACK_BATTLE_SCENE_INCLUDED
#define UFOATTACK_BATTLE_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "../engine/ufoutil.h"
#include "../engine/map.h"
#include "gamelimits.h"
#include "targets.h"
#include "../grinliz/glbitarray.h"
#include "../grinliz/glvector.h"

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

/*
	Naming:			

	Length is always consistent for easy parsing:
	FARM_16_TILE_00.xml				Map
	FARM_16_TILE_00_TEX.png			Ground texture			(16: 128x128 pixels, 32: 256x256 pixels, 64: 512x512 pixels)
	FARM_16_TILE_00_DAY.png			Daytime light map		(16x16, 32x32, 64x64)
	FARM_16_TILE_00_NGT.png			Night light map

	Set:
		FARM

	Size:
		16, 32, 64

	Type:
		TILE		basic map tileset
		UFOx		UFO (small, on the ground)
		LAND		lander

	Variation:
		00-99

*/
class BattleScene : public Scene, public IPathBlocker
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

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D );
	virtual void DoTick( U32 currentTime, U32 deltaTime );
	virtual void Debug3D();
	virtual void DrawHUD();
	virtual void HandleHotKeyMask( int mask );

	virtual void Save( TiXmlElement* doc );
	virtual void Load( const TiXmlElement* doc );

	// debugging / MapMaker
	void MouseMove( int x, int y );
#ifdef MAPMAKER
	void RotateSelection( int delta );
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );
#endif

	virtual void MakePathBlockCurrent( Map* map, const void* user );

private:
	enum {
		BTN_TAKE_OFF,
		BTN_HELP,
		BTN_END_TURN,
		BTN_TARGET,
		BTN_CHAR_SCREEN,

		BTN_ROTATE_CCW,
		BTN_ROTATE_CW,

		BTN_PREV,
		BTN_NEXT,

		BTN_COUNT
	};

	enum {
		ACTION_NONE,
		ACTION_MOVE,
		ACTION_ROTATE,
		ACTION_SHOOT,
		ACTION_DELAY,
		ACTION_HIT,
		ACTION_CAMERA,
		ACTION_CAMERA_BOUNDS,
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

	struct CameraBoundsAction {
		grinliz::Vector3F	target;
		grinliz::Vector3F	normal;
		float				speed;
	};

	struct Action
	{
		int actionID;
		Unit* unit;			// unit performing the action (sometimes null)

		union {
			MoveAction			move;
			RotateAction		rotate;
			ShootAction			shoot;
			DelayAction			delay;
			HitAction			hit;
			CameraAction		camera;
			CameraBoundsAction	cameraBounds;
		} type;

		void Clear()							{ actionID = ACTION_NONE; memset( &type, 0, sizeof( type ) ); }
		void Init( int id, Unit* unit )			{ Clear(); actionID = id; this->unit = unit; }
		bool NoAction()							{ return actionID == ACTION_NONE; }
	};

	CStack< Action > actionStack;

	void PushRotateAction( Unit* src, const grinliz::Vector3F& dst, bool quantize );
	
	// Try to shoot. Return true if success.

	bool PushShootAction(	Unit* src, 
							const grinliz::Vector3F& dst, 
							int select,					// weapon mode
							int type,
							float useError,				// if 0, perfect shot. <1 improve, 1 normal error, >1 more error
							bool clearMoveIfShoot );	// clears move commands if needed

	void PushScrollOnScreen( const grinliz::Vector3F& v );

	// Process the current action. Returns flags that describe what happened.
	enum { 
		STEP_COMPLETE			= 0x01,		// the step of a unit on a path completed. The unit is centered on the map grid
		UNIT_ACTION_COMPLETE	= 0x02,		// shooting, rotating, some other unit action finished (resulted in Pop() )
		OTHER_ACTION_COMPLETE	= 0x04,		// a non unit action (camera motion) finised (resulted in a Pop() )
	};		
	int ProcessAction( U32 deltaTime );
	int ProcessActionShoot( Action* action, Unit* unit, Model* model );
	int ProcessActionHit( Action* action );	

	void OrderNextPrev();
	bool EndCondition( TacticalEndSceneData* data );

	void StopForNewTeamTarget();
	void DoReactionFire();
	void DrawFireWidget();
	void DrawHPBars();
	int CenterRectIntersection( const grinliz::Vector2I& p,
								const grinliz::Rectangle2I& rect,
								grinliz::Vector2I* out );

	std::vector< grinliz::Vector2<S16> >	pathCache;

	// Show the UI zones arount the selected unit
	enum {
		NEAR_PATH_VALID,
		NEAR_PATH_INVALID,
	};
	int nearPathState;
	void ShowNearPath( const Unit* unit );
	// set the fire widget to the primary and secondary weapon
	void SetFireWidget();
	float Travel( U32 timeMSec, float speed ) { return speed * (float)timeMSec / 1000.0f; }

	void InitUnits();
	void TestHitTesting();
	void TestCoordinates();

	bool rotationUIOn;
	bool nextUIOn;

	Unit* UnitFromModel( Model* m, bool useWeaponModel=false );
	Unit* GetUnitFromTile( int x, int z );
	bool HandleIconTap( int screenX, int screenY );
	void HandleNextUnit( int bias );
	void HandleRotation( float bias );
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
	UIImage*		menuImage;

	int				subTurnOrder[MAX_TERRANS];
	int				subTurnIndex;
	int				subTurnCount;

	bool			targetArrowOn[MAX_ALIENS];
	UIImage*		targetArrow[MAX_ALIENS];

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
		// The 'bounds' reflect the area that is invalid, not the visibility of the units.
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

	Unit	units[MAX_UNITS];
	UIBar*	hpBars[MAX_UNITS];
	int		hpBarsFadeTime[MAX_UNITS];

#ifdef MAPMAKER
	void UpdatePreview();
	Model* mapSelection;
	Model* preview;
	int currentMapItem;
#endif
};


#endif // UFOATTACK_BATTLE_SCENE_INCLUDED