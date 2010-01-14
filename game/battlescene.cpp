#include "battlescene.h"
#include "game.h"
#include "cgame.h"

#include "../engine/uirendering.h"
#include "../engine/platformgl.h"
#include "../engine/particle.h"
#include "../engine/text.h"

#include "battlestream.h"

#include "../grinliz/glfixed.h"
#include "../micropather/micropather.h"
#include "../grinliz/glstringutil.h"

#include "../tinyxml/tinyxml.h"

using namespace grinliz;


BattleScene::BattleScene( Game* game ) : Scene( game )
{
	engine  = &game->engine;
	uiMode = UIM_NORMAL;

#ifdef MAPMAKER
	currentMapItem = 1;
	sqlite3_open( "./resin/map.db", &mapDatabase );
	engine->GetMap()->SyncToDB( mapDatabase, "farmland" );
#endif

	path.Clear();

	// On screen menu.
	widgets = new UIButtonGroup( engine->GetScreenport() );
	
	{
		const int icons[] = {	ICON_BLUE_BUTTON,		// 0 take-off
								ICON_GREEN_BUTTON,		// 1 end turn
								ICON_GREEN_BUTTON,		// 2 next
								//ICON_GREEN_BUTTON,		// 3 next-done
								ICON_RED_BUTTON,		// 4 target
								//ICON_BLUE_BUTTON,		// 5 left
								//ICON_BLUE_BUTTON,		// 6 right
								ICON_GREEN_BUTTON		// 7 character
		};

		const char* iconText[] = {	"EXIT",
									"O",
									"N",
									//"ND",
									"",
									//"<-",
									//"->",
									""	
								  };		

		widgets->InitButtons( icons, 5 );
		widgets->SetText( iconText );

		widgets->SetDeco( BTN_CHAR_SCREEN, DECO_CHARACTER );
		widgets->SetDeco( BTN_TARGET, DECO_AIMED );

		const Screenport& port = engine->GetScreenport();
		const Vector2I& pad = widgets->GetPadding();
		const Vector2I& size = widgets->GetButtonSize();
		int h = port.UIHeight();
		int w = port.UIWidth();

		widgets->SetPos( BTN_TAKE_OFF,		0, h-size.y );
		widgets->SetPos( BTN_END_TURN,		0, h-(size.y*2+pad.y) );
		widgets->SetPos( BTN_NEXT,			0, size.y+pad.y );
		//widgets->SetPos( BTN_NEXT_DONE,		0, 0 );
		widgets->SetPos( BTN_TARGET,		size.x+pad.x, 0 );
		//widgets->SetPos( 5,		w-(size.x*2+pad.x), 0 );
		//widgets->SetPos( 6,		w-size.x, 0 );
		widgets->SetPos( BTN_CHAR_SCREEN,		w-size.x, size.y+pad.y );
	}
	// When enemy targeted.
	fireWidget = new UIButtonBox( engine->GetScreenport() );
	const int fireIcons[] = { ICON_TRANS_RED, ICON_TRANS_RED, ICON_TRANS_RED, ICON_TRANS_RED,
							  ICON_TRANS_RED, ICON_TRANS_RED, ICON_TRANS_BLUE, ICON_TRANS_BLUE };
	fireWidget->InitButtons( fireIcons, 8 );
	fireWidget->SetColumns( 2 );
	fireWidget->SetPadding( 0, 0 );
	fireWidget->SetButtonSize( 60, 60 );
	fireWidget->SetDeco( 0, DECO_SNAP );
	fireWidget->SetDeco( 1, DECO_SNAP );
	fireWidget->SetDeco( 2, DECO_AUTO );
	fireWidget->SetDeco( 3, DECO_AUTO );
	fireWidget->SetDeco( 4, DECO_AIMED );
	//fireWidget->SetDeco( 5, DECO_AIMED );
	//fireWidget->SetDeco( 6, DECO_ROCKET );
	fireWidget->SetDeco( 5, DECO_SHELLS );

	engine->EnableMap( true );

#ifdef MAPMAKER
	{
		const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( "selection" );
		mapSelection = engine->AllocModel( res );
		mapSelection->SetPos( 0.5f, 0.0f, 0.5f );
		preview = 0;
	}
#endif

#if defined( MAPMAKER )
#else
	// 2 ways to start this:
	// - creating a new map
	// - linking up the current map (writeable)
	// This should really always hook up to the current map, but for
	// now do a copy over.

	UFOStream* stream = game->OpenStream( "BattleScene", false );
	if ( !stream ) {
		InitUnits();
	}
	else {
		Load( stream );
	}
#endif

	visibilityMap.ClearAll();
	SetFogOfWar();
	NewTurn( Unit::SOLDIER );
	engine->GetMap()->QueryAllDoors( &doors );
	ProcessDoors();
}


BattleScene::~BattleScene()
{
#ifdef MAPMAKER
	{
		TiXmlDocument doc( "testmap.xml" );
		engine->GetMap()->Save( &doc );
		doc.SaveFile();
	}
#endif 
	UFOStream* stream = game->OpenStream( "BattleScene" );
	Save( stream );
	ParticleSystem::Instance()->Clear();
	//FreePathEndModel();

#if defined( MAPMAKER )
	if ( mapSelection ) 
		engine->FreeModel( mapSelection );
	if ( preview )
		engine->FreeModel( preview );
	sqlite3_close( mapDatabase );
#endif

	delete fireWidget;
	delete widgets;
}


void BattleScene::InitUnits()
{
	int Z = engine->GetMap()->Height();
	Random random(1098305);
	random.Rand();
	random.Rand();

	Item gun0( game, "PST" ),
		 gun1( game, "RAY-1" ),
		 ar3( game, "AR-3P" ),
		 plasmaRifle( game, "PLS-2" ),
		 medkit( game, "Med" ),
		 armor( game, "ARM-1" ),
		 fuel( game, "Gel" ),
		 grenade( game, "RPG", -1 ),
		 autoClip( game, "AClip", -1 ),
		 cell( game, "Cell", -1 ),
		 tachyon( game, "Tach", -1 ),
		 clip( game, "Clip", -1 );

	gun0.Insert( clip );
	gun1.Insert( cell );
	plasmaRifle.Insert( cell );
	plasmaRifle.Insert( tachyon );
	ar3.Insert( autoClip );
	ar3.Insert( grenade );

	for( int i=0; i<4; ++i ) {
		Vector2I pos = { (i*2)+10, Z-10 };
		Unit* unit = &units[TERRAN_UNITS_START+i];

		random.Rand();
		unit->Init( engine, game, Unit::SOLDIER, 0, random.Rand() );

		Inventory* inventory = unit->GetInventory();
		if ( (i & 1) == 0 )
			inventory->AddItem( Inventory::WEAPON_SLOT, ar3 );
		else
			inventory->AddItem( Inventory::WEAPON_SLOT, plasmaRifle );

		inventory->AddItem( Inventory::ANY_SLOT, Item( clip ));
		inventory->AddItem( Inventory::ANY_SLOT, Item( clip ));
		inventory->AddItem( Inventory::ANY_SLOT, medkit );
		inventory->AddItem( Inventory::ANY_SLOT, armor );
		inventory->AddItem( Inventory::ANY_SLOT, fuel );
		inventory->AddItem( Inventory::ANY_SLOT, gun0 );
		unit->UpdateInventory();
		unit->SetMapPos( pos.x, pos.y );
		unit->SetYRotation( (float)(((i+2)%8)*45) );
	}
	
	const Vector2I alienPos[4] = { 
		{ 16, 21 }, {12, 16 }, { 30, 25 }, { 29, 30 }
	};
	for( int i=0; i<4; ++i ) {
		Unit* unit = &units[ALIEN_UNITS_START+i];

		unit->Init( engine, game, Unit::ALIEN, i&3, random.Rand() );
		Inventory* inventory = unit->GetInventory();
		inventory->AddItem( Inventory::WEAPON_SLOT, gun1 );
		unit->UpdateInventory();
		unit->SetMapPos( alienPos[i] );
	}

	/*
	for( int i=0; i<4; ++i ) {
		Vector2I pos = { (i*2)+10, Z-12 };
		units[CIV_UNITS_START+i].Init( engine, game, Unit::CIVILIAN, 0, random.Rand() );
		units[CIV_UNITS_START+i].SetMapPos( pos.x, pos.y );
	}
	*/
}


void BattleScene::NewTurn( int team )
{
	currentTeamTurn = team;
	switch ( team ) {
		case Unit::SOLDIER:
			GLOUTPUT(( "New Turn: Terran\n" ));
			for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i )
				units[i].NewTurn();
			break;

		case Unit::ALIEN:
			GLOUTPUT(( "New Turn: Alien\n" ));
			for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i )
				units[i].NewTurn();
			break;

		case Unit::CIVILIAN:
			GLOUTPUT(( "New Turn: Civ\n" ));
			for( int i=CIV_UNITS_START; i<CIV_UNITS_END; ++i )
				units[i].NewTurn();
			break;

		default:
			GLASSERT( 0 );
			break;
	}

	// Calc the targets and flush it:
	CalcTeamTargets();
	targetEvents.Clear();

	// Allow the map to change (fire and smoke)
	Rectangle2I change;
	change.SetInvalid();
	engine->GetMap()->DoSubTurn( &change );
	InvalidateAllVisibility( change );

	// Since the map has changed:
	CalcTeamTargets();
	SetFogOfWar();
}


void BattleScene::Save( UFOStream* /*s*/ )
{
	U32 selectionIndex = MAX_UNITS;
	if ( selection.soldierUnit ) {
		selectionIndex = selection.soldierUnit - units;
	}

	BattleSceneStream stream( game );
	stream.Save( selectionIndex, units, &engine->camera, engine->GetMap() );
}


