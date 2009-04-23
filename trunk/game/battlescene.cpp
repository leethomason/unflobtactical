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

	selected = -1;
	pathEndModel = 0;
	engine  = &game->engine;
	pathLen = 0;
	pathStart.Set( -1, -1 );
	pathEnd.Set( -1, -1 );

	Texture* t = game->GetTexture( "icons" );	
	widgets = new UIButtonBox( t );

	int icons[] = { UIButtonBox::ICON_PLAIN, UIButtonBox::ICON_PLAIN, UIButtonBox::ICON_CHARACTER };
	const char* iconText[] = { "home", "d/n", 0 };
	widgets->SetButtons( icons, iconText, 3 );
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
		units[TERRAN_UNITS_START+i].GenerateTerran( random.Rand() );
		units[TERRAN_UNITS_START+i].CreateModel( game, engine );
		units[TERRAN_UNITS_START+i].SetPos( pos.x, pos.y );
	}
	
	for( int i=0; i<4; ++i ) {
		Vector2I pos = { (i*2)+10, Z-8 };
		units[ALIEN_UNITS_START+i].GenerateAlien( i&3, random.Rand() );
		units[ALIEN_UNITS_START+i].CreateModel( game, engine );
		units[ALIEN_UNITS_START+i].SetPos( pos.x, pos.y );
	}

	for( int i=0; i<4; ++i ) {
		Vector2I pos = { (i*2)+10, Z-12 };
		units[CIV_UNITS_START+i].GenerateCiv( random.Rand() );
		units[CIV_UNITS_START+i].CreateModel( game, engine );
		units[CIV_UNITS_START+i].SetPos( pos.x, pos.y );
	}
	SetUnitsDraggable();
}


void BattleScene::SetUnitsDraggable()
{
	for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i ) {
		Model* m = units[i].GetModel();
		if ( m && units[i].Status() == Unit::STATUS_ALIVE ) {
			m->Set( Model::MODEL_DRAGGABLE );
		}
	}
}


void BattleScene::Save( UFOStream* s )
{
	s->WriteU32( UFOStream::MAGIC0 );
	s->WriteU32( selected );

	for( int i=0; i<MAX_UNITS; ++i ) {
		units[i].Save( s );
		if ( units[i].Status() != Unit::STATUS_UNUSED ) {
			Model* model = units[i].GetModel();
			GLASSERT( model );

			s->WriteFloat( model->Pos().x );
			s->WriteFloat( model->Pos().z );
			s->WriteFloat( model->GetYRotation() );
		}
	}
	s->WriteU32( UFOStream::MAGIC1 );
}


void BattleScene::Load( UFOStream* s )
{
	U32 magic = s->ReadU32();
	GLASSERT( magic == UFOStream::MAGIC0 );
	selected = s->ReadU32();

	for( int i=0; i<MAX_UNITS; ++i ) {
		units[i].Load( s );
		if ( units[i].Status() != Unit::STATUS_UNUSED ) {
			Model* model = units[i].CreateModel( game, engine );
			Vector3F pos = { 0, 0, 0 };
			float rot;

			pos.x = s->ReadFloat();
			pos.z = s->ReadFloat();
			rot   = s->ReadFloat();

			model->SetPos( pos );
			model->SetYRotation( rot );
		}
	}
	magic = s->ReadU32();
	GLASSERT( magic == UFOStream::MAGIC1 );
	SetUnitsDraggable();
}


void BattleScene::Activate()
{
	engine->EnableMap( true );

	int n = 0;
	ModelResource* resource = 0;
	pathEndModel = 0;
	pathLen = 0;

#ifdef MAPMAKER
	resource = game->GetResource( "selection" );
	selection = engine->AllocModel( resource );
	selection->SetPos( 0.5f, 0.0f, 0.5f );
	preview = 0;
#endif

	resource = game->GetResource( "crate" );
	crateTest = engine->AllocModel( resource );
	//crateTest->SetDraggable( true );
	crateTest->Set( Model::MODEL_DRAGGABLE );
	crateTest->SetPos( 3.5f, 0.0f, 20.0f );

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
		units[i].FreeModel();
	}
	if ( pathEndModel ) {
		engine->FreeModel( pathEndModel );
		pathEndModel = 0;
	}
	engine->FreeModel( crateTest );
	game->particleSystem->Clear();
}


