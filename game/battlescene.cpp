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

	action.Clear();
	engine  = &game->engine;

	path.Clear();

	Texture* t = game->GetTexture( "icons" );	
	widgets = new UIButtonBox( t, engine->GetScreenport() );

	int icons[] = { UIButtonBox::ICON_PLAIN, UIButtonBox::ICON_PLAIN, UIButtonBox::ICON_CHARACTER };
	const char* iconText[] = { "home", "d/n", 0 };
	widgets->CalcButtons( icons, iconText, 3 );
}


BattleScene::~BattleScene()
{
	delete widgets;
}


void BattleScene::InitUnits()
{
	int Z = engine->GetMap()->Height();
	Random random(5);

	for( int i=0; i<6; ++i ) {
		Vector2I pos = { (i*2)+10, Z-10 };
		units[TERRAN_UNITS_START+i].Init( engine, game, Unit::SOLDIER, 0, random.Rand() );
		units[TERRAN_UNITS_START+i].SetWeapon( game->GetItemDef( "gun0" ) );
		units[TERRAN_UNITS_START+i].SetMapPos( pos.x, pos.y );
	}
	
	for( int i=0; i<4; ++i ) {
		Vector2I pos = { (i*2)+10, Z-8 };
		units[ALIEN_UNITS_START+i].Init( engine, game, Unit::ALIEN, i&3, random.Rand() );
		units[ALIEN_UNITS_START+i].SetWeapon( game->GetItemDef( "gun1" ) );
		units[ALIEN_UNITS_START+i].SetMapPos( pos.x, pos.y );
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
	s->WriteU32( UFOStream::MAGIC0 );
	// FIXME
	//s->WriteU32( selectedUnit );

	for( int i=0; i<MAX_UNITS; ++i ) {
		units[i].Save( s );
	}
	s->WriteU32( UFOStream::MAGIC1 );
}


void BattleScene::Load( UFOStream* s )
{
	U32 magic = s->ReadU32();
	GLASSERT( magic == UFOStream::MAGIC0 );
	// FIXME
	//selectedUnit = s->ReadU32();

	for( int i=0; i<MAX_UNITS; ++i ) {
		units[i].Load( s, engine, game );
	}
	magic = s->ReadU32();
	GLASSERT( magic == UFOStream::MAGIC1 );
	SetUnitsDraggable();
}


void BattleScene::Activate()
{
	engine->EnableMap( true );

	//int n = 0;
	ModelResource* resource = 0;
	path.Clear();
	action.Clear();

#ifdef MAPMAKER
	resource = game->GetResource( "selection" );
	selection = engine->AllocModel( resource );
	selection->SetPos( 0.5f, 0.0f, 0.5f );
	preview = 0;
#endif

	resource = game->GetResource( "crate" );
	//crateTest = engine->AllocModel( resource );
	//crateTest->Set( Model::MODEL_SELECTABLE );
	//crateTest->SetPos( 3.5f, 0.0f, 20.0f );

	// Do we have a saved state?
	UFOStream* stream = game->OpenStream( "BattleScene", false );
	if ( !stream ) {
		InitUnits();
	}
	else {
		Load( stream );
	}
}


void BattleScene::DeActivate()
{
	UFOStream* s =game->OpenStream( "BattleScene" );
	Save( s );

#ifdef MAPMAKER
	engine->FreeModel( selection );
	if ( preview ) {
		engine->FreeModel( preview );
	}
#endif
	for( int i=0; i<MAX_UNITS; ++i ) {
		units[i].Free();
	}
	FreePathEndModel();
	//engine->FreeModel( crateTest );
	game->particleSystem->Clear();
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

		game->particleSystem->Emit(	ParticleSystem::POINT,
									0,		// type
									40,		// count
									ParticleSystem::PARTICLE_SPHERE,
									col,	colVel,
									pos,	0.1f,	
									vel,	0.1f,
									1200 );
	}
	grinliz::Vector3F pos = { 13.f, 0.0f, 28.0f };
	game->particleSystem->EmitFlame( deltaTime, pos );

	if (    SelectedTerran()
		 && SelectedTerranUnit()->Status() == Unit::STATUS_ALIVE
		 && SelectedTerranUnit()->Team() == Unit::SOLDIER ) 
	{
		Model* m = SelectedTerranModel();
		if ( m ) {
			//const U32 INTERVAL = 500;
			float alpha = 0.5f;	//0.5f + 0.4f*(float)(currentTime%500)/(float)(INTERVAL);
			game->particleSystem->EmitDecal( ParticleSystem::DECAL_SELECTED, 
											 ParticleSystem::DECAL_BOTTOM,
											 m->Pos(), alpha,
											 m->GetYRotation() );
		}
	}
	if ( action.action == ACTION_NONE ) {
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
	else if ( action.action == ACTION_MOVE ) {
		// Move the unit.

		const float SPEED = 3.0f;
		float travel = SPEED * (float)deltaTime / 1000.0f;

		while( action.pathStep < path.len-1 && travel > 0.0f ) {
			path.Travel( &travel, &action.pathStep, &action.pathFraction );
		}
		float x, z, r;
		path.GetPos( action.pathStep, action.pathFraction, &x, &z, &r );

		//Model* selected = SelectedTerranModel();
		//selected->SetPos( x+0.5f, 0.0f, z+0.5f );
		//selected->SetYRotation( r );
		Vector3F v = { x+0.5f, 0.0f, z+0.5f };
		Unit* unit = SelectedTerranUnit();
		unit->SetPos( v, r );

		if ( action.pathStep == path.len-1 ) {
			action.Clear();
			path.Clear();
			FreePathEndModel();
		}
	}

	if ( AlienTargeted() ) {
		Vector3F pos = { 0, 0, 0 };
		AlienUnit()->CalcPos( &pos );

		const float ALPHA = 0.3f;
		game->particleSystem->EmitDecal( ParticleSystem::DECALTARGET,
										 ParticleSystem::DECAL_BOTH,
										 pos, ALPHA, 0 );


	}
}


void BattleScene::FreePathEndModel()
{
	if ( selection.pathEndModel ) {
		engine->FreeModel( selection.pathEndModel );
		selection.pathEndModel = 0;
	}
}


void BattleScene::SetSelection( int unit ) 
{
	FreePathEndModel();
	if ( unit >= TERRAN_UNITS_START && unit < TERRAN_UNITS_END ) {
		selection.terranUnit = unit;
		selection.targetUnit = -1;
	}
	else if ( unit >= ALIEN_UNITS_START && unit < ALIEN_UNITS_END ) {
		GLASSERT( SelectedTerran() );
		selection.targetUnit = unit;
	}
}


bool BattleScene::HandleIconTap( int screenX, int screenY )
{
	bool iconTapped = true;
	int icon = widgets->QueryTap( screenX, screenY );

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
			if ( action.action == ACTION_NONE ) {
				game->PushScene( Game::CHARACTER_SCENE );
			}
			break;

		default:
			iconTapped = false;
			break;
	}
	return iconTapped;
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

	if ( !iconSelected && action.NoAction() ) {
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
		bool canSelectAlien =    SelectedTerran()			// a soldier is selected
			                  && !PathEndModel();			// there is not a path

		for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) {
			if ( units[i].GetModel() ) {
				if ( canSelectAlien )
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
			Model* selected = SelectedTerranModel();
			if ( model == selected ) {
				// If there is a path, clear it.
				// If there is no path, go to rotate mode.
				path.Clear();
				FreePathEndModel();
			}
			else if ( model == PathEndModel() ) {
				// Go!
				action.Move();
			}
			else {
				path.Clear();
				int unit = UnitFromModel( model );
				SetSelection( unit );
			}
		}
		else {
			// Not a model - which tile?
			Model* selected = SelectedTerranModel();

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
								selection.targetUnit = -1;
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


int BattleScene::UnitFromModel( Model* m )
{
	for( int i=0; i<MAX_UNITS; ++i ) {
		if ( units[i].GetModel() == m )
			return i;
	}
	return -1;
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


void BattleScene::Drag( int action, const grinliz::Vector2I& screenRaw )
{
	switch ( action ) 
	{
		case GAME_DRAG_START:
		{
			Ray ray;
			engine->CalcModelViewProjectionInverse( &dragMVPI );
			engine->RayFromScreenToYPlane( screenRaw.x, screenRaw.y, dragMVPI, &ray, &dragStart );
			dragStartCameraWC = engine->camera.PosWC();
		}
		break;

		case GAME_DRAG_MOVE:
		{
			Vector3F drag;
			Ray ray;
			engine->RayFromScreenToYPlane( screenRaw.x, screenRaw.y, dragMVPI, &ray, &drag );

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
	widgets->Draw();

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