void BattleScene::Load( UFOStream* /*s*/ )
{
	selection.Clear();
	int selected;

	BattleSceneStream stream( game );
	stream.Load( &selected, units, true, &engine->camera, engine->GetMap() );

	if ( selected < MAX_UNITS ) {
		selection.soldierUnit = &units[selected];
	}
}


void BattleScene::SetFogOfWar()
{
	CalcAllVisibility();
	grinliz::BitArray<Map::SIZE, Map::SIZE, 1>* fow = engine->GetMap()->LockFogOfWar();
#ifdef MAPMAKER
	fow->SetAll();
#else
	for( int j=0; j<MAP_SIZE; ++j ) {
		for( int i=0; i<MAP_SIZE; ++i ) {
			Rectangle3I query;
			query.Set( i, j, TERRAN_UNITS_START, i, j, TERRAN_UNITS_END-1 );
			if ( !visibilityMap.IsRectEmpty( query ) )
				fow->Set( i, j );
			else
				fow->Clear( i, j );
		}
	}
#endif
	engine->GetMap()->ReleaseFogOfWar();
}


void BattleScene::TestHitTesting()
{
	/*
	GRINLIZ_PERFTRACK
	for ( int i=0; i<MAX_UNITS; ++i ) {
		if ( units[i].Status() == Unit::STATUS_ALIVE ) {
			int cx = (int)units[i].GetModel()->X();
			int cz = (int)units[i].GetModel()->Z();
			const int DELTA = 10;

			Ray ray;
			ray.origin.Set( (float)cx+0.5f, 1.0f, (float)cz+0.5f );

			for( int x=cx-DELTA; x<cx+DELTA; ++x ) {
				for( int z=cz-DELTA; z<cz+DELTA; ++z ) {
					if ( x>=0 && z>=0 && x<engine->GetMap()->Width() && z<engine->GetMap()->Height() ) {

						Vector3F dest = {(float)x, 1.0f, (float)z};

						ray.direction = dest - ray.origin;
						ray.length = 1.0f;

						Vector3F intersection;
						engine->IntersectModel( ray, TEST_TRI, 0, 0, &intersection );
					}
				}
			}
		}
	}
	*/
}

void BattleScene::DoTick( U32 currentTime, U32 deltaTime )
{
	TestHitTesting();
/*
	if ( currentTime/1000 != (currentTime-deltaTime)/1000 ) {
		grinliz::Vector3F pos = { 10.0f, 1.0f, 28.0f };
		grinliz::Vector3F vel = { 0.0f, 1.0f, 0.0f };
		Color4F col = { 1.0f, -0.5f, 0.0f, 1.0f };
		Color4F colVel = { 0.0f, 0.0f, 0.0f, -1.0f/1.2f };

		game->particleSystem->EmitPoint(	40,		// count
											ParticleSystem::PARTICLE_HEMISPHERE,
											col,	colVel,
											pos,	0.1f,	
											vel,	0.1f,
											1200 );
	}
*/
//	grinliz::Vector3F pos = { 13.f, 0.0f, 28.0f };
// 	game->particleSystem->EmitFlame( deltaTime, pos );

	engine->GetMap()->EmitParticles( deltaTime );

	if (    SelectedSoldier()
		 && SelectedSoldierUnit()->Status() == Unit::STATUS_ALIVE
		 && SelectedSoldierUnit()->Team() == Unit::SOLDIER ) 
	{
		Model* m = SelectedSoldierModel();
		GLASSERT( m );

		float alpha = 0.5f;
		ParticleSystem::Instance()->EmitDecal(	ParticleSystem::DECAL_SELECTION, 
												ParticleSystem::DECAL_BOTTOM,
												m->Pos(), alpha,
												m->GetYRotation() );

		int unitID = SelectedSoldierUnit() - units;
		const float ALPHA = 0.3f;

		/*
		// Debug unit targets.
		for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) {
			if ( targets.terran.alienTargets.IsSet( unitID-TERRAN_UNITS_START, i-ALIEN_UNITS_START ) ) {
				Vector3F p;
				units[i].CalcPos( &p );
				game->particleSystem->EmitDecal( ParticleSystem::DECAL_UNIT_SIGHT,
												 ParticleSystem::DECAL_BOTH,
												 p, ALPHA, 0 );	
			}
		}
		*/
		/*
		// Debug team targets.
		for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) {
			if ( targets.terran.teamAlienTargets.IsSet( i-ALIEN_UNITS_START ) ) {
				Vector3F p;
				units[i].CalcPos( &p );
				game->particleSystem->EmitDecal( ParticleSystem::DECAL_TEAM_SIGHT,
												 ParticleSystem::DECAL_BOTH,
												 p, ALPHA, 0 );	
			}
		}
		*/

		for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) {
			if ( m_targets.CanSee( unitID, i ) ) {
				Vector3F p;
				units[i].CalcPos( &p );
				ParticleSystem::Instance()->EmitDecal(	ParticleSystem::DECAL_TARGET,
														ParticleSystem::DECAL_BOTH,
														p, ALPHA, 0 );	
			}
		}
	}
	bool check = ProcessAction( deltaTime );
	if ( check ) {
		//DumpTargetEvents();
		ProcessDoors();
		StopForNewTeamTarget();
		DoReactionFire();
		targetEvents.Clear();	// All done! They don't get to carry on beyond the moment.
	}

	// Render the target (if it is on-screen)
	if ( HasTarget() ) {
		Vector3F pos = { 0, 0, 0 };
		if ( AlienUnit() )
			AlienUnit()->CalcPos( &pos );
		else
			pos.Set( (float)(selection.targetPos.x) + 0.5f,
					 0.02f,
					 (float)(selection.targetPos.y) + 0.5f );

		// Double up with previous target indicator.
		const float ALPHA = 0.3f;
		ParticleSystem::Instance()->EmitDecal(	ParticleSystem::DECAL_TARGET,
												ParticleSystem::DECAL_BOTH,
												pos, ALPHA, 0 );

		Vector2F r;
		engine->WorldToScreen( pos, &r );
		//GLOUTPUT(( "View: %.1f,%.1f\n", r.x, r.y ));
		int uiX, uiY;
		const Screenport& port = engine->GetScreenport();
		port.ViewToUI( (int)r.x, (int)r.y, &uiX, &uiY );

		int x, y, w, h;
		const int DX = 10;

		// Make sure it fits on the screen.
		fireWidget->CalcDimensions( 0, 0, 0, &h );
		fireWidget->SetOrigin( uiX+DX, uiY-h/2 );
		fireWidget->CalcDimensions( &x, &y, &w, &h );
		if ( x < 0 ) {
			x = 0;
		}
		else if ( x+w >= port.UIWidth() ) {
			x = port.UIWidth() - w;
		}
		if ( y < 0 ) {
			y = 0;
		}
		else if ( y+h >= port.UIHeight() ) {
			y = port.UIHeight() - h;
		}
		fireWidget->SetOrigin( x, y );
	}
}




void BattleScene::ScrollOnScreen( const Vector3F& pos )
{
	Vector2F r;
	engine->WorldToScreen( pos, &r );
	GLOUTPUT(( "screen: %.1f, %.1f\n", r.x, r.y ));

	const Screenport& port = engine->GetScreenport();
	if ( r.x < 0.0f || r.x > port.PhysicalWidth() || r.y < 0.0f || r.y > port.PhysicalHeight() ) {
		// Scroll
		GLOUTPUT(( "Scroll!\n" ));
		Vector3F dest;
		engine->MoveCameraXZ( pos.x, pos.z, &dest );

		Action action;
		InitAction( &action, ACTION_CAMERA );

		const int TIME = 500;
		action.type.camera.start = engine->camera.PosWC();
		action.type.camera.end   = dest;
		action.type.camera.time  = TIME;
		action.type.camera.timeLeft = TIME;
		actionStack.Push( action );
	}
}




void BattleScene::SetSelection( Unit* unit ) 
{
	if ( !unit ) {
		selection.soldierUnit = 0;
		selection.targetUnit = 0;
	}
	else {
		GLASSERT( unit->IsAlive() );

		if ( unit->Team() == Unit::SOLDIER ) {
			selection.soldierUnit = unit;
			selection.targetUnit = 0;
		}
		else if ( unit->Team() == Unit::ALIEN ) {
			GLASSERT( SelectedSoldier() );
			selection.targetUnit = unit;
			selection.targetPos.Set( -1, -1 );

			GLASSERT( uiMode == UIM_NORMAL );
			SetFireWidget();
			uiMode = UIM_FIRE_MENU;
		}
		else {
			GLASSERT( 0 );
		}
	}
}


void BattleScene::PushRotateAction( Unit* src, const Vector3F& dst3F, bool quantize )
{
	GLASSERT( src->GetModel() );

	Vector2I dst = { (int)dst3F.x, (int)dst3F.z };

	float rot = src->AngleBetween( dst, quantize );
	if ( src->GetModel()->GetYRotation() != rot ) {
		Action action;
		action.Init( ACTION_ROTATE, src );
		action.type.rotate.rotation = rot;
		actionStack.Push( action );
	}
}


