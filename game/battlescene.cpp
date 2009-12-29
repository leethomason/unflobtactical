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
								ICON_GREEN_BUTTON,		// 3 next-done
								ICON_RED_BUTTON,		// 4 target
								ICON_BLUE_BUTTON,		// 5 left
								ICON_BLUE_BUTTON,		// 6 right
								ICON_GREEN_BUTTON		// 7 character
		};

		const char* iconText[] = {	"EXIT",
									"O",
									"N",
									"ND",
									"",
									"<-",
									"->",
									""	
								  };		

		widgets->InitButtons( icons, 8 );
		widgets->SetText( iconText );

		widgets->SetDeco( 7, DECO_CHARACTER );
		widgets->SetDeco( 4, DECO_AIMED );

		const Screenport& port = engine->GetScreenport();
		const Vector2I& pad = widgets->GetPadding();
		const Vector2I& size = widgets->GetButtonSize();
		int h = port.UIHeight();
		int w = port.UIWidth();

		widgets->SetPos( 0,		0, h-size.y );
		widgets->SetPos( 1,		0, h-(size.y*2+pad.y) );
		widgets->SetPos( 2,		0, size.y+pad.y );
		widgets->SetPos( 3,		0, 0 );
		widgets->SetPos( 4,		size.x+pad.x, 0 );
		widgets->SetPos( 5,		w-(size.x*2+pad.x), 0 );
		widgets->SetPos( 6,		w-size.x, 0 );
		widgets->SetPos( 7,		w-size.x, size.y+pad.y );
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
	fireWidget->SetDeco( 5, DECO_AIMED );
	fireWidget->SetDeco( 6, DECO_ROCKET );
	fireWidget->SetDeco( 7, DECO_SHELLS );

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

	CalcAllVisibility();
	SetFogOfWar();
	CalcTeamTargets();
	targetEvents.Clear();
	NewTurn( Unit::SOLDIER );
}


BattleScene::~BattleScene()
{
	UFOStream* stream = game->OpenStream( "BattleScene" );
	Save( stream );
	game->particleSystem->Clear();
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
		 clip( game, "Clip", -1 );

	gun0.Insert( clip );
	gun1.Insert( cell );
	plasmaRifle.Insert( cell );
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
		{ 16, 21 }, {12, 18 }, { 30, 25 }, { 29, 30 }
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
	switch ( team ) {
		case Unit::SOLDIER:
			for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i )
				units[i].NewTurn();
			break;

		case Unit::ALIEN:
			for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i )
				units[i].NewTurn();
			break;

		case Unit::CIVILIAN:
			for( int i=CIV_UNITS_START; i<CIV_UNITS_END; ++i )
				units[i].NewTurn();
			break;

		default:
			GLASSERT( 0 );
			break;
	}
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

	if (    SelectedSoldier()
		 && SelectedSoldierUnit()->Status() == Unit::STATUS_ALIVE
		 && SelectedSoldierUnit()->Team() == Unit::SOLDIER ) 
	{
		Model* m = SelectedSoldierModel();
		GLASSERT( m );

		float alpha = 0.5f;
		game->particleSystem->EmitDecal( ParticleSystem::DECAL_SELECTION, 
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
			if ( targets.terran.alienTargets.IsSet( unitID-TERRAN_UNITS_START, i-ALIEN_UNITS_START ) ) {
				Vector3F p;
				units[i].CalcPos( &p );
				game->particleSystem->EmitDecal( ParticleSystem::DECAL_TARGET,
												 ParticleSystem::DECAL_BOTH,
												 p, ALPHA, 0 );	
			}
		}
	}

	ProcessAction( deltaTime );


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
		game->particleSystem->EmitDecal( ParticleSystem::DECAL_TARGET,
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


void BattleScene::RotateAction( Unit* src, const Vector3F& dst3F, bool quantize )
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


void BattleScene::ShootAction( Unit* src, const grinliz::Vector3F& dst, int select )
{
	GLASSERT( select == 0 || select == 1 );

	Action action;
	action.Init( ACTION_SHOOT, src );
	action.type.shoot.target = dst;
	action.type.shoot.select = select;
	actionStack.Push( action );
}


void BattleScene::ProcessAction( U32 deltaTime )
{
	if ( !actionStack.Empty() )
	{
		Action* action = &actionStack.Top();

		Unit* unit = 0;
		Model* model = 0;
		if ( action->unit ) {
			if ( !action->unit->IsAlive() || !action->unit->GetModel() ) {
				GLASSERT( 0 );	// may be okay, but untested.
				actionStack.Pop();
				return;
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
					
					//Vector2I originalPos;
					//float originalRot;
					//unit->CalcMapPos( &originalPos, &originalRot );

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
								break;
							}
						}
						path.GetPos( action->type.move.pathStep, action->type.move.pathFraction, &x, &z, &r );

						Vector3F v = { x+0.5f, 0.0f, z+0.5f };
						unit->SetPos( v, model->GetYRotation() );

						if ( action->type.move.pathStep == path.statePath.size()-1 ) {
							actionStack.Pop();
							path.Clear();
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
					}
					else {
						unit->SetYRotation( model->GetYRotation() + bias*travel );
					}
				}
				break;

			case ACTION_SHOOT:
				ProcessActionShoot( action, unit, model );
				break;

			case ACTION_HIT:
				ProcessActionHit( action );
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
				CalcVisibility( unit, newPos.x, newPos.y, newRot );
				SetFogOfWar();
				CalcTeamTargets();
				DumpTargetEvents();
				targetEvents.Clear();
			}

			// If actions are empty and this is the selected unit, update
			// the path feedback.
			if ( actionStack.Empty() && unit == SelectedSoldierUnit() ) {
				ShowNearPath( unit );
			}
		}
	}
}


