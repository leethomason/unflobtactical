#include "battlescene.h"
#include "game.h"
#include "cgame.h"

#include "../engine/uirendering.h"
#include "../engine/platformgl.h"
#include "../engine/particle.h"
#include "../engine/text.h"

#include "battlestream.h"

#include "../grinliz/glfixed.h"


using namespace grinliz;


BattleScene::BattleScene( Game* game ) : Scene( game )
{
#ifdef MAPMAKER
	currentMapItem = 1;
#endif
	engine  = &game->engine;

	path.Clear();

	// On screen menu.
	widgets = new UIButtonBox( engine->GetScreenport() );
	
	int x, y, w, h;

	{
		const int icons[] = { ICON_GREEN_BUTTON, ICON_GREEN_BUTTON, ICON_GREEN_BUTTON, ICON_GREEN_BUTTON };
		const char* iconText[] = { "home", "d/n", 0, "fow" };
		widgets->InitButtons( icons, 4 );
		widgets->SetDeco( 2, DECO_CHARACTER );
		widgets->SetText( iconText );
		widgets->CalcDimensions( &x, &y, &w, &h );
		widgets->SetOrigin( 0, engine->GetScreenport().UIHeight()-h );
	}
	// When enemy targeted.
	fireWidget = new UIButtonBox( engine->GetScreenport() );
	const int fireIcons[] = { ICON_TRANS_RED, ICON_TRANS_RED, ICON_TRANS_RED };
	fireWidget->InitButtons( fireIcons, 3 );
	fireWidget->SetDeco( 2, DECO_AIMED );
	fireWidget->SetDeco( 1, DECO_SNAP );
	fireWidget->SetDeco( 0, DECO_AUTO );

	engine->EnableMap( true );

#ifdef MAPMAKER
	{
		const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( "selection" );
		mapSelection = engine->AllocModel( res );
		mapSelection->SetPos( 0.5f, 0.0f, 0.5f );
		preview = 0;
	}
#endif

	// Do we have a saved state?
	UFOStream* stream = game->OpenStream( "BattleScene", false );
	if ( !stream ) {
		InitUnits();
	}
	else {
		Load( stream );
	}

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
	SetFogOfWar();
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
#endif

	delete fireWidget;
	delete widgets;
}