bool BattleScene::PushShootAction( Unit* unit, const grinliz::Vector3F& target, 
								   int select, int type,
								   bool useError )
{
	GLASSERT( unit );
	GLASSERT( select == 0 || select == 1 );\
	GLASSERT( type >=0 && type < 3 );

	if ( !unit->IsAlive() )
		return false;
	
	Item* weapon = unit->GetWeapon();
	if ( !weapon )
		return false;

	// Stack - push in reverse order.
	int nShots = ( type == AUTO_SHOT ) ? 3 : 1;

	Vector3F normal, right, up, p;
	unit->CalcPos( &p );
	normal = target - p;
	float length = normal.Length();
	normal.Normalize();

	up.Set( 0.0f, 1.0f, 0.0f );
	CrossProduct( normal, up, &right );
	CrossProduct( normal, right, &up );

	if (    unit->GetStats().TU() >= selection.soldierUnit->FireTimeUnits( select, type )
		 && ( nShots <= weapon->RoundsAvailable(select+1)) ) 
	{
		unit->UseTU( selection.soldierUnit->FireTimeUnits( select, type ) );

		for( int i=0; i<nShots; ++i ) {
			Vector3F t = target;
			if ( useError ) {
				float d = length * random.Uniform() * unit->GetStats().Accuracy();
				Matrix4 m;
				m.SetAxisAngle( normal, (float)random.Rand( 360 ) );
				t = target + (m * right)*d;
			}

			Action action;
			action.Init( ACTION_SHOOT, unit );
			action.type.shoot.target = t;
			action.type.shoot.select = select;
			actionStack.Push( action );
			weapon->UseRound( select+1 );
		}
		PushRotateAction( unit, target, true );
		return true;
	}
	return false;
}


void BattleScene::DoReactionFire()
{
	int antiTeam = Unit::ALIEN;
	if ( currentTeamTurn == Unit::ALIEN )
		antiTeam = Unit::SOLDIER;

	bool react = false;
	if ( actionStack.Empty() ) {
		react = true;
	}
	else { 
		const Action& action = actionStack.Top();
		if (    action.action == ACTION_MOVE
			&& action.unit
			&& action.unit->Team() == currentTeamTurn
			&& action.type.move.pathFraction == 0 )
		{
			react = true;		
		}
	}

	if ( react ) {
		int i=0;
		while( i < targetEvents.Size() ) {
			TargetEvent t = targetEvents[i];
			if (    t.team == 0				// individual
				 && t.gain == 1 
				 && units[t.viewerID].Team() == antiTeam
				 && units[t.targetID].Team() == currentTeamTurn ) 
			{
				// Reaction fire

				if ( units[t.targetID].IsAlive() && units[t.targetID].GetModel() ) {
					// Do we really react? Are we that lucky? Well, are you, punk?
					float r = random.Uniform();
					float reaction = units[t.targetID].GetStats().Reaction();
					
					GLOUTPUT(( "reaction fire possible. (if %.2f < %.2f)\n", r, reaction ));

					if ( r <= reaction ) {
						Vector3F target;
						units[t.targetID].GetModel()->CalcTarget( &target );

						int shot = PushShootAction( &units[t.viewerID], target, 0, 1, true );	// auto
						if ( !shot )
							PushShootAction( &units[t.viewerID], target, 0, 0, true );	// snap
					}
				}
				targetEvents.SwapRemove( i );
			}
			else {
				++i;
			}
		}
	}
}


void BattleScene::ProcessDoors()
{
	bool doorChange = false;
	Rectangle2I invalid;
	invalid.SetInvalid();

	// Where are all the units? Then go through and set
	// each door. Setting a door to its current value
	// does nothing.
	BitArray< MAP_SIZE, MAP_SIZE, 1 > map;
	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( units[i].IsAlive() ) {

			Vector2I pos;
			units[i].CalcMapPos( &pos, 0 );
			//GLOUTPUT(( "Set %d,%d\n", pos.x, pos.y ));
			map.Set( pos.x, pos.y, 0 );
		}
	}

	for( int i=0; i<doors.Size(); ++i ) {
		const Vector2I& v = doors[i];
		if (    map.IsSet( v.x, v.y )
			 || map.IsSet( v.x+1, v.y )
			 || map.IsSet( v.x-1, v.y )
			 || map.IsSet( v.x, v.y+1 )
			 || map.IsSet( v.x, v.y-1 ) )
		{
			doorChange |= engine->GetMap()->OpenDoor( v.x, v.y, true );
			invalid.DoUnion( v );
		}
		else {
			doorChange |= engine->GetMap()->OpenDoor( v.x, v.y, false );
			invalid.DoUnion( v );
		}
	}
	if ( doorChange ) {
		InvalidateAllVisibility( invalid );
		SetFogOfWar();
		CalcTeamTargets();
	}
}


void BattleScene::StopForNewTeamTarget()
{
	int antiTeam = Unit::ALIEN;
	if ( currentTeamTurn == Unit::ALIEN )
		antiTeam = Unit::SOLDIER;

	if ( actionStack.Size() == 1 ) {
		const Action& action = actionStack.Top();
		if (    action.action == ACTION_MOVE
			&& action.unit
			&& action.unit->Team() == currentTeamTurn
			&& action.type.move.pathFraction == 0 )
		{
			// THEN check for interuption.
			// Clear out the current team events, look for "new team"
			int i=0;
			bool newTeam = false;
			while( i < targetEvents.Size() ) {
				TargetEvent t = targetEvents[i];
				if (	t.team == 1 
					 && t.gain == 1 
					 && t.viewerID == currentTeamTurn )
				{
					// New sighting!
					GLOUTPUT(( "new team target sighted.\n" ));
					newTeam = true;
					targetEvents.SwapRemove( i );
				}
				else {
					++i;
				}
			}
			if ( newTeam ) {
				actionStack.Clear();
				if ( action.unit == SelectedSoldierUnit() )
					ShowNearPath( action.unit );
			}
		}
	}
}


bool BattleScene::ProcessAction( U32 deltaTime )
{
	bool stackChange = false;

	if ( !actionStack.Empty() )
	{
		Action* action = &actionStack.Top();

		Unit* unit = 0;
		Model* model = 0;

		if ( action->unit ) {
			if ( !action->unit->IsAlive() || !action->unit->GetModel() ) {
				GLASSERT( 0 );	// may be okay, but untested.
				actionStack.Pop();
				return true;
			}
			unit = action->unit;
			model = action->unit->GetModel();
		}

		Vector2I originalPos = { 0, 0 };
		float originalRot = 0.0f;
		float originalTU = 0.0f;
		if ( unit ) {
			unit->CalcMapPos( &originalPos, &originalRot );
			originalTU = unit->GetStats().TU();
		}

		switch ( action->action ) {
			case ACTION_MOVE: 
				{
					// Move the unit. Be careful to never move more than one step (Travel() does not)
					// and be careful to handle rotation between steps so that visibility can update
					// and game events can happen.
					// This means that motion is really of 2 types: x,y and rotation.
					const float SPEED = 3.0f;
					const float ROTATION_SPEED = 150.0f;
					float x, z, r;
					
					// Do we need to rotate, or move?
					path.GetPos( action->type.move.pathStep, action->type.move.pathFraction, &x, &z, &r );
					if ( model->GetYRotation() != r ) {
						// We aren't lined up. Rotate first, then move.
						GLASSERT( model->X() == floorf(x)+0.5f );
						GLASSERT( model->Z() == floorf(z)+0.5f );
						
						float delta, bias;
						MinDeltaDegrees( model->GetYRotation(), r, &delta, &bias );
						float travelRotation = Travel( deltaTime, ROTATION_SPEED );
						delta -= travelRotation;
						if ( delta <= 0.0f ) {
							// Done rotating! Next time we'll move.
							Vector3F p = { model->X(), 0.0f, model->Z() };
							unit->SetPos( p, r );
							stackChange = true;	// not a real stack change, but a change in the path loc.
						}
						else {
							Vector3F p = { model->X(), 0.0f, model->Z() };
							unit->SetPos( p, model->GetYRotation() + travelRotation*bias );
						}
					}
					else {
						float travel = Travel( deltaTime, SPEED );

						while(    (action->type.move.pathStep < (int)(path.statePath.size()-1))
							   && travel > 0.0f ) 
						{
							path.Travel( &travel, &action->type.move.pathStep, &action->type.move.pathFraction );
							if ( action->type.move.pathFraction == 0.0f ) {
								// crossed a path boundary.
								GLASSERT( unit->GetStats().TU() >= 1.0 );	// one move is one TU
								
								Vector2<S16> v0 = path.GetPathAt( action->type.move.pathStep-1 );
								Vector2<S16> v1 = path.GetPathAt( action->type.move.pathStep );
								int d = abs( v0.x-v1.x ) + abs( v0.y-v1.y );

								if ( d == 1 )
									unit->UseTU( 1.0f );
								else if ( d == 2 )
									unit->UseTU( 1.41f );
								else { GLASSERT( 0 ); }

								stackChange = true;	// not a real stack change, but a change in the path loc.
								break;
							}
						}
						path.GetPos( action->type.move.pathStep, action->type.move.pathFraction, &x, &z, &r );

						Vector3F v = { x+0.5f, 0.0f, z+0.5f };
						unit->SetPos( v, model->GetYRotation() );

						if ( action->type.move.pathStep == path.statePath.size()-1 ) {
							actionStack.Pop();
							path.Clear();
							stackChange = true;
						}
					}
				}
				break;

			case ACTION_ROTATE:
				{
					const float ROTSPEED = 400.0f;
					float travel = Travel( deltaTime, ROTSPEED );

					float delta, bias;
					MinDeltaDegrees( model->GetYRotation(), action->type.rotate.rotation, &delta, &bias );

					if ( delta <= travel ) {
						unit->SetYRotation( action->type.rotate.rotation );
						actionStack.Pop();
						stackChange = true;
					}
					else {
						unit->SetYRotation( model->GetYRotation() + bias*travel );
					}
				}
				break;

			case ACTION_SHOOT:
				stackChange = ProcessActionShoot( action, unit, model );
				break;

			case ACTION_HIT:
				stackChange = ProcessActionHit( action );
				break;

			case ACTION_DELAY:
				{
					if ( deltaTime >= action->type.delay.delay ) {
						actionStack.Pop();
					}
					else {
						action->type.delay.delay -= deltaTime;
					}
				}
				break;

			case ACTION_CAMERA:
				{
					action->type.camera.timeLeft -= deltaTime;
					if ( action->type.camera.timeLeft > 0 ) {
						Vector3F v;
						for( int i=0; i<3; ++i ) {
							v.X(i) = Interpolate(	(float)action->type.camera.time, 
													action->type.camera.start.X(i),
													0.0f,							
													action->type.camera.end.X(i),
													(float)action->type.camera.timeLeft );
						}
						engine->camera.SetPosWC( v );
					}
					else {
						actionStack.Pop();
					}
				}
				break;

			default:
				GLASSERT( 0 );
				break;
		}
		if ( unit ) {
			Vector2I newPos;
			float newRot;
			unit->CalcMapPos( &newPos, &newRot );

			// If we changed map position, update UI feedback.
			if ( newPos != originalPos || newRot != originalRot ) {
				SetFogOfWar();
				CalcTeamTargets();
			}

			// If actions are empty and this is the selected unit, update
			// the path feedback.
			if ( actionStack.Empty() && unit == SelectedSoldierUnit() ) {
				ShowNearPath( unit );
			}
		}
	}
	return stackChange;
}


