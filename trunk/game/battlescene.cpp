#include "battlescene.h"
#include "game.h"
#include "cgame.h"

#include "../engine/uirendering.h"
#include "../engine/platformgl.h"
#include "../engine/particle.h"
#include "../engine/text.h"
 


using namespace grinliz;


BattleScene::BattleScene( Game* game ) : Scene( game )
{
#ifdef MAPMAKER
	currentMapItem = 1;
#endif
	engine  = &game->engine;

	path.Clear();

	// On screen menu.
	Texture* t = game->GetTexture( "icons" );	
	widgets = new UIButtonBox( t, engine->GetScreenport() );
	
	int x, y, w, h;

	const int icons[] = { UIButtonBox::ICON_PLAIN, UIButtonBox::ICON_PLAIN, UIButtonBox::ICON_CHARACTER, UIButtonBox::ICON_PLAIN };
	const char* iconText[] = { "home", "d/n", 0, "fow" };
	widgets->SetButtons( icons, 4 );
	widgets->SetText( iconText );
	widgets->CalcDimensions( &x, &y, &w, &h );
	widgets->SetOrigin( 0, engine->GetScreenport().UIHeight()-h );

	// When enemy targeted.
	fireWidget = new UIButtonBox( t, engine->GetScreenport() );
	const int fireIcons[] = { UIButtonBox::ICON_AUTO, UIButtonBox::ICON_SNAP, UIButtonBox::ICON_AIM };
	fireWidget->SetButtons( fireIcons, 3 );

	engine->EnableMap( true );

#ifdef MAPMAKER
	resource = game->GetResource( "selection" );
	selection = engine->AllocModel( resource );
	selection->SetPos( 0.5f, 0.0f, 0.5f );
	preview = 0;
#endif

	// Do we have a saved state?
	UFOStream* stream = game->OpenStream( "BattleScene", false );
	if ( !stream ) {
		InitUnits();
	}
	else {
		Load( stream );
	}
}


BattleScene::~BattleScene()
{
	UFOStream* stream = game->OpenStream( "BattleScene" );
	Save( stream );
	game->particleSystem->Clear();

	delete fireWidget;
	delete widgets;
}


void BattleScene::InitUnits()
{
	int Z = engine->GetMap()->Height();
	Random random(5);

	Item gun0, gun1, clip;

	gun0.Init( game->GetItemDef( "Pst-0" ) );
	gun1.Init( game->GetItemDef( "Ray-0" ) );
	clip.Init( game->GetItemDef( "Clip" ) );

	for( int i=0; i<6; ++i ) {
		Vector2I pos = { (i*2)+10, Z-10 };
		Unit* unit = &units[TERRAN_UNITS_START+i];

		unit->Init( engine, game, Unit::SOLDIER, 0, random.Rand() );
		Inventory* inventory = unit->GetInventory();
		inventory->AddItem( Inventory::WEAPON_SLOT, gun0 );
		inventory->AddItem( Inventory::ANY_SLOT, clip );
		inventory->AddItem( Inventory::ANY_SLOT, clip );
		unit->UpdateInventory();
		unit->SetMapPos( pos.x, pos.y );
	}
	
	for( int i=0; i<4; ++i ) {
		Vector2I pos = { (i*2)+10, Z-8 };
		Unit* unit = &units[ALIEN_UNITS_START+i];

		unit->Init( engine, game, Unit::ALIEN, i&3, random.Rand() );
		Inventory* inventory = unit->GetInventory();
		inventory->AddItem( Inventory::WEAPON_SLOT, gun1 );
		unit->UpdateInventory();
		unit->SetMapPos( pos.x, pos.y );
	}

	for( int i=0; i<4; ++i ) {
		Vector2I pos = { (i*2)+10, Z-12 };
		units[CIV_UNITS_START+i].Init( engine, game, Unit::CIVILIAN, 0, random.Rand() );
		units[CIV_UNITS_START+i].SetMapPos( pos.x, pos.y );
	}
	SetUnitsDraggable();
}


void BattleScene::SetUnitsDraggable()
{
	for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i ) {
		Model* m = units[i].GetModel();
		if ( m && units[i].Status() == Unit::STATUS_ALIVE ) {
			m->SetFlag( Model::MODEL_SELECTABLE );
		}
	}
}