void BattleScene::ProcessActionShoot( Action* action, Unit* unit, Model* model )
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

		if ( intersection.y < 0.0f ) {
			// hit ground first.
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
									 game->particleSystem,
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
}


void BattleScene::ProcessActionHit( Action* action )
{
	bool changed = false;

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
					targetEvents.Clear();	// don't need to handle notification of obvious (alien shot)
					changed = true;
				}
				GLOUTPUT(( "Hit Unit 0x%x hp=%d/%d\n", (unsigned)hitUnit, (int)hitUnit->GetStats().HP(), (int)hitUnit->GetStats().TotalHP() ));
			}
		}
		else if ( m && m->IsFlagSet( Model::MODEL_OWNED_BY_MAP ) ) {
			// Hit world object.
			bool destroyed = engine->GetMap()->DoDamage( m, action->type.hit.damageDesc );
			if ( destroyed )
				changed = true;
		}
	}
	else {
		// Explosion
		const int MAX_RAD = 3;
		// There is a small offset to move the explosion back towards the shooter.
		// If it hits a wall (common) this will move it to the previous square.
		// Also means a model hit may be a "near miss"...but explosions are messy.
		const int x0 = (int)(action->type.hit.p.x - 0.2f*action->type.hit.n.x);
		const int y0 = (int)(action->type.hit.p.z - 0.2f*action->type.hit.n.z);

		for( int rad=0; rad<MAX_RAD; ++rad ) {
			DamageDesc dd = action->type.hit.damageDesc;
			dd.Scale( (float)(MAX_RAD-rad) / (float)(MAX_RAD) );

			for( int y=y0-rad; y<=y0+rad; ++y ) {
				for( int x=x0-rad; x<=x0+rad; ++x ) {
					if ( x==(x0-rad) || x==(x0+rad) || y==(y0-rad) || y==(y0+rad) ) {
						// can the tile to be damaged be reached by the explosion?
						// visibility is checked up to the tile before this one, else
						// it is possible to miss because "you can't see yourself"
						bool canSee = true;
						if ( rad > 0 ) {
							LineWalk walk( x0, y0, x, y );
							for( ; walk.CurrentStep() < (walk.NumSteps()-1); walk.Step() ) {
								Vector2I p = walk.P();
								Vector2I q = walk.Q();

								if ( !engine->GetMap()->CanSee( p, q ) ) {
									canSee = false;
									break;
								}
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
								targetEvents.Clear();	// don't need to handle notification of obvious (alien shot)
								changed = true;
							}
						}
						if ( engine->GetMap()->DoDamage( x, y, dd ) ) {
							changed = true;
						}
					}
				}
			}
		}
	}
	if ( changed ) {
		CalcAllVisibility();
		SetFogOfWar();
		CalcTeamTargets();
	}
	actionStack.Pop();
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

			//float tu = selection.soldierUnit->FireTimeUnits( select, type );
			//GLASSERT( tu > 0 );
			//float autoTU = tu * 0.33333f;

			Vector3F target;
			if ( selection.targetPos.x >= 0 ) {
				target.Set( (float)selection.targetPos.x + 0.5f, 1.0f, (float)selection.targetPos.y + 0.5f );
			}
			else {
				selection.targetUnit->GetModel()->CalcTarget( &target );
			}

			Item* weapon = selection.soldierUnit->GetWeapon();
			GLASSERT( weapon );	// else how did we get the fire menu??

			// Stack - push in reverse order.

			int nShots = ( type == AUTO_SHOT ) ? 3 : 1;
			if (    selection.soldierUnit->GetStats().TU() >= selection.soldierUnit->FireTimeUnits( select, type )
				 && (weapon->RoundsRequired(select+1)*nShots <= weapon->RoundsAvailable(select+1)) ) 
			{
				selection.soldierUnit->UseTU( selection.soldierUnit->FireTimeUnits( select, type ) );

				for( int i=0; i<nShots; ++i ) {
					ShootAction( selection.soldierUnit, target, select );
					weapon->UseRound( select+1 );
				}
			}
			RotateAction( selection.soldierUnit, target, true );
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
			/*
			case 1:
				if ( engine->GetDayTime() )
					engine->SetDayNight( false, game->GetLightMap( "farmlandN" ) );
				else
					engine->SetDayNight( true, 0 );

				CalcAllVisibility();
				SetFogOfWar();
				CalcTeamTargets();
				targetEvents.Clear();
				break;
			*/

			case BTN_TARGET:
				{
					uiMode = UIM_TARGET_TILE;
				}
				break;

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
				NewTurn( Unit::SOLDIER );	// FIXME: should go alien or civ
				break;

			case BTN_NEXT:
			case BTN_NEXT_DONE:
				{
					int index = TERRAN_UNITS_END-1;
					if ( SelectedSoldierUnit() ) {
						index = SelectedSoldierUnit() - units;
						if ( icon == BTN_NEXT_DONE )
							units[index].SetUserDone();
					}
					int i = index+1;
					if ( i == TERRAN_UNITS_END )
						i = TERRAN_UNITS_START;
					while( i != index ) {
						if ( units[i].IsAlive() && !units[i].IsUserDone() ) {
							SetSelection( &units[i] );
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

#ifdef MAPMAKER
	if ( !iconSelected ) {
		const Vector3F& pos = mapSelection->Pos();
		int rotation = (int) (mapSelection->GetYRotation() / 90.0f );

		engine->GetMap()->AddItem( (int)pos.x, (int)pos.z, rotation, currentMapItem, -1, 0 );
		iconSelected = 0;	// don't keep processing
	}
#endif	

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
				selection.soldierUnit->UseTU( cost );

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

		float d=0;
		if ( item->HasPart( select+1 ) ) {
			fireWidget->SetDeco( 6+select, item->Deco(select+1) );
			
			DamageDesc dd;
			wid->DamageBase( select, &dd );

			SNPRINTF( buffer0, 16, "D%d", (int)dd.Total() );
			SNPRINTF( buffer1, 16, "R%d", item->RoundsAvailable(select+1) );
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
			enable = enable &&	(item->RoundsRequired(select+1)*nShots <= item->RoundsAvailable(select+1));
			enable = enable &&  unit->GetStats().TU() >= unit->FireTimeUnits( select, type );

			if ( enable ) {
				unit->FireStatistics( select, type, distToTarget, &fraction, &tu, &dptu );
				SNPRINTF( buffer0, 16, "%d%%", (int)LRintf( fraction*100.0f ) );

				if ( tu >= 10.0f && dptu >= 10.0f )
					SNPRINTF( buffer1, 16, "%2d %2d", LRintf(tu), LRintf(dptu) );
				else if ( tu < 10.0f && dptu >= 10.0f )
					SNPRINTF( buffer1, 16, "%.1f %2d", tu, LRintf(dptu) );
				else if ( tu >= 10.0f && dptu < 10.0f )
					SNPRINTF( buffer1, 16, "%2d %.1f", LRintf(tu), dptu );
				else if ( tu < 10.0f && dptu < 10.0f )
					SNPRINTF( buffer1, 16, "%.1f %.1f", tu, dptu );

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

	float lowTU = unit->FireTimeUnits( 0, SNAP_SHOT );
	float hiTU = Max( unit->FireTimeUnits( 0, AIMED_SHOT ), unit->FireTimeUnits( 0, AUTO_SHOT ) );

	engine->GetMap()->ShowNearPath( start, stats.TU()-hiTU, stats.TU()-lowTU, stats.TU() );
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


int BattleScene::Targets::AlienTargets( int id )
{
	GLASSERT( id >= TERRAN_UNITS_START && id < TERRAN_UNITS_END );
	Rectangle3I r;
	r.Set(	id-TERRAN_UNITS_START, 0, 0,
			id-TERRAN_UNITS_START, MAX_ALIENS-1, 0 );

	int count = terran.alienTargets.NumSet( r );
	return count;
}


int BattleScene::Targets::TotalAlienTargets()
{
	Rectangle3I r;
	r.Set(	0, 0, 0, MAX_ALIENS-1, 0, 0 );

	int count = terran.teamAlienTargets.NumSet( r );
	return count;
}

void BattleScene::DumpTargetEvents()
{
	for( int i=0; i<targetEvents.Size(); ++i ) {
		const TargetEvent& e = targetEvents[i];
		if ( !e.team ) {
			GLOUTPUT(( "Terran Unit %d %s alien %d\n",
					   e.viewerID, 
					   e.gain ? "gain" : "loss", 
					   e.targetID ));
		}
		else {
			GLOUTPUT(( "Terran Team %s alien %d\n",
					   e.gain ? "gain" : "loss", 
					   e.targetID ));
		}
	}
}

void BattleScene::CalcTeamTargets()
{
	// generate events.
	// - if team gets/loses target
	// - if unit gets/loses target

	Targets old = targets;
	targets.Clear();

	// Terran to Alien
	// Go through each alien, and check #terrans that can target it.
	//
	for( int j=ALIEN_UNITS_START; j<ALIEN_UNITS_END; ++j ) {
		// Push IsAlive() checks into the cases below, so that a unit
		// death creates a visibility change. However, initialization 
		// happens at the beginning, so we can skip that.

		if ( !units[j].InUse() )
			continue;

		Vector2I mapPos;
		units[j].CalcMapPos( &mapPos, 0 );
		
		Rectangle3I r;
		r.Set( mapPos.x, mapPos.y, TERRAN_UNITS_START,
			   mapPos.x, mapPos.y, TERRAN_UNITS_END-1 );

		for( int k=TERRAN_UNITS_START; k<TERRAN_UNITS_END; ++k ) {
			// Main test: can a terran see an alien at this location?
			if ( units[k].IsAlive() && units[j].IsAlive() && visibilityMap.IsSet( mapPos.x, mapPos.y, k ) ) {
				targets.terran.alienTargets.Set( k-TERRAN_UNITS_START, j-ALIEN_UNITS_START );
				targets.terran.teamAlienTargets.Set( j-ALIEN_UNITS_START, 0, 0 );
			}

			// check unit change.
			if (	old.terran.alienTargets.IsSet( k-TERRAN_UNITS_START, j-ALIEN_UNITS_START )
				 && !targets.terran.alienTargets.IsSet( k-TERRAN_UNITS_START, j-ALIEN_UNITS_START ) )
			{	
				// Lost unit.
				TargetEvent e = { 0, 0, k, j };
				targetEvents.Push( e );
			}
			else if (	 !old.terran.alienTargets.IsSet( k-TERRAN_UNITS_START, j-ALIEN_UNITS_START )
					  && targets.terran.alienTargets.IsSet( k-TERRAN_UNITS_START, j-ALIEN_UNITS_START ) )
			{
				// Gain unit.
				TargetEvent e = { 0, 1, k, j };
				targetEvents.Push( e );
			}
		}
		// Check team change.
		if (	old.terran.teamAlienTargets.IsSet( j-ALIEN_UNITS_START )
			 && !targets.terran.teamAlienTargets.IsSet( j-ALIEN_UNITS_START ) )
		{	
			TargetEvent e = { 1, 0, 0, j };
			targetEvents.Push( e );
		}
		else if (    !old.terran.teamAlienTargets.IsSet( j-ALIEN_UNITS_START )
				  && targets.terran.teamAlienTargets.IsSet( j-ALIEN_UNITS_START ) )
		{	
			TargetEvent e = { 1, 1, 0, j };
			targetEvents.Push( e );
		}
	}
}


void BattleScene::CalcAllVisibility()
{
	visibilityMap.ClearAll();
	for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i ) {
		if ( units[i].IsAlive() ) {
			Vector2I pos;
			float rotation;
			units[i].CalcMapPos( &pos, &rotation );
			CalcVisibility( &units[i], pos.x, pos.y, rotation );
		}
	}
	for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) {
		if ( units[i].IsAlive() ) {
			Vector2I pos;
			float rotation;
			units[i].CalcMapPos( &pos, &rotation );
			CalcVisibility( &units[i], pos.x, pos.y, rotation );
		}
	}
}


void BattleScene::CalcVisibility( const Unit* unit, int x, int y, float rotation )
{
	int unitID = unit - units;
	//if ( unitID > 0 )
	//	return;
	GLASSERT( unitID >= 0 && unitID < MAX_UNITS );

	// Clear out the old settings.
	// Walk the area in range around the unit and cast rays.
	visibilityMap.ClearPlane( unitID );

	Rectangle2I r, b;
	r.Set( 0, 0, MAP_SIZE-1, MAP_SIZE-1 );
	b.Set( x-MAX_EYESIGHT_RANGE, y-MAX_EYESIGHT_RANGE, x+MAX_EYESIGHT_RANGE, y+MAX_EYESIGHT_RANGE );
	b.DoIntersection( r );

	Vector2I facing;
	facing.x = (int)(10.0f*sinf(ToRadian(rotation)));
	facing.y = (int)(10.0f*cosf(ToRadian(rotation)));
	
	/* Previous pass used a true ray casting approach, but this doesn't get good results. Numerical errors,
	   view stopped by leaves, rays going through cracks. Switching to a line walking approach to 
	   acheive stability and simplicity. (And probably performance.)
	*/

	const int MAX_SIGHT_SQUARED = MAX_EYESIGHT_RANGE*MAX_EYESIGHT_RANGE;

	for( int j=b.min.y; j<=b.max.y; ++j ) {
		for( int i=b.min.x; i<=b.max.x; ++i ) {

			// Early out the simple cases:
			if ( i==x && j==y ) {
				// Can always see yourself.
				visibilityMap.Set( i, j, unitID );
				continue;
			}

			// Correct direction
			int dx = i-x;
			int dy = j-y;
			Vector2I vec = { dx, dy };
			int dot = DotProduct( facing, vec );
			if ( dot < 0 )
				continue;

			// Max sight
			int len2 = dx*dx + dy*dy;
			if ( (float)len2 > ((float)MAX_SIGHT_SQUARED * (0.3f+0.7f*dot) ) )
				continue;

			int axis=0;
			int axisDir = 1;
			int steps = abs(dx);
			Fixed delta( 0 );

			if ( abs( dy ) > abs(dx) ) {
				// y is major axis. delta = dx per distance y
				axis = 1;
				steps = abs(dy);
				if ( dy < 0 )
					axisDir = -1;
				delta = Fixed( dx ) / Fixed( abs(dy) );
				GLASSERT( delta < 1 && delta > -1 );
			}
			else {
				// x is the major aris. delta = dy per distance x
				if ( dx < 0 )
					axisDir = -1;
				delta = Fixed( dy ) / Fixed( abs(dx) );
				GLASSERT( delta <= 1 && delta >= -1 );
			}

			/* Line walking algorithm. Step 1 unit on the major axis,
			   and occasionally on the minor axis.
			*/
			float light = 1.0f;
			const Surface* lightMap = engine->GetMap()->GetLightMap(1);
			GLASSERT( lightMap->Format() == Surface::RGB16 );
			//const U16* pixel = (const U16*) lightMap->Pixels();

			Vector2<Fixed> p = { Fixed(x)+Fixed(0.5f), Fixed(y)+Fixed(0.5f) };

			int k=0;
			for( ; k<steps; ++k ) {
				Vector2<Fixed> q = p;
				q.X(axis) += axisDir;
				q.X(!axis) += delta;

				Vector2I p0 = { (int)p.x, (int)p.y };
				Vector2I q0 = { (int)q.x, (int)q.y };
				bool canSee = engine->GetMap()->CanSee( p0, q0 );
				if ( !canSee ) {
					break;
				}

				// Put light at the bottom: if we run out, we can't see the NEXT thing.
				if ( !engine->GetDayTime() ) {

					float dist=1.0f;
					if ( abs( p0.x-q0.x ) + abs( p0.y-q0.y ) == 2 ) {
						dist = 1.4f;
					}

					Surface::RGBA rgba;
					U16 c = lightMap->ImagePixel16( q0.x, q0.y );
					Surface::CalcRGB16( c, &rgba );

					const float DARK  = 0.16f;
					const float LIGHT = 0.08f;
					const int EPS = 10;

					if (    rgba.r > (EL_NIGHT_RED_U8+EPS)
						 || rgba.g > (EL_NIGHT_GREEN_U8+EPS)
						 || rgba.b > (EL_NIGHT_BLUE_U8+EPS) ) 
					{
						light -= LIGHT * dist;
					}
					else {
						light -= DARK * dist;
					}

					if ( light <= 0.0f )
						break;
				}
				p = q;
			}
			if ( k == steps )
				visibilityMap.Set( i, j, unitID );
		}
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
	bool enabled = SelectedSoldierUnit() && actionStack.Empty();
	{
		widgets->SetEnabled( BTN_TARGET, enabled );
		widgets->SetEnabled( BTN_LEFT, enabled );
		widgets->SetEnabled( BTN_RIGHT, enabled );
		widgets->SetEnabled( BTN_CHAR_SCREEN, enabled );
	}
	enabled = actionStack.Empty();
	{
		widgets->SetEnabled( BTN_TAKE_OFF, enabled );
		widgets->SetEnabled( BTN_END_TURN, enabled );
		widgets->SetEnabled( BTN_NEXT, enabled );
		widgets->SetEnabled( BTN_NEXT_DONE, enabled );
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
			targets.AlienTargets( id ), targets.TotalAlienTargets() );
	}

#ifdef MAPMAKER
	engine->GetMap()->DumpTile( (int)mapSelection->X(), (int)mapSelection->Z() );
	UFOText::Draw( 0,  16, "(%2d,%2d) 0x%2x:'%s'", 
				   (int)mapSelection->X(), (int)mapSelection->Z(),
				   currentMapItem, engine->GetMap()->GetItemDefName( currentMapItem ) );
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