bool BattleScene::ProcessActionShoot( Action* action, Unit* unit, Model* model )
{
	DamageDesc damageDesc;
	bool impact = false;
	Model* modelHit = 0;
	Vector3F intersection;
	U32 delayTime = 0;
	int select = action->type.shoot.select;
	GLASSERT( select == 0 || select == 1 );
	const WeaponItemDef* weaponDef = 0;
	Ray ray;

	if ( unit && model && unit->IsAlive() ) {
		Vector3F p0, p1;
		Vector3F beam0 = { 0, 0, 0 }, beam1 = { 0, 0, 0 };

		const Item* weaponItem = unit->GetWeapon();
		weaponDef = weaponItem->GetItemDef()->IsWeapon();
		GLASSERT( weaponDef );

		model->CalcTrigger( &p0 );
		p1 = action->type.shoot.target;

		ray.origin = p0;
		ray.direction = p1-p0;

		// What can we hit?
		// model
		//		unit, alive (does damage)
		//		unit, dead (does nothing)
		//		model, world (does damage)
		//		gun, does nothing
		// ground / bounds

		// Don't hit the shooter:
		const Model* ignore[] = { unit->GetModel(), unit->GetWeaponModel(), 0 };
		Model* m = engine->IntersectModel( ray, TEST_TRI, 0, 0, ignore, &intersection );

		if ( m && intersection.y < 0.0f ) {
			// hit ground before the unit (intesection is with the part under ground)
			impact = true;
		}

		weaponDef->DamageBase( select, &damageDesc );

		if ( m ) {
			impact = true;
			beam0 = p0;
			beam1 = intersection;
			modelHit = m;
		}
		else {		
			Vector3F in, out;
			int inResult, outResult;
			Rectangle3F worldBounds;
			worldBounds.Set( 0, 0, 0, (float)engine->GetMap()->Width(), 4.0f, (float)engine->GetMap()->Height() );	// FIXME: who owns these constants???

			int result = IntersectRayAllAABB( ray.origin, ray.direction, worldBounds, 
											  &inResult, &in, &outResult, &out );

			GLASSERT( result == grinliz::INTERSECT );
			if ( result == grinliz::INTERSECT ) {
				beam0 = p0;
				beam1 = out;

				if ( out.y < 0.01 ) {
					// hit the ground
					impact = true;
				}
			}
		}

		if ( beam0 != beam1 ) {
			weaponDef->RenderWeapon( select,
									 ParticleSystem::Instance(),
									 beam0, beam1, 
									 impact, 
									 game->CurrentTime(), 
									 &delayTime );
		}
	}
	actionStack.Pop();

	if ( impact ) {
		GLASSERT( weaponDef );

		Action h;
		h.Init( ACTION_HIT, 0 );
		h.type.hit.damageDesc = damageDesc;
		h.type.hit.explosive = (weaponDef->weapon[select].flags & WEAPON_EXPLOSIVE) ? true : false;
		h.type.hit.p = intersection;
		
		h.type.hit.n = ray.direction;
		h.type.hit.n.Normalize();

		h.type.hit.m = modelHit;
		actionStack.Push( h );
	}

	if ( delayTime ) {
		Action a;
		a.Init( ACTION_DELAY, 0 );
		a.type.delay.delay = delayTime;
		actionStack.Push( a );
	}
	return true;
}


bool BattleScene::ProcessActionHit( Action* action )
{
	Rectangle2I destroyed;
	destroyed.SetInvalid();

	if ( !action->type.hit.explosive ) {
		// Apply direct hit damage
		Model* m = action->type.hit.m;
		Unit* hitUnit = 0;

		if ( m ) 
			hitUnit = UnitFromModel( m );
		if ( hitUnit ) {
			if ( hitUnit->IsAlive() ) {

				hitUnit->DoDamage( action->type.hit.damageDesc );
				if ( !hitUnit->IsAlive() ) {
					selection.ClearTarget();			
					// visibility invalidated automatically when unit killed
				}
				GLOUTPUT(( "Hit Unit 0x%x hp=%d/%d\n", (unsigned)hitUnit, (int)hitUnit->GetStats().HP(), (int)hitUnit->GetStats().TotalHP() ));
			}
		}
		else if ( m && m->IsFlagSet( Model::MODEL_OWNED_BY_MAP ) ) {
			Rectangle2I bounds;
			engine->GetMap()->MapBoundsOfModel( m, &bounds );

			// Hit world object.
			engine->GetMap()->DoDamage( m, action->type.hit.damageDesc, &destroyed );
		}
	}
	else {
		// Explosion
		const int MAX_RAD = 2;
		const int MAX_RAD_2 = MAX_RAD*MAX_RAD;

		// There is a small offset to move the explosion back towards the shooter.
		// If it hits a wall (common) this will move it to the previous square.
		// Also means a model hit may be a "near miss"...but explosions are messy.
		const int x0 = (int)(action->type.hit.p.x - 0.2f*action->type.hit.n.x);
		const int y0 = (int)(action->type.hit.p.z - 0.2f*action->type.hit.n.z);
		// generates smoke:
		destroyed.Set( x0-MAX_RAD, y0-MAX_RAD, x0+MAX_RAD, y0+MAX_RAD );

		for( int rad=0; rad<=MAX_RAD; ++rad ) {
			DamageDesc dd = action->type.hit.damageDesc;
			dd.Scale( (float)(1+MAX_RAD-rad) / (float)(1+MAX_RAD) );

			for( int y=y0-rad; y<=y0+rad; ++y ) {
				for( int x=x0-rad; x<=x0+rad; ++x ) {
					if ( x==(x0-rad) || x==(x0+rad) || y==(y0-rad) || y==(y0+rad) ) {
						
						// Tried to do this with the pather, but the issue with 
						// walls being on the inside and outside of squares got 
						// ugly. But the other option - ray casting - also nasty.
						// Go one further than could be walked.
						int radius2 = (x-x0)*(x-x0) + (y-y0)*(y-y0);
						if ( radius2 > MAX_RAD_2 )
							continue;
						
						// can the tile to be damaged be reached by the explosion?
						// visibility is checked up to the tile before this one, else
						// it is possible to miss because "you can't see yourself"
						bool canSee = true;
						if ( rad > 0 ) {
							LineWalk walk( x0, y0, x, y );
							// Actually 'less than' so that damage goes through walls a little.
							while( walk.CurrentStep() < walk.NumSteps() ) {
								Vector2I p = walk.P();
								Vector2I q = walk.Q();

								// Was using CanSee, but that has the problem that
								// some walls are inner and some walls are outer.
								// *sigh* Use the pather.
								if ( !engine->GetMap()->CanSee( p, q ) ) {
									canSee = false;
									break;
								}
								walk.Step();
							}
							// Go with the sight check to see if the explosion can
							// reach this tile.
							if ( !canSee )
								continue;
						}

						Unit* unit = GetUnitFromTile( x, y );
						if ( unit && unit->IsAlive() ) {
							unit->DoDamage( dd );
							if ( !unit->IsAlive() && unit == SelectedSoldierUnit() ) {
								selection.ClearTarget();			
							}
						}
						bool hitAnything = false;
						engine->GetMap()->DoDamage( x, y, dd, &destroyed );

						// Where to add smoke?
						// - if we hit anything
						// - change of smoke anyway
						if ( hitAnything || random.Bit() ) {
							int turns = 4 + random.Rand( 4 );
							engine->GetMap()->AddSmoke( x, y, turns );
						}
					}
				}
			}
		}
	}
	if ( destroyed.IsValid() ) {
		// The MAP changed - reset all views.
		InvalidateAllVisibility( destroyed );
		SetFogOfWar();
	}
	CalcTeamTargets();
	actionStack.Pop();
	return true;
}