void BattleScene::Save( UFOStream* s )
{
	// Selection
	// Units[]

	s->WriteU32( UFOStream::MAGIC0 );

	U32 selectionIndex = MAX_UNITS;
	if ( selection.soldierUnit ) {
		selectionIndex = selection.soldierUnit - units;
	}
	s->WriteU32( selectionIndex );

	for( int i=0; i<MAX_UNITS; ++i ) {
		units[i].Save( s );
	}

	s->WriteU32( UFOStream::MAGIC1 );
}


void BattleScene::Load( UFOStream* s )
{
	U32 magic = s->ReadU32();
	GLASSERT( magic == UFOStream::MAGIC0 );
	
	U32 selectedUnit = s->ReadU32();
	selection.Clear();

	if ( selectedUnit < MAX_UNITS ) {
		selection.soldierUnit = &units[selectedUnit];
	}

	for( int i=0; i<MAX_UNITS; ++i ) {
		units[i].Load( s, engine, game );
	}
	magic = s->ReadU32();
	GLASSERT( magic == UFOStream::MAGIC1 );
	SetUnitsDraggable();
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

	if ( currentTime/1000 != (currentTime-deltaTime)/1000 ) {
		grinliz::Vector3F pos = { 10.0f, 1.0f, 28.0f };
		grinliz::Vector3F vel = { 0.0f, 1.0f, 0.0f };
		Color4F col = { 1.0f, -0.5f, 0.0f, 1.0f };
		Color4F colVel = { 0.0f, 0.0f, 0.0f, 0.0f };

		game->particleSystem->EmitPoint(	40,		// count
											ParticleSystem::PARTICLE_SPHERE,
											col,	colVel,
											pos,	0.1f,	
											vel,	0.1f,
											1200 );
	}
	grinliz::Vector3F pos = { 13.f, 0.0f, 28.0f };
	game->particleSystem->EmitFlame( deltaTime, pos );

	if (    SelectedSoldier()
		 && SelectedSoldierUnit()->Status() == Unit::STATUS_ALIVE
		 && SelectedSoldierUnit()->Team() == Unit::SOLDIER ) 
	{
		Model* m = SelectedSoldierModel();
		if ( m ) {
			//const U32 INTERVAL = 500;
			float alpha = 0.5f;	//0.5f + 0.4f*(float)(currentTime%500)/(float)(INTERVAL);
			game->particleSystem->EmitDecal( ParticleSystem::DECAL_SELECTION, 
											 ParticleSystem::DECAL_BOTTOM,
											 m->Pos(), alpha,
											 m->GetYRotation() );
		}
	}
	if ( actionStack.Empty() ) {
		// Show the path.

		if ( path.len > 0 ) {
			//float rot = 0.0f;
			for( int i=0; i<path.len-1; ++i ) {
				Vector3F pos = { (float)(path.path[i].x)+0.5f, 0.0f, (float)(path.path[i].y)+0.5f };
				float rot;
				path.CalcDelta( i, i+1, 0, &rot );

				const float alpha = 0.2f;
				game->particleSystem->EmitDecal( ParticleSystem::DECAL_PATH, 
												 ParticleSystem::DECAL_BOTH,
												 pos, alpha,
												 rot );
			}
		}
	}
	ProcessAction( deltaTime );

	if ( AlienTargeted() ) {
		Vector3F pos = { 0, 0, 0 };
		AlienUnit()->CalcPos( &pos );

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


void BattleScene::FreePathEndModel()
{
	if ( selection.pathEndModel ) {
		engine->FreeModel( selection.pathEndModel );
		selection.pathEndModel = 0;
	}
}


void BattleScene::SetSelection( Unit* unit ) 
{
	FreePathEndModel();

	GLASSERT( unit->IsAlive() );

	if ( unit->Team() == Unit::SOLDIER ) {
		selection.soldierUnit = unit;
		selection.targetUnit = 0;
	}
	else if ( unit->Team() == Unit::ALIEN ) {
		GLASSERT( SelectedSoldier() );
		selection.targetUnit = unit;
	}
	else {
		GLASSERT( 0 );
	}
}


void BattleScene::RotateAction( Unit* src, const Unit* dst, bool quantize )
{
	GLASSERT( src->GetModel() );
	GLASSERT( dst->GetModel() );

	float rot = src->AngleBetween( dst, quantize );
	if ( src->GetModel()->GetYRotation() != rot ) {
		Action action;
		action.Rotate( src, rot );
		actionStack.Push( action );
	}
}


void BattleScene::ShootAction( Unit* src, const grinliz::Vector3F& dst )
{
	Action action;
	action.Shoot( src, dst );
	actionStack.Push( action );
}


void BattleScene::ProcessAction( U32 deltaTime )
{
	if ( !actionStack.Empty() )
	{
		Action* action = &actionStack.Top();

		Unit* unit = 0;
		Model* model = 0;
		if ( action->action != ACTION_DELAY ) {
			if ( !action->unit || !action->unit->IsAlive() || !action->unit->GetModel() ) 
				return;

			unit = action->unit;
			model = action->unit->GetModel();
		}

		switch ( action->action ) {
			case ACTION_MOVE: 
				{
					// Move the unit.

					const float SPEED = 3.0f;
					float travel = Travel( deltaTime, SPEED );

					while( action->move.pathStep < path.len-1 && travel > 0.0f ) {
						path.Travel( &travel, &action->move.pathStep, &action->move.pathFraction );
					}
					float x, z, r;
					path.GetPos( action->move.pathStep, action->move.pathFraction, &x, &z, &r );

					Vector3F v = { x+0.5f, 0.0f, z+0.5f };
					unit->SetPos( v, r );

					if ( action->move.pathStep == path.len-1 ) {
						actionStack.Pop();
						path.Clear();
						FreePathEndModel();
					}
				}
				break;

			case ACTION_ROTATE:
				{
					const float ROTSPEED = 400.0f;
					float travel = Travel( deltaTime, ROTSPEED );

					float delta, bias;
					MinDeltaDegrees( model->GetYRotation(), action->rotation, &delta, &bias );

					if ( delta <= travel ) {
						unit->SetYRotation( action->rotation );
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

			case ACTION_DELAY:
				{
					if ( deltaTime >= action->delay ) {
						actionStack.Pop();
					}
					else {
						action->delay -= deltaTime;
					}
				}
				break;

			default:
				GLASSERT( 0 );
				break;
		}
	}
}


void BattleScene::ProcessActionShoot( Action* action, Unit* unit, Model* model )
{
	if ( unit && model && unit->IsAlive() ) {
		Vector3F p0, p1;

		Vector3F beam0 = { 0, 0, 0 }, beam1 = { 0, 0, 0 };
		
		Vector4F	beamColor	= { 1, 1, 1, 1 };
		float		beamDecay	= -3.0f;
		Vector4F	impactColor	= { 1, 1, 1, 1 };
		float		beamWidth = 0.01f;
		bool hitSomething = false;

		const Item* weaponItem = unit->GetWeapon();
		const ItemDef* weaponDef = weaponItem->itemDef;
		weaponDef->QueryWeaponRender( &beamColor, &beamDecay, &beamWidth, &impactColor );

		model->CalcTrigger( &p0 );
		p1 = action->target;

		Ray ray;
		ray.origin = p0;
		ray.direction = p1-p0;

		// What can we hit?
		// model
		//		unit, alive (does damage)
		//		unit, dead (does nothing)
		//		model, world (does damage)
		//		gun, does nothing
		// ground / bounds

		Vector3F hit;
		// Don't hit the shooter:
		if ( unit->GetModel() )
			unit->GetModel()->SetFlag( Model::MODEL_HIDDEN_FROM_TREE );
		if ( unit->GetWeaponModel() )
			unit->GetWeaponModel()->SetFlag( Model::MODEL_HIDDEN_FROM_TREE );

		Model* m = engine->IntersectModel( ray, TEST_TRI, 0, 0, &hit );

		if ( unit->GetModel() )
			unit->GetModel()->ClearFlag( Model::MODEL_HIDDEN_FROM_TREE );
		if ( unit->GetWeaponModel() )
			unit->GetWeaponModel()->ClearFlag( Model::MODEL_HIDDEN_FROM_TREE );

		if ( hit.y < 0.0f ) {
			// hit ground first.
			m = 0;
		}

		if ( m ) {
			hitSomething = true;
			beam0 = p0;
			beam1 = hit;

			Unit* hitUnit = UnitFromModel( m );
			if ( hitUnit ) {
				if ( hitUnit->IsAlive() ) {
					hitUnit->DoDamage( weaponDef->damageBase, weaponDef->material );
					GLOUTPUT(( "Hit Unit 0x%x hp=%d/%d\n", hitUnit, hitUnit->GetStats().HP(), hitUnit->GetStats().TotalHP() ));
				}
			}
			else if ( m && m->IsFlagSet( Model::MODEL_OWNED_BY_MAP ) ) {
				// Hit world object.
				engine->GetMap()->DoDamage( weaponDef->damageBase, m, weaponDef->material );
			}
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
					hitSomething = true;
				}
			}
		}

		if ( beam0 != beam1 ) {
			Vector4F colorVel = { 0, 0, 0, beamDecay };
			game->particleSystem->EmitBeam( beamColor, colorVel, beam0, beam1, beamWidth, 1000 );
		}
		if ( hitSomething ) {
			Vector4F colorVel = { 0, 0, 0, -1.0f };
			Vector3F velocity = beam1 - beam0;
			velocity.Normalize();

			game->particleSystem->EmitPoint( 20, ParticleSystem::PARTICLE_HEMISPHERE, 
											 impactColor, colorVel, beam1, 0.1f, velocity, 0.4f, 1000 );
		}
	}
	actionStack.Pop();
}


bool BattleScene::HandleIconTap( int vX, int vY )
{
	int screenX, screenY;
	engine->GetScreenport().ViewToUI( vX, vY, &screenX, &screenY );

	int icon = -1;

	if ( AlienTargeted() ) {
		icon = fireWidget->QueryTap( screenX, screenY );
		switch ( icon ) {
			case 0:	//aut0
			case 1: //snap
			case 2: //aim
				// shooting creates a turn action then a shoot action.
				GLASSERT( selection.soldierUnit >= 0 );
				GLASSERT( selection.targetUnit >= 0 );
				Vector3F target;
				selection.targetUnit->GetModel()->CalcTarget( &target );

				// Stack - push in reverse order.
				if ( icon == 0 ) {
					Action delay;
					delay.Delay( 500 );


					ShootAction( selection.soldierUnit, target );
					actionStack.Push( delay );
					ShootAction( selection.soldierUnit, target );
					actionStack.Push( delay );
				}
				ShootAction( selection.soldierUnit, target );
				RotateAction( selection.soldierUnit, selection.targetUnit, true );
				selection.targetUnit = 0;
				break;

			default:
				break;
		}
	}

	if ( icon == -1 ) {
		icon = widgets->QueryTap( screenX, screenY );

		switch( icon ) {
			case 0:
				engine->MoveCameraHome();
				break;

			case 1:
				if ( engine->GetDayTime() )
					engine->SetDayNight( false, game->GetLightMap( "farmlandN" ) );
				else
					engine->SetDayNight( true, 0 );
				break;

			case 2:
				if ( actionStack.Empty() && SelectedSoldierUnit() ) {
					UFOStream* stream = game->OpenStream( "SingleUnit" );
					SelectedSoldierUnit()->Save( stream );
					game->PushScene( Game::CHARACTER_SCENE );
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
	bool iconSelected = false;
	if ( tap == 1 ) {
		iconSelected = HandleIconTap( screen.x, screen.y );
	}

#ifdef MAPMAKER
	if ( !iconSelected ) {
		const Vector3F& pos = selection->Pos();
		int rotation = (int) (selection->GetYRotation() / 90.0f );
		engine->GetMap()->AddToTile( (int)pos.x, (int)pos.z, currentMapItem, rotation );
		icon = 0;	// don't keep processing
	}
#endif	

	if ( !iconSelected && actionStack.Empty() ) {
		// We didn't tap a button.
		// What got tapped? First look to see if a SELECTABLE model was tapped. If not, 
		// look for a selectable model from the tile.

		Vector3F intersect;
		Vector2I tilePos = { 0, 0 };
		int result = IntersectRayPlane( world.origin, world.direction, 1, 0.0f, &intersect );
		if ( result == grinliz::INTERSECT ) {
			tilePos.Set( (int)intersect.x, (int) intersect.z );
		}

		// If there is a selected model, then we can tap a target model.
		bool canSelectAlien =    SelectedSoldier()			// a soldier is selected
			                  && !PathEndModel();			// there is not a path

		for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) {
			if ( units[i].GetModel() ) {
				if ( canSelectAlien && units[i].IsAlive() )
					units[i].GetModel()->SetFlag( Model::MODEL_SELECTABLE );
				else
					units[i].GetModel()->ClearFlag( Model::MODEL_SELECTABLE );
			}	
		}

		Model* model = engine->IntersectModel( world, TEST_HIT_AABB, Model::MODEL_SELECTABLE, 0, 0 );
		if ( !model && result == grinliz::INTERSECT && PathEndModel() ) {
			// check the path end model.
			if ( (int)PathEndModel()->X() == tilePos.x && (int)PathEndModel()->Z() == tilePos.y ) {
				model = PathEndModel();
			}
		}

		if ( model ) {
			Model* selected = SelectedSoldierModel();
			if ( model == selected ) {
				// If there is a path, clear it.
				// If there is no path, go to rotate mode.
				path.Clear();
				FreePathEndModel();
			}
			else if ( model == PathEndModel() ) {
				// Go!
				Action action;
				action.Move( SelectedSoldierUnit() );
				actionStack.Push( action );
			}
			else {
				path.Clear();
				SetSelection( UnitFromModel( model ) );
			}
		}
		else {
			// Not a model - which tile?
			Model* selected = SelectedSoldierModel();

			if ( selected ) {
				Vector2<S16> start   = { (S16)selected->X(), (S16)selected->Z() };

				if ( result == grinliz::INTERSECT ) {

					Vector2<S16> end = { (S16)intersect.x, (S16)intersect.z };
					if ( end.x >= 0 && end.y >=0 && end.x < Map::SIZE && end.y < Map::SIZE ) {
					
						if ( start != path.start || end != path.end ) {
							path.len = 0;
							path.start = start;
							path.end = end;

							if ( start != end ) {
								Map* map = engine->GetMap();
								map->ClearPathBlocks();

								for( int i=0; i<MAX_UNITS; ++i ) {
									if (    units[i].Status() == Unit::STATUS_ALIVE 
										 && units[i].GetModel() 
										 && units[i].GetModel() != selected ) // oops - don't cause self to not path
									{
										int x = (int)units[i].GetModel()->X();
										int z = (int)units[i].GetModel()->Z();
										map->SetPathBlock( x, z );
									}
								}
								float cost;
								map->SolvePath( start, end, &cost, path.path, &path.len, path.MAX_PATH );
							}
							FreePathEndModel();

							if ( path.len > 1 ) {
								selection.targetUnit = 0;
								selection.pathEndModel = engine->AllocModel( selected->GetResource() );
								selection.pathEndModel->SetPos( (float)path.end.x + 0.5f, 0.0f, (float)path.end.y + 0.5f );
								selection.pathEndModel->SetTexture( game->GetTexture( "translucent" ) );
								selection.pathEndModel->SetTexXForm( 0, 0, TRANSLUCENT_WHITE, 0.5f );
								selection.pathEndModel->SetFlag( Model::MODEL_SELECTABLE );	// FIXME not needed, remove entire draggable thing

								float rot;
								path.CalcDelta( path.len-2, path.len-1, 0, &rot );
								selection.pathEndModel->SetYRotation( rot );
							}
						}
					}
				}
			}
		}
	}
}


Unit* BattleScene::UnitFromModel( Model* m )
{
	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( units[i].GetModel() == m )
			return &units[i];
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
	// { "home", "d/n", character screen, "fow" };
	widgets->SetEnabled( 2, SelectedSoldierUnit() && actionStack.Empty() );
	widgets->Draw();

	if ( AlienTargeted() ) {
		fireWidget->Draw();
	}

#ifdef MAPMAKER
	engine->GetMap()->DumpTile( (int)selection->X(), (int)selection->Z() );
	UFOText::Draw( 0,  16, "0x%2x:'%s'", currentMapItem, engine->GetMap()->GetItemDefName( currentMapItem ) );
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
	GLASSERT( i0>=0 && i0<len-1 );
	GLASSERT( i1>=1 && i1<len );

	int dx = path[i1].x - path[i0].x;
	int dy = path[i1].y - path[i0].y;
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

	GLASSERT( *pos < len-1 );

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
	GLASSERT( step < len );
	GLASSERT( fraction >= 0.0f && fraction < 1.0f );
	if ( step == len-1 ) {
		step = len-2;
		fraction = 1.0f;
	}
	int dx = path[step+1].x - path[step].x;
	int dy = path[step+1].y - path[step].y;
	*x = (float)path[step].x + fraction*(float)( dx );
	*z = (float)path[step].y + fraction*(float)( dy );
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
		preview = engine->GetMap()->CreatePreview(	(int)selection->X(), 
													(int)selection->Z(), 
													currentMapItem, 
													(int)(selection->GetYRotation()/90.0f) );

		if ( preview ) {
			const Texture* t = game->GetTexture( "translucent" );
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
	selection->SetPos( (float)newX + 0.5f, 0.0f, (float)newZ + 0.5f );
	
	UpdatePreview();
}

void BattleScene::RotateSelection( int delta )
{
	float rot = selection->GetYRotation() + 90.0f*(float)delta;
	selection->SetYRotation( rot );
	UpdatePreview();
}

void BattleScene::DeleteAtSelection()
{
	const Vector3F& pos = selection->Pos();
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