void BattleScene::InitUnits()
{
	int Z = engine->GetMap()->Height();
	Random random(5);

	Item gun0( game, "PST-1" ),
		 gun1( game, "RAY-1" ),
		 ar3( game, "AR-3" ),
		 medkit( game, "Med" ),
		 armor( game, "ARM-1" ),
		 fuel( game, "Gel" ),
		 grenade( game, "RPG", -1 ),
		 autoClip( game, "AClip", -1 ),
		 cell( game, "Cell", -1 ),
		 clip( game, "Clip", -1 );

	gun0.Insert( clip );
	gun1.Insert( cell );
	ar3.Insert( autoClip );
	ar3.Insert( grenade );

	for( int i=0; i<6; ++i ) {
		Vector2I pos = { (i*2)+10, Z-10 };
		Unit* unit = &units[TERRAN_UNITS_START+i];

		unit->Init( engine, game, Unit::SOLDIER, 0, random.Rand() );
		Inventory* inventory = unit->GetInventory();
		inventory->AddItem( Inventory::WEAPON_SLOT, gun0 );
		inventory->AddItem( Inventory::ANY_SLOT, Item( clip ));
		inventory->AddItem( Inventory::ANY_SLOT, Item( clip ));
		inventory->AddItem( Inventory::ANY_SLOT, medkit );
		inventory->AddItem( Inventory::ANY_SLOT, armor );
		inventory->AddItem( Inventory::ANY_SLOT, fuel );
		inventory->AddItem( Inventory::ANY_SLOT, ar3 );
		unit->UpdateInventory();
		unit->SetMapPos( pos.x, pos.y );
		unit->SetYRotation( (float)(((i+2)%8)*45) );
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
	SetUnitsDraggable();
}


void BattleScene::SetFogOfWar()
{
	grinliz::BitArray<Map::SIZE, Map::SIZE, 1>* fow = engine->GetFogOfWar();
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
	engine->UpdateFogOfWar();
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


void BattleScene::SetSelection( Unit* unit ) 
{
	//FreePathEndModel();

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
					// Move the unit. Be careful to never move more than one step (Travel() does not)
					// and be careful to handle rotation between steps so that visibility can update
					// and game events can happen.
					// This means that motion is really of 2 types: x,y and rotation.
					const float SPEED = 3.0f;
					const float ROTATION_SPEED = 150.0f;
					float x, z, r;
					
					Vector2I originalPos;
					float originalRot;
					unit->CalcMapPos( &originalPos, &originalRot );

					// Do we need to rotate, or move?
					path.GetPos( action->move.pathStep, action->move.pathFraction, &x, &z, &r );
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

						while(    (action->move.pathStep < (int)(path.statePath.size()-1))
							   && travel > 0.0f ) 
						{
							path.Travel( &travel, &action->move.pathStep, &action->move.pathFraction );
							if ( action->move.pathFraction == 0.0f ) {
								// crossed a path boundary.
								break;
							}
						}
						path.GetPos( action->move.pathStep, action->move.pathFraction, &x, &z, &r );

						Vector3F v = { x+0.5f, 0.0f, z+0.5f };
						unit->SetPos( v, model->GetYRotation() );

						if ( action->move.pathStep == path.statePath.size()-1 ) {
							actionStack.Pop();
							path.Clear();

							SetPathBlocks();
							Vector2<S16> start   = { (S16)model->X(), (S16)model->Z() };
							engine->GetMap()->ShowNearPath( start, 2.0f, 4.0f, 6.0f );
						}
					}
					Vector2I newPos;
					float newRot;
					unit->CalcMapPos( &newPos, &newRot );
					if ( newPos != originalPos || newRot != originalRot ) {
						CalcVisibility( unit, newPos.x, newPos.y, newRot );
						SetFogOfWar();
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
		const WeaponItemDef* weaponDef = weaponItem->GetItemDef()->IsWeapon();
		weaponDef->QueryWeaponRender( 0, &beamColor, &beamDecay, &beamWidth, &impactColor );

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
		const Model* ignore[] = { unit->GetModel(), unit->GetWeaponModel(), 0 };
		Model* m = engine->IntersectModel( ray, TEST_TRI, 0, 0, ignore, &hit );

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
					hitUnit->DoDamage( weaponDef->weapon[0].damageBase, weaponDef->weapon[0].shell );
					GLOUTPUT(( "Hit Unit 0x%x hp=%d/%d\n", hitUnit, hitUnit->GetStats().HP(), hitUnit->GetStats().TotalHP() ));
				}
			}
			else if ( m && m->IsFlagSet( Model::MODEL_OWNED_BY_MAP ) ) {
				// Hit world object.
				engine->GetMap()->DoDamage( weaponDef->weapon[0].damageBase, m, weaponDef->weapon[0].shell );
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
					stream->WriteU8( (U8)(SelectedSoldierUnit()-units ) );
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
		const Vector3F& pos = mapSelection->Pos();
		int rotation = (int) (mapSelection->GetYRotation() / 90.0f );
		engine->GetMap()->AddToTile( (int)pos.x, (int)pos.z, currentMapItem, rotation, -1, true );
		iconSelected = 0;	// don't keep processing
	}
#endif	

	if ( iconSelected || !actionStack.Empty() ) {
		return;
	}

	// We didn't tap a button.
	// What got tapped? First look to see if a SELECTABLE model was tapped. If not, 
	// look for a selectable model from the tile.

	Vector3F intersect;
	Map* map = engine->GetMap();

	Vector2I tilePos = { 0, 0 };
	int result = IntersectRayPlane( world.origin, world.direction, 1, 0.0f, &intersect );
	if ( result == grinliz::INTERSECT ) {
		tilePos.Set( (int)intersect.x, (int) intersect.z );
	}

	// If there is a selected model, then we can tap a target model.
	bool canSelectAlien = SelectedSoldier();			// a soldier is selected

	for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) {
		if ( units[i].GetModel() ) {
			if ( canSelectAlien && units[i].IsAlive() )
				units[i].GetModel()->SetFlag( Model::MODEL_SELECTABLE );
			else
				units[i].GetModel()->ClearFlag( Model::MODEL_SELECTABLE );
		}	
	}

	Model* model = engine->IntersectModel( world, TEST_HIT_AABB, Model::MODEL_SELECTABLE, 0, 0, 0 );

	if ( model ) {
		Model* selected = SelectedSoldierModel();
		SetSelection( UnitFromModel( model ) );		// sets either the Alien or the Unit

		if ( !selection.targetUnit ) {
			SetPathBlocks();
			Vector2<S16> start   = { (S16)model->X(), (S16)model->Z() };
			map->ShowNearPath( start, 2.0f, 4.0f, 6.0f );
		}
		else {
			map->ClearNearPath();
		}
	}
	else {
		// Not a model - use the tile
		if ( SelectedSoldierModel() && !selection.targetUnit ) {
			Vector2<S16> start   = { (S16)SelectedSoldierModel()->X(), (S16)SelectedSoldierModel()->Z() };

			if ( result == grinliz::INTERSECT ) {
				Vector2<S16> end = { (S16)intersect.x, (S16)intersect.z };
				if ( end.x >= 0 && end.y >=0 && end.x < Map::SIZE && end.y < Map::SIZE ) {
					if ( start != path.start || end != path.end ) {
						// Compute the path:
						SetPathBlocks();
						float cost;
						engine->GetMap()->SolvePath( start, end, &cost, &path.statePath );

						// Go!
						Action action;
						action.Move( SelectedSoldierUnit() );
						actionStack.Push( action );

						engine->GetMap()->ClearNearPath();
					}
				}
			}
		}
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


void BattleScene::CalcVisibility( const Unit* unit, int x, int y, float rotation )
{
	int unitID = unit - units;
	if ( unitID != 0 )
		return;
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
	
	Vector3F eye = { (float)(x+0.5f), EYE_HEIGHT, (float)(y+0.5f) };
	const Model* ignore[] = { unit->GetModel(), unit->GetWeaponModel(), 0 };

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

			// Max sight
			int dx = i-x;
			int dy = j-y;
			int len2 = dx*dx + dy*dy;
			if ( len2 > MAX_SIGHT_SQUARED )
				continue;

			// Correct direction
			Vector2I vec = { dx, dy };
			int dot = DotProduct( facing, vec );
			if ( dot < 0 )
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

			Vector2<Fixed> p = { Fixed(x)+Fixed(0.5f), Fixed(y)+Fixed(0.5f) };
			for( int k=0; k<steps; ++k ) {
				Vector2<Fixed> q = p;
				q.X(axis) += axisDir;
				q.X(!axis) += delta;

				Vector2I p0 = { (int)p.x, (int)p.y };
				Vector2I q0 = { (int)q.x, (int)q.y };
				bool canSee = engine->GetMap()->CanSee( p0, q0 );
				if ( canSee ) {
					// We are trying to see if the next (q0) can be scene. A unit can always
					// see itself, and looks out from there.
					visibilityMap.Set( q0.x, q0.y, unitID );
				}
				else {
					break;
				}
				p = q;
			}
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
	// { "home", "d/n", character screen, "fow" };
	widgets->SetEnabled( 2, SelectedSoldierUnit() && actionStack.Empty() );
	widgets->Draw();

	if ( AlienTargeted() ) {
		fireWidget->Draw();
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
	int len = (int)statePath.size();
	GLASSERT( i0>=0 && i0<len-1 );
	GLASSERT( i1>=1 && i1<len );
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