bool BattleScene::HandleIconTap( int vX, int vY )
{
	int screenX, screenY;
	engine->GetScreenport().ViewToUI( vX, vY, &screenX, &screenY );

	int icon = -1;

	if ( uiMode == UIM_FIRE_MENU ) {
		icon = fireWidget->QueryTap( screenX, screenY );

		/*
			pri		sec
			4aimed	5aimed
			2auto	3auto
			0snap	1snap
		*/

		if ( icon >= 0 && icon < 6 ) {
			int select = icon & 1;	// primary(0) or secondary(1)
			int type = icon / 2;

			// shooting creates a turn action then a shoot action.
			GLASSERT( selection.soldierUnit >= 0 );
			GLASSERT( selection.targetUnit >= 0 );

			Vector3F target;
			if ( selection.targetPos.x >= 0 ) {
				target.Set( (float)selection.targetPos.x + 0.5f, 1.0f, (float)selection.targetPos.y + 0.5f );
			}
			else {
				selection.targetUnit->GetModel()->CalcTarget( &target );
			}
			PushShootAction( selection.soldierUnit, target, select, type, true );
			selection.targetUnit = 0;
		}
	}
	else if ( uiMode == UIM_TARGET_TILE ) {
		icon = widgets->QueryTap( screenX, screenY );
		if ( icon == BTN_TARGET ) {
			uiMode = UIM_NORMAL;
		}
	}
	else {
		icon = widgets->QueryTap( screenX, screenY );

		switch( icon ) {

			case BTN_TARGET:
				{
					uiMode = UIM_TARGET_TILE;
				}
				break;
/*
			case BTN_LEFT:
			case BTN_RIGHT:
				if ( actionStack.Empty() && SelectedSoldierUnit() ) {
					Unit* unit = SelectedSoldierUnit();
					const Stats& stats = unit->GetStats();
					if ( stats.TU() >= TU_TURN ) {
						float r = unit->GetModel()->GetYRotation();
						r += ( icon == BTN_LEFT ) ? 45.0f : -45.f;
						r = NormalizeAngleDegrees( r );

						Action action;
						action.Init( ACTION_ROTATE, unit );
						action.type.rotate.rotation = r;
						actionStack.Push( action );

						unit->UseTU( TU_TURN );
					}
				}
				break;
*/
			case BTN_CHAR_SCREEN:
				if ( actionStack.Empty() && SelectedSoldierUnit() ) {
					UFOStream* stream = game->OpenStream( "SingleUnit" );
					stream->WriteU8( (U8)(SelectedSoldierUnit()-units ) );
					SelectedSoldierUnit()->Save( stream );
					game->PushScene( Game::CHARACTER_SCENE );
				}
				break;

			case BTN_END_TURN:
				SetSelection( 0 );
				engine->GetMap()->ClearNearPath();
				NewTurn( Unit::ALIEN );
				NewTurn( Unit::CIVILIAN );
				NewTurn( Unit::SOLDIER );	// FIXME: should go alien or civ
				break;

			case BTN_NEXT:
			//case BTN_NEXT_DONE:
				{
					int index = TERRAN_UNITS_END-1;
					if ( SelectedSoldierUnit() ) {
						index = SelectedSoldierUnit() - units;
						//if ( icon == BTN_NEXT_DONE )
						//	units[index].SetUserDone();
					}
					int i = index+1;
					if ( i == TERRAN_UNITS_END )
						i = TERRAN_UNITS_START;
					while( i != index ) {
						if ( units[i].IsAlive() && !units[i].IsUserDone() ) {
							SetSelection( &units[i] );
							ScrollOnScreen( units[i].GetModel()->Pos() );
							break;
						}
						++i;
						if ( i == TERRAN_UNITS_END )
							i = TERRAN_UNITS_START;
					}
					ShowNearPath( selection.soldierUnit );
				}
				break;

			default:
				break;
		}
	}
	return icon >= 0;
}


void BattleScene::Tap(	int tap, 
						const grinliz::Vector2I& screen,
						const grinliz::Ray& world )
{
	if ( tap > 1 )
		return;
	if ( !actionStack.Empty() )
		return;

	/* Modes:
		- if target is up, it needs to be selected or cleared.
		- if targetMode is on, wait for tile selection or targetMode off
		- else normal mode

	*/

	if ( uiMode == UIM_NORMAL ) {
		bool iconSelected = HandleIconTap( screen.x, screen.y );
		if ( iconSelected )
			return;
	}
	else if ( uiMode == UIM_TARGET_TILE ) {
		bool iconSelected = HandleIconTap( screen.x, screen.y );
		// If the mode was turned off, return. Else the selection is handled below.
		if ( iconSelected )
			return;
	}
	else if ( uiMode == UIM_FIRE_MENU ) {
		HandleIconTap( screen.x, screen.y );
		// Whether or not something was selected, drop back to normal mode.
		uiMode = UIM_NORMAL;
		selection.targetPos.Set( -1, -1 );
		selection.targetUnit = 0;
		ShowNearPath( selection.soldierUnit );
		return;
	}

#ifdef MAPMAKER
	const Vector3F& pos = mapSelection->Pos();
	int rotation = (int) (mapSelection->GetYRotation() / 90.0f );

	engine->GetMap()->AddItem( (int)pos.x, (int)pos.z, rotation, currentMapItem, -1, 0 );
#endif	

	// Get the map intersection. May be used by TARGET_TILE or NORMAL
	Vector3F intersect;
	Map* map = engine->GetMap();

	Vector2I tilePos = { 0, 0 };
	bool hasTilePos = false;

	int result = IntersectRayPlane( world.origin, world.direction, 1, 0.0f, &intersect );
	if ( result == grinliz::INTERSECT && intersect.x >= 0 && intersect.x < Map::SIZE && intersect.z >= 0 && intersect.z < Map::SIZE ) {
		tilePos.Set( (int)intersect.x, (int) intersect.z );
		hasTilePos = true;
	}

	if ( uiMode == UIM_TARGET_TILE ) {
		if ( hasTilePos ) {
			selection.targetUnit = 0;
			selection.targetPos.Set( tilePos.x, tilePos.y );
			SetFireWidget();
			uiMode = UIM_FIRE_MENU;
		}
		return;
	}

	GLASSERT( uiMode == UIM_NORMAL );

	// We didn't tap a button.
	// What got tapped? First look to see if a SELECTABLE model was tapped. If not, 
	// look for a selectable model from the tile.

	// If there is a selected model, then we can tap a target model.
	bool canSelectAlien = SelectedSoldier();			// a soldier is selected

	for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i ) {
		if ( units[i].GetModel() ) units[i].GetModel()->ClearFlag( Model::MODEL_SELECTABLE );

		if ( units[i].IsAlive() ) {
			GLASSERT( units[i].GetModel() );
			units[i].GetModel()->SetFlag( Model::MODEL_SELECTABLE );
		}
	}
	for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) {
		if ( units[i].GetModel() ) units[i].GetModel()->ClearFlag( Model::MODEL_SELECTABLE );

		if ( canSelectAlien && units[i].IsAlive() ) {
			GLASSERT( units[i].GetModel() );
			units[i].GetModel()->SetFlag( Model::MODEL_SELECTABLE );
		}
	}

	Model* tappedModel = engine->IntersectModel( world, TEST_HIT_AABB, Model::MODEL_SELECTABLE, 0, 0, 0 );
	const Unit* tappedUnit = UnitFromModel( tappedModel );

	if ( tappedModel && tappedUnit ) {
		if ( tappedUnit->Team() == Unit::ALIEN ) {
			SetSelection( UnitFromModel( tappedModel ) );		// sets either the Alien or the Unit
			map->ClearNearPath();
		}
		else if ( tappedUnit->Team() == Unit::SOLDIER ) {
			SetSelection( UnitFromModel( tappedModel ) );
			ShowNearPath( tappedUnit );
		}
		else {
			SetSelection( 0 );
			map->ClearNearPath();
		}
	}
	else if ( !tappedModel ) {
		// Not a model - use the tile
		if ( SelectedSoldierModel() && !selection.targetUnit && hasTilePos ) {
			Vector2<S16> start   = { (S16)SelectedSoldierModel()->X(), (S16)SelectedSoldierModel()->Z() };

			Vector2<S16> end = { (S16)tilePos.x, (S16)tilePos.y };

			// Compute the path:
			float cost;
			SetPathBlocks();
			const Stats& stats = selection.soldierUnit->GetStats();

			int result = engine->GetMap()->SolvePath( start, end, &cost, &path.statePath );
			if ( result == micropather::MicroPather::SOLVED && cost <= stats.TU() ) {
				// TU for a move gets used up "as we go" to account for reaction fire and changes.
				// Go!
				Action action;
				action.Init( ACTION_MOVE, SelectedSoldierUnit() );
				actionStack.Push( action );

				engine->GetMap()->ClearNearPath();
			}
		}
	}
}


void BattleScene::SetFireWidget()
{
	GLASSERT( SelectedSoldierUnit() );
	GLASSERT( SelectedSoldierModel() );

	Unit* unit = SelectedSoldierUnit();
	Item* item = unit->GetWeapon();
	char buffer0[16];
	char buffer1[16];

	Vector3F target;
	if ( selection.targetPos.x >= 0 ) {
		target.Set( (float)selection.targetPos.x+0.5f, 1.0f, (float)selection.targetPos.y+0.5f );
	}
	else {
		target = selection.targetUnit->GetModel()->Pos();
	}
	Vector3F distVector = target - SelectedSoldierModel()->Pos();
	float distToTarget = distVector.Length();

	if ( !item || !item->IsWeapon() ) {
		fireWidget->SetDeco( 6, DECO_NONE );
		fireWidget->SetDeco( 7, DECO_NONE );
		for( int i=0; i<6; ++i ) {
			fireWidget->SetEnabled( i, false );
		}
		return;
	}

	for( int select=0; select<2; ++select ) {

		const WeaponItemDef* wid = item->IsWeapon();
		GLASSERT( wid );

		//float d=0;
		if ( item->HasPart( select+1 ) ) {
			fireWidget->SetDeco( 6+select, item->Deco(select+1) );
			
			DamageDesc dd;
			wid->DamageBase( select, &dd );

			SNPrintf( buffer0, 16, "D%d", (int)dd.Total() );
			SNPrintf( buffer1, 16, "R%d", item->RoundsAvailable(select+1) );
			fireWidget->SetText( 6+select, buffer0, buffer1 );
		}
		else {
			fireWidget->SetDeco( 6+select, DECO_NONE );
			fireWidget->SetText( 6+select, "" );
		}

		const int id[] = { DECO_SNAP, DECO_AUTO, DECO_AIMED };

		for( int type=0; type<3; ++type ) {
			float tu = 0.0f;
			float fraction = 0;
			float dptu = 0;
			int nShots = (type==AUTO_SHOT) ? 3 : 1;
			
			// weird syntax aids debugging.
			bool enable =		true;	//item->HasPart(select+1); cell weapons don't have part 2
			enable = enable &&	item->IsClip( 1 );	// cell or clip
			enable = enable &&	item->IsWeapon();
			enable = enable &&	item->IsWeapon()->SupportsType( select, type );
			enable = enable &&	(nShots <= item->RoundsAvailable(select+1));
			enable = enable &&  unit->GetStats().TU() >= unit->FireTimeUnits( select, type );

			if ( enable ) {
				unit->FireStatistics( select, type, distToTarget, &fraction, &tu, &dptu );
				SNPrintf( buffer0, 16, "%d%%", (int)LRintf( fraction*100.0f ) );

				if ( tu >= 10.0f && dptu >= 10.0f )
					SNPrintf( buffer1, 16, "%2d %2d", LRintf(tu), LRintf(dptu) );
				else if ( tu < 10.0f && dptu >= 10.0f )
					SNPrintf( buffer1, 16, "%.1f %2d", tu, LRintf(dptu) );
				else if ( tu >= 10.0f && dptu < 10.0f )
					SNPrintf( buffer1, 16, "%2d %.1f", LRintf(tu), dptu );
				else if ( tu < 10.0f && dptu < 10.0f )
					SNPrintf( buffer1, 16, "%.1f %.1f", tu, dptu );

				fireWidget->SetText( type*2+select, buffer0, buffer1 );
			}
			else {
				fireWidget->SetText( type*2+select, "" );
			}
			fireWidget->SetEnabled( type*2+select, enable );
			fireWidget->SetDeco( type*2+select, id[type] );
		}
	}
}