void BattleScene::DoTick( U32 currentTime, U32 deltaTime )
{
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

	if (    selected >= 0 && selected < MAX_UNITS 
		 && units[selected].Status() == Unit::STATUS_ALIVE
		 && units[selected].Team() == Unit::TERRAN_MARINE ) 
	{
		Model* m = units[selected].GetModel();
		if ( m ) {
			//const U32 INTERVAL = 500;
			float alpha = 0.7f;	//0.5f + 0.4f*(float)(currentTime%500)/(float)(INTERVAL);
			game->particleSystem->EmitDecal( ParticleSystem::DECAL_SELECTED, 
											 m->Pos(), alpha,
											 m->GetYRotation() );
		}
	}
	if ( pathLen > 0 ) {
		float rot = 0.0f;
		for( int i=0; i<pathLen-1; ++i ) {
			float alpha = 0.7f;
			Vector3F pos = { (float)(path[i].x)+0.5f, 0.0f, (float)(path[i].y)+0.5f };

			int dx = path[i+1].x - path[i].x;
			int dy = path[i+1].y - path[i].y;
			rot = ToDegree( atan2( (float)dx, (float)dy ) );

			game->particleSystem->EmitDecal( ParticleSystem::DECAL_PATH, 
											 pos, alpha,
											 rot );
		}
	}
}


void BattleScene::Tap(	int tap, 
						const grinliz::Vector2I& screen,
						const grinliz::Ray& world )
{
	if ( tap == 1 ) {
		int icon = widgets->QueryTap( screen.x, screen.y );

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
				game->PushScene( Game::CHARACTER_SCENE );
				break;

			default:
				break;
		}

#ifdef MAPMAKER
		if ( icon < 0 ) {
			const Vector3F& pos = selection->Pos();
			int rotation = (int) (selection->GetYRotation() / 90.0f );
			engine->GetMap()->AddToTile( (int)pos.x, (int)pos.z, currentMapItem, rotation );
		}
#endif	

	}
	else if ( tap == 2 ) {
		Vector3F p;

		if ( grinliz::INTERSECT == IntersectRayPlane( world.origin, world.direction, 1, 0.0f, &p ) )
		{
			engine->MoveCameraXZ( p.x, p.z ); 
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


void BattleScene::Drag( int action, const grinliz::Vector2I& screenRaw )
{
	switch ( action ) 
	{
		case GAME_DRAG_START:
		{
			Ray ray;
			engine->CalcModelViewProjectionInverse( &dragMVPI );
			engine->RayFromScreenToYPlane( screenRaw.x, screenRaw.y, dragMVPI, &ray, &dragStart );

			draggingModel = engine->IntersectModel( ray, true );
			if ( draggingModel ) {
				draggingModelOrigin = draggingModel->Pos();
				selected = UnitFromModel( draggingModel );
				pathLen = 0;
			}
			else {
				dragStartCameraWC = engine->camera.PosWC();
			}
		}
		break;

		case GAME_DRAG_MOVE:
		{
			Vector3F drag;
			Ray ray;
			engine->RayFromScreenToYPlane( screenRaw.x, screenRaw.y, dragMVPI, &ray, &drag );

			Vector3F delta = drag - dragStart;
			delta.y = 0.0f;

			if ( draggingModel ) {
				//draggingModel->SetPos(	draggingModelOrigin.x + delta.x,
				//						draggingModelOrigin.y,
				//						draggingModelOrigin.z + delta.z );
				Vector2<S16> start = { (S16)draggingModelOrigin.x, (S16)draggingModelOrigin.z };
				Vector2<S16> end   = { (S16)(draggingModelOrigin.x+delta.x), (S16)(draggingModelOrigin.z+delta.z) };
				if ( pathStart != start || pathEnd != end ) {
					pathLen = 0;
					pathStart = start;
					pathEnd = end;

					if ( start != end ) {
						float cost;
						engine->GetMap()->SolvePath( start, end, &cost, path, &pathLen, MAX_PATH );
					}
					if ( pathEndModel ) {
						engine->FreeModel( pathEndModel );
						pathEndModel = 0;
					}
					if ( pathLen > 1 ) {
						pathEndModel = engine->AllocModel( draggingModel->GetResource() );
						pathEndModel->SetPos( (float)end.x + 0.5f, 0.0f, (float)end.y + 0.5f );
						pathEndModel->SetTexture( game->GetTexture( "translucent" ) );
						pathEndModel->SetTexXForm( 0, 0, TRANSLUCENT_WHITE, 0.5f );
						int dx = path[pathLen-1].x - path[pathLen-2].x;
						int dy = path[pathLen-1].y - path[pathLen-2].y;
						float rot = ToDegree( atan2( (float)dx, (float)dy ) );
						pathEndModel->SetYRotation( rot );
					}
				}
			}
			else {
				engine->camera.SetPosWC( dragStartCameraWC - delta );
				engine->RestrictCamera();
			}
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
	int h = engine->Width();
	int w = engine->Height();
	int rotation = game->Rotation();
	widgets->Draw( w, h, rotation );

#ifdef MAPMAKER
	engine->GetMap()->DumpTile( (int)selection->X(), (int)selection->Z() );
	UFOText::Draw( 0,  16, "0x%2x:'%s'", currentMapItem, engine->GetMap()->GetItemDefName( currentMapItem ) );
#endif
}


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