void BattleScene::ShowNearPath( const Unit* unit )
{
	if ( !unit ) {
		engine->GetMap()->ClearNearPath();
		return;
	}

	GLASSERT( unit );
	GLASSERT( unit->GetModel() );
	const Model* model = unit->GetModel();
	Vector2<S16> start = { (S16)model->X(), (S16)model->Z() };

	SetPathBlocks();
	const Stats& stats = unit->GetStats();

	float snappedTU = unit->FireTimeUnits( 0, SNAP_SHOT );
	float autoTU = unit->FireTimeUnits( 0, AUTO_SHOT );

	if ( autoTU > 0.0f ) {
		engine->GetMap()->ShowNearPath( start, stats.TU()-autoTU, stats.TU()-snappedTU, stats.TU() );
	}
	else {
		engine->GetMap()->ShowNearPath( start, stats.TU()-snappedTU, -1, stats.TU() );
	}
}


void BattleScene::SetPathBlocks()
{
	Map* map = engine->GetMap();
	map->ClearPathBlocks();

	for( int i=0; i<MAX_UNITS; ++i ) {
		if (    units[i].Status() == Unit::STATUS_ALIVE 
			 && units[i].GetModel() 
			 && &units[i] != selection.soldierUnit ) // oops - don't cause self to not path
		{
			int x = (int)units[i].GetModel()->X();
			int z = (int)units[i].GetModel()->Z();
			map->SetPathBlock( x, z );
		}
	}
}

Unit* BattleScene::UnitFromModel( Model* m )
{
	if ( m ) {
		for( int i=0; i<MAX_UNITS; ++i ) {
			if ( units[i].GetModel() == m )
				return &units[i];
		}
	}
	return 0;
}


Unit* BattleScene::GetUnitFromTile( int x, int z )
{
	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( units[i].GetModel() ) {
			int ux = (int)units[i].GetModel()->X();
			if ( ux == x ) {
				int uz = (int)units[i].GetModel()->Z();
				if ( uz == z ) { 
					return &units[i];
				}
			}
		}
	}
	return 0;
}


void BattleScene::DumpTargetEvents()
{
	for( int i=0; i<targetEvents.Size(); ++i ) {
		const TargetEvent& e = targetEvents[i];
		const char* teams[] = { "Terran", "Civ", "Alien" };

		if ( !e.team ) {
			GLOUTPUT(( "%s Unit %d %s %s %d\n",
						teams[ units[e.viewerID].Team() ],
						e.viewerID, 
						e.gain ? "gain" : "loss", 
						teams[ units[e.targetID].Team() ],
						e.targetID ));
		}
		else {
			GLOUTPUT(( "%s Team %s %s %d\n",
						teams[ e.viewerID ],
						e.gain ? "gain" : "loss", 
						teams[ units[e.targetID].Team() ],
						e.targetID ));
		}
	}
}


void BattleScene::CalcTeamTargets()
{
	// generate events.
	// - if team gets/loses target
	// - if unit gets/loses target

	Targets old = m_targets;
	m_targets.Clear();
	CalcAllVisibility();

	Vector2I targets[] = { { ALIEN_UNITS_START, ALIEN_UNITS_END },   { TERRAN_UNITS_START, TERRAN_UNITS_END } };
	Vector2I viewers[] = { { TERRAN_UNITS_START, TERRAN_UNITS_END }, { ALIEN_UNITS_START, ALIEN_UNITS_END } };
	int viewerTeam[3]  = { Unit::SOLDIER, Unit::ALIEN };

	for( int range=0; range<2; ++range ) {
		// Terran to Alien
		// Go through each alien, and check #terrans that can target it.
		//
		for( int j=targets[range].x; j<targets[range].y; ++j ) {
			// Push IsAlive() checks into the cases below, so that a unit
			// death creates a visibility change. However, initialization 
			// happens at the beginning, so we can skip that.

			if ( !units[j].InUse() )
				continue;

			Vector2I mapPos;
			units[j].CalcMapPos( &mapPos, 0 );
			
			for( int k=viewers[range].x; k<viewers[range].y; ++k ) {
				// Main test: can a terran see an alien at this location?
				if (    units[k].IsAlive() 
					 && units[j].IsAlive() 
					 && visibilityMap.IsSet( mapPos.x, mapPos.y, k ) ) 
				{
					m_targets.Set( k, j );
				}

				// check unit change.
				if ( old.CanSee( k, j ) && !m_targets.CanSee( k, j ) ) 
				{
					// Lost unit.
					TargetEvent e = { 0, 0, k, j };
					targetEvents.Push( e );
				}
				else if ( !old.CanSee( k, j )  && m_targets.CanSee( k, j ) ) 
				{
					// Gain unit.
					TargetEvent e = { 0, 1, k, j };
					targetEvents.Push( e );
				}
			}
			// Check team change.
			if ( old.TeamCanSee( viewerTeam[range], j ) && !m_targets.TeamCanSee( viewerTeam[range], j ) )
			{	
				TargetEvent e = { 1, 0, viewerTeam[range], j };
				targetEvents.Push( e );
			}
			else if ( !old.TeamCanSee( viewerTeam[range], j ) && m_targets.TeamCanSee( viewerTeam[range], j ) )
			{	
				TargetEvent e = { 1, 1, viewerTeam[range], j };
				targetEvents.Push( e );
			}
		}
	}
}


void BattleScene::InvalidateAllVisibility( const Rectangle2I& bounds )
{
	if ( !bounds.IsValid() )
		return;

	Vector2I range[2] = {{ TERRAN_UNITS_START, TERRAN_UNITS_END }, {ALIEN_UNITS_START, ALIEN_UNITS_END}};
	Rectangle2I vis;

	for( int k=0; k<2; ++k ) {
		for( int i=range[k].x; i<range[k].y; ++i ) {
			if ( units[i].IsAlive() ) {
				units[i].CalcVisBounds( &vis );
				if ( bounds.Intersect( vis ) ) 
					units[i].SetVisibilityCurrent( false );
			}
		}
	}
}


void BattleScene::InvalidateAllVisibility()
{
	Vector2I range[2] = {{ TERRAN_UNITS_START, TERRAN_UNITS_END }, {ALIEN_UNITS_START, ALIEN_UNITS_END}};
	for( int k=0; k<2; ++k ) {
		for( int i=range[k].x; i<range[k].y; ++i ) {
			if ( units[i].IsAlive() ) {
				units[i].SetVisibilityCurrent( false );
			}
		}
	}
}


/*	Huge ol' performance bottleneck.
	The CalcVis() is pretty good (or at least doesn't chew up too much time)
	but the CalcAll() is terrible.
	Debug mode.
	Start: 141 MClocks
	Moving to "smart recursion": 18 MClocks - that's good! That's good enough to hide the cost is caching.
		...but also has lots of artifacts in visibility.
	Switched to a "cached ray" approach. 58 MClocks. Much better, correct results, and can be optimized more.

	In Core clocks:
	"cached ray" 45 clocks
	33 clocks after tuning. (About 1/4 of initial cost.)
	Tighter walk: 29 MClocks

	Back on the Atom:
	88 MClocks. But...experimenting with switching to 360degree view.
	...now 79 MClocks. That makes little sense. Did facing take a bunch of cycles??
*/
void BattleScene::CalcAllVisibility()
{
	QuickProfile qp( "CalcAllVisibility()" );
	Vector2I range[2] = {{ TERRAN_UNITS_START, TERRAN_UNITS_END }, {ALIEN_UNITS_START, ALIEN_UNITS_END}};

	for( int k=0; k<2; ++k ) {
		for( int i=range[k].x; i<range[k].y; ++i ) {
			if ( units[i].IsAlive() ) {
				if ( !units[i].VisibilityCurrent() ) {
					CalcUnitVisibility( &units[i] );
					units[i].SetVisibilityCurrent( true );
				}
			}
			else {
				if ( !units[i].VisibilityCurrent() ) {
					visibilityMap.ClearPlane( i );
					units[i].SetVisibilityCurrent( true );
				}
			}
		}
	}
}


void BattleScene::CalcUnitVisibility( const Unit* unit )
{
	//unit = units;	// debugging: 1st unit only

	int unitID = unit - units;
	GLASSERT( unitID >= 0 && unitID < MAX_UNITS );
	Vector2I pos;
	float rotation;
	unit->CalcMapPos( &pos, &rotation );
	int r = NormalizeAngleDegrees( (int)LRintf( rotation ) );

	// Clear out the old settings.
	// Walk the area in range around the unit and cast rays.
	visibilityMap.ClearPlane( unitID );
	visibilityProcessed.ClearAll();

//	Vector2I facing = { 0, 0 };	// only used in debug checks and expensive to compute.
//#ifdef DEBUG
//	facing.x = LRintf((float)MAX_EYESIGHT_RANGE*sinf(ToRadian(rotation)));
//	facing.y = LRintf((float)MAX_EYESIGHT_RANGE*cosf(ToRadian(rotation)));
//#endif

	Rectangle2I mapBounds;
	engine->GetMap()->CalcBounds( &mapBounds );

	// Can always see yourself.
	visibilityMap.Set( pos.x, pos.y, unitID );
	visibilityProcessed.Set( pos.x, pos.y, 0 );

	const int MAX_SIGHT_SQUARED = MAX_EYESIGHT_RANGE*MAX_EYESIGHT_RANGE;

	for( int r=MAX_EYESIGHT_RANGE; r>0; --r ) {
		Vector2I p = { pos.x-r, pos.y-r };
		Vector2I delta[4] = { { 1,0 }, {0,1}, {-1,0}, {0,-1} };

		for( int k=0; k<4; ++k ) {
			for( int i=0; i<r*2; ++i ) {
				if (    mapBounds.Contains( p )
					 && !visibilityProcessed.IsSet( p.x, p.y )
					 && (p-pos).LengthSquared() <= MAX_SIGHT_SQUARED ) 
				{
					CalcVisibilityRay( unitID, p, pos );
				}
				p += delta[k];
			}
		}
	}

/*	// Remembering rotation of 0 is staring down the z axis.
	Vector2I v;		// the starting (far) point of the view triangle. (2D frustum).
	Vector2I dV[2];	// change in V per step.
	Vector2I m;		// direction to walk
	int      len;	// length to walk
	int		 steps;

	switch ( r ) {
		case 0:
		case 90:
		case 180:
		case 270:
			{
				Vector2I offset = { -MAX_EYESIGHT_RANGE, MAX_EYESIGHT_RANGE };
				dV[0].Set( 0, -1 );
				dV[1].Set( 0, -1 );
				m.Set( 1, 0 );
				len = MAX_EYESIGHT_RANGE*2 + 1;
				steps = MAX_EYESIGHT_RANGE + 1;

				Matrix2I rot;
				rot.SetRotation( r );

				v = pos + rot*offset;
				dV[0] = rot * dV[0];
				dV[1] = rot * dV[1];
				m = rot * m;
			}
			break;

		case 45:
		case 135:
		case 225:
		case 315:
			{
				Vector2I offset = { 0, MAX_EYESIGHT_RANGE_45*2 };
				dV[0].Set( -1, 0 );
				dV[1].Set( 0, -1 );
				m.Set( 1, -1 );
				len = MAX_EYESIGHT_RANGE_45*2+1;
				steps = MAX_EYESIGHT_RANGE_45*2+1;

				Matrix2I rot;
				rot.SetRotation( r - 45 );

				v = pos + rot*offset;
				dV[0] = rot * dV[0];
				dV[1] = rot * dV[1];
				m = rot * m;
			}
			break;

		default:
			GLASSERT( 0 );	
			break;
	}

	const int MAX_SIGHT_SQUARED = MAX_EYESIGHT_RANGE*MAX_EYESIGHT_RANGE;

	for( int k=0; k<steps; ++k ) {
		for( int i=0; i<len; ++i ) {
			Vector2I p = v + m*i;

			if (    mapBounds.Contains( p )
				 && !visibilityProcessed.IsSet( p.x, p.y )
				 && (p-pos).LengthSquared() <= MAX_SIGHT_SQUARED ) 
			{
				CalcVisibilityRay( unitID, p, pos, facing );
			}
		}
		v += dV[k&1];
	}
*/

	/* This code is worth keeping because it would work for any rotation.
	Rectangle2I visBounds;
	visBounds.Set( pos.x, pos.y, pos.x, pos.y );

	Vector2I fL = facing; fL.RotateNeg90();
	Vector2I fR = facing; fR.RotatePos90();
	Vector2I fDelta[4] = { pos+fL, pos+fL+facing, pos+fR, pos+fR+facing };

	for( int i=0; i<4; ++i ) {
		visBounds.DoUnion( fDelta[i].x, fDelta[i].y );
	}

	int x0, y0, xBias, yBias;
	int w = visBounds.Width();
	int h = visBounds.Height();

	if ( facing.x >= 0 ) {
		xBias = -1;
		x0 = visBounds.max.x;
	}
	else {
		xBias = 1;
		x0 = visBounds.min.x;
	}

	if ( facing.y >= 0 ) {
		yBias = -1;
		y0 = visBounds.max.y;
	}
	else {
		yBias = 1;
		y0 = visBounds.min.y;
	}

	for( int j=0; j<h; ++j ) {
		for( int i=0; i<w; ++i ) {

			Vector2I p = { x0 + xBias*i, y0 + yBias*j };

			if (    mapBounds.Contains( p )
				 && !visibilityProcessed.IsSet( p.x, p.y ) ) 
			{
				CalcVisibilityRay( unitID, p, pos, facing );
			}
		}
	}
	*/
}


void BattleScene::CalcVisibilityRay(	int unitID,
										const Vector2I& pos,
										const Vector2I& origin )
{
	/* Previous pass used a true ray casting approach, but this doesn't get good results. Numerical errors,
	   view stopped by leaves, rays going through cracks. Switching to a line walking approach to 
	   acheive stability and simplicity. (And probably performance.)
	*/


#ifdef DEBUG
	// Because of how the calling walk is done, the direction and sight will always be correct.
	// For arbitrary rotation this would need to be turned back on for release.
	{
/*		GLASSERT( facing.LengthSquared() > 0 );
		// Correct direction
		Vector2I vec = pos - origin;
		int dot = DotProduct( facing, vec );
		GLASSERT( dot >= 0 );
		if ( dot < 0 )
			return;
*/
		// Max sight
		const int MAX_SIGHT_SQUARED = MAX_EYESIGHT_RANGE*MAX_EYESIGHT_RANGE;
		Vector2I vec = pos - origin;
		int len2 = vec.LengthSquared();
		GLASSERT( len2 <= MAX_SIGHT_SQUARED );
		if ( len2 > MAX_SIGHT_SQUARED )
			return;
	}
#endif

	const Surface* lightMap = engine->GetMap()->GetLightMap(1);
	GLASSERT( lightMap->Format() == Surface::RGB16 );

	const float OBSCURED = 0.50f;
	const float DARK  = 0.16f;
	const float LIGHT = 0.08f;
	const int EPS = 10;

	float light = 1.0f;
	bool canSee = true;

	// Always walk the entire line so that the places we can not see are set
	// as well as the places we can.
	LineWalk line( origin.x, origin.y, pos.x, pos.y ); 
	while ( line.CurrentStep() <= line.NumSteps() )
	{
		Vector2I p = line.P();
		Vector2I q = line.Q();
		Vector2I delta = q-p;

		if ( canSee ) {
			canSee = engine->GetMap()->CanSee( p, q );

			if ( canSee ) {
				Surface::RGBA rgba;
				U16 c = lightMap->ImagePixel16( q.x, q.y );
				Surface::CalcRGB16( c, &rgba );

				const float distance = ( delta.LengthSquared() > 1 ) ? 1.4f : 1.0f;

				if ( engine->GetMap()->Obscured( q.x, q.y ) ) {
					light -= OBSCURED * distance;
				}
				else if (   rgba.r > (EL_NIGHT_RED_U8+EPS)
						 || rgba.g > (EL_NIGHT_GREEN_U8+EPS)
						 || rgba.b > (EL_NIGHT_BLUE_U8+EPS) ) 
				{
					light -= LIGHT * distance;
				}
				else {
					light -= DARK * distance;
				}
			}
		}
		visibilityProcessed.Set( q.x, q.y );
		if ( canSee ) {
			visibilityMap.Set( q.x, q.y, unitID, true );
		}

		// If all the light is used up, we will see no further.	
		if ( canSee && light < 0.0f )
			canSee = false;	

		line.Step(); 
	}
}


void BattleScene::Drag( int action, const grinliz::Vector2I& view )
{
	switch ( action ) 
	{
		case GAME_DRAG_START:
		{
			Ray ray;
			engine->CalcModelViewProjectionInverse( &dragMVPI );
			engine->RayFromScreenToYPlane( view.x, view.y, dragMVPI, &ray, &dragStart );
			dragStartCameraWC = engine->camera.PosWC();
		}
		break;

		case GAME_DRAG_MOVE:
		{
			Vector3F drag;
			Ray ray;
			engine->RayFromScreenToYPlane( view.x, view.y, dragMVPI, &ray, &drag );

			Vector3F delta = drag - dragStart;
			delta.y = 0.0f;

			engine->camera.SetPosWC( dragStartCameraWC - delta );
			engine->RestrictCamera();
		}
		break;

		case GAME_DRAG_END:
		{
		}
		break;

		default:
			GLASSERT( 0 );
			break;
	}
}


void BattleScene::Zoom( int action, int distance )
{
	switch ( action )
	{
		case GAME_ZOOM_START:
			initZoomDistance = distance;
			initZoom = engine->GetZoom();
//			GLOUTPUT(( "initZoomStart=%.2f distance=%d initDist=%d\n", initZoom, distance, initZoomDistance ));
			break;

		case GAME_ZOOM_MOVE:
			{
				//float z = initZoom * (float)distance / (float)initZoomDistance;	// original. wrong feel.
				float z = initZoom - (float)(distance-initZoomDistance)/800.0f;	// better, but slow out zoom-out, fast at zoom-in
				
//				GLOUTPUT(( "initZoom=%.2f distance=%d initDist=%d\n", initZoom, distance, initZoomDistance ));
				engine->SetZoom( z );
			}
			break;

		default:
			GLASSERT( 0 );
			break;
	}
	//GLOUTPUT(( "Zoom action=%d distance=%d initZoomDistance=%d lastZoomDistance=%d z=%.2f\n",
	//		   action, distance, initZoomDistance, lastZoomDistance, GetZoom() ));
}


void BattleScene::Rotate( int action, float degrees )
{
	if ( action == GAME_ROTATE_START ) {
		orbitPole.Set( 0.0f, 0.0f, 0.0f );

		const Vector3F* dir = engine->camera.EyeDir3();
		const Vector3F& pos = engine->camera.PosWC();

		IntersectRayPlane( pos, dir[0], 1, 0.0f, &orbitPole );	
		//orbitPole.Dump( "pole" );

		orbitStart = ToDegree( atan2f( pos.x-orbitPole.x, pos.z-orbitPole.z ) );
	}
	else {
		//GLOUTPUT(( "degrees=%.2f\n", degrees ));
		engine->camera.Orbit( orbitPole, orbitStart + degrees );
	}
}


void BattleScene::DrawHUD()
{
#ifdef MAPMAKER
	engine->GetMap()->DumpTile( (int)mapSelection->X(), (int)mapSelection->Z() );
	UFOText::Draw( 0,  16, "(%2d,%2d) 0x%2x:'%s'", 
				   (int)mapSelection->X(), (int)mapSelection->Z(),
				   currentMapItem, engine->GetMap()->GetItemDefName( currentMapItem ) );
#else
	bool enabled = SelectedSoldierUnit() && actionStack.Empty();
	{
		widgets->SetEnabled( BTN_TARGET, enabled );
		//widgets->SetEnabled( BTN_LEFT, enabled );
		//widgets->SetEnabled( BTN_RIGHT, enabled );
		widgets->SetEnabled( BTN_CHAR_SCREEN, enabled );
	}
	enabled = actionStack.Empty();
	{
		widgets->SetEnabled( BTN_TAKE_OFF, enabled );
		widgets->SetEnabled( BTN_END_TURN, enabled );
		widgets->SetEnabled( BTN_NEXT, enabled );
		//widgets->SetEnabled( BTN_NEXT_DONE, enabled );
	}
	widgets->SetHighLight( BTN_TARGET, uiMode == UIM_TARGET_TILE ? true : false );

	widgets->Draw();

	if ( HasTarget() ) {
		fireWidget->Draw();
	}

	if ( SelectedSoldierUnit() ) {
		int id = SelectedSoldierUnit() - units;
		const Unit* unit = SelectedSoldierUnit();
		UFOText::Draw( 55, 304, "%s %s %s", 
			unit->Rank(),
			unit->FirstName(),
			unit->LastName() );

		UFOText::Draw( 260, 304, 
			"TU:%.1f HP:%d Tgt:%02d/%02d", 
			unit->GetStats().TU(),
			unit->GetStats().HP(),
			m_targets.CalcTotalUnitCanSee( id, Unit::ALIEN ),
			m_targets.TotalTeamCanSee( Unit::SOLDIER, Unit::ALIEN ) );
	}
#endif
}


float BattleScene::Path::DeltaToRotation( int dx, int dy )
{
	float rot = 0.0f;
	GLASSERT( dx || dy );
	GLASSERT( dx >= -1 && dx <= 1 );
	GLASSERT( dy >= -1 && dy <= 1 );

	if ( dx == 1 ) 
		if ( dy == 1 )
			rot = 45.0f;
		else if ( dy == 0 )
			rot = 90.0f;
		else
			rot = 135.0f;
	else if ( dx == 0 )
		if ( dy == 1 )
			rot = 0.0f;
		else
			rot = 180.0f;
	else
		if ( dy == 1 )
			rot = 315.0f;
		else if ( dy == 0 )
			rot = 270.0f;
		else
			rot = 225.0f;
	return rot;
}

void BattleScene::Path::CalcDelta( int i0, int i1, grinliz::Vector2I* vec, float* rot )
{
#ifdef DEBUG
	int len = (int)statePath.size();
	GLASSERT( i0>=0 && i0<len-1 );
	GLASSERT( i1>=1 && i1<len );
#endif
	Vector2<S16> path0 = GetPathAt( i0 );
	Vector2<S16> path1 = GetPathAt( i1 );

	int dx = path1.x - path0.x;
	int dy = path1.y - path0.y;
	if ( vec ) {
		vec->x = dx;
		vec->y = dy;
	}
	if ( rot ) {
		*rot = DeltaToRotation( dx, dy );
	}
}


void BattleScene::Path::Travel(	float* travel,
								int* pos,
								float* fraction )
{
	// fraction is a bit funny. It is the lerp value between 2 path locations,
	// so it isn't a constant distance.

	GLASSERT( *pos < (int)statePath.size()-1 );

	Vector2I vec;
	CalcDelta( *pos, *pos+1, &vec, 0 );

	float distBetween = 1.0f;
	if ( vec.x && vec.y ) {
		distBetween = 1.41f;
	}
	float distRemain = (1.0f-*fraction) * distBetween;

	if ( *travel >= distRemain ) {
		*travel -= distRemain;
		(*pos)++;
		*fraction = 0.0f;
	}
	else {
		*fraction += *travel / distBetween;
		*travel = 0.0f;
	}
}


void BattleScene::Path::GetPos( int step, float fraction, float* x, float* z, float* rot )
{
	int len = (int)statePath.size();
	GLASSERT( step < len );
	GLASSERT( fraction >= 0.0f && fraction < 1.0f );
	if ( step == len-1 ) {
		step = len-2;
		fraction = 1.0f;
	}
	Vector2<S16> path0 = GetPathAt( step );
	Vector2<S16> path1 = GetPathAt( step+1 );

	int dx = path1.x - path0.x;
	int dy = path1.y - path0.y;
	*x = (float)path0.x + fraction*(float)( dx );
	*z = (float)path0.y + fraction*(float)( dy );
	*rot = DeltaToRotation( dx, dy );
}


#if !defined(MAPMAKER) && defined(_MSC_VER)
// TEST CODE
void BattleScene::MouseMove( int x, int y )
{
	/*
	grinliz::Matrix4 mvpi;
	grinliz::Ray ray;
	Vector3F intersection;

	engine->CalcModelViewProjectionInverse( &mvpi );
	engine->RayFromScreen( x, y, mvpi, &ray );
	Model* model = engine->IntersectModel( ray, TEST_TRI, Model::MODEL_SELECTABLE, 0, &intersection );

	if ( model ) {
		Color4F color = { 1.0f, 0.0f, 0.0f, 1.0f };
		Color4F colorVel = { 0.0f, 0.0f, 0.0f, 0.0f };
		Vector3F vel = { 0, -0.5, 0 };

		game->particleSystem->Emit(	ParticleSystem::POINT,
									0, 1, ParticleSystem::PARTICLE_RAY,
									color,
									colorVel,
									intersection,
									0.0f,			// posFuzz
									vel,
									0.0f,			// velFuzz
									2000 );


		vel.y = 0;
		intersection.y = 0;
		color.Set( 0, 0, 1, 1 );

		game->particleSystem->Emit(	ParticleSystem::POINT,
									0, 1, ParticleSystem::PARTICLE_RAY,
									color,
									colorVel,
									intersection,
									0.0f,			// posFuzz
									vel,
									0.0f,			// velFuzz
									2000 );
	}	
	*/
}
#endif



#ifdef MAPMAKER


void BattleScene::UpdatePreview()
{
	if ( preview ) {
		engine->FreeModel( preview );
		preview = 0;
	}
	if ( currentMapItem >= 0 ) {
		preview = engine->GetMap()->CreatePreview(	(int)mapSelection->X(), 
													(int)mapSelection->Z(), 
													currentMapItem, 
													(int)(mapSelection->GetYRotation()/90.0f) );

		if ( preview ) {
			const Texture* t = TextureManager::Instance()->GetTexture( "translucent" );
			preview->SetTexture( t );
			preview->SetTexXForm( 0, 0, TRANSLUCENT_WHITE, 0.0f );
		}
	}
}


void BattleScene::MouseMove( int x, int y )
{
	grinliz::Matrix4 mvpi;
	grinliz::Ray ray;
	grinliz::Vector3F p;

	engine->CalcModelViewProjectionInverse( &mvpi );
	engine->RayFromScreenToYPlane( x, y, mvpi, &ray, &p );

	int newX = (int)( p.x );
	int newZ = (int)( p.z );
	newX = Clamp( newX, 0, Map::SIZE-1 );
	newZ = Clamp( newZ, 0, Map::SIZE-1 );
	mapSelection->SetPos( (float)newX + 0.5f, 0.0f, (float)newZ + 0.5f );
	
	UpdatePreview();
}

void BattleScene::RotateSelection( int delta )
{
	float rot = mapSelection->GetYRotation() + 90.0f*(float)delta;
	mapSelection->SetYRotation( rot );
	UpdatePreview();
}

void BattleScene::DeleteAtSelection()
{
	const Vector3F& pos = mapSelection->Pos();
	engine->GetMap()->DeleteAt( (int)pos.x, (int)pos.z );
	UpdatePreview();
}


void BattleScene::DeltaCurrentMapItem( int d )
{
	currentMapItem += d;
	while ( currentMapItem < 0 ) { currentMapItem += Map::MAX_ITEM_DEF; }
	while ( currentMapItem >= Map::MAX_ITEM_DEF ) { currentMapItem -= Map::MAX_ITEM_DEF; }
	if ( currentMapItem == 0 ) currentMapItem = 1;
	UpdatePreview();
}

#endif


