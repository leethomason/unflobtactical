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
		units[TERRAN_UNITS_START+i].GetModel()->SetDraggable( true );
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
		units[CIV_UNITS_START+i].GetModel()->SetDraggable( true );
	}
}


void BattleScene::Activate()
{
	engine->EnableMap( true );

	int n = 0;
	ModelResource* resource = 0;

#ifdef MAPMAKER
	resource = game->GetResource( "selection" );
	selection = engine->AllocModel( resource );
	selection->SetPos( 0.5f, 0.0f, 0.5f );
#endif

	resource = game->GetResource( "crate" );
	crateTest = engine->AllocModel( resource );
	crateTest->SetDraggable( true );
	crateTest->SetPos( 3.5f, 0.0f, 20.0f );

	// Do we have a saved state?
	Stream* stream = game->OpenStream( "BattleScene", false );
	if ( !stream ) {
		InitUnits();
	}
}


void BattleScene::DeActivate()
{
#ifdef MAPMAKER
	engine->FreeModel( selection );
#endif
	for( int i=0; i<MAX_UNITS; ++i ) {
		units[i].FreeModel();
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

	//game->particleSystem->EmitDecal( 1, testModel[7]->Pos(), 0.7f, testModel[7]->GetYRotation() );
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
		if ( icon == UIWidgets::ICON_COUNT ) {
			const Vector3X& pos = selection->Pos();
			int rotation = (int) (selection->GetYRotation() / grinliz::Fixed(90) );
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
				draggingModel->SetPos(	draggingModelOrigin.x + delta.x,
										draggingModelOrigin.y,
										draggingModelOrigin.z + delta.z );
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


void BattleScene::DrawHUD()
{
	int h = engine->Width();
	int w = engine->Height();
	int rotation = game->Rotation();
	widgets->Draw( w, h, rotation );

#ifdef MAPMAKER
	UFODrawText( 0,  16, "%3d:'%s'", currentMapItem, engine->GetMap()->GetItemDefName( currentMapItem ) );
#endif
}


#ifdef MAPMAKER

void BattleScene::MouseMove( int x, int y )
{
	grinliz::Matrix4 mvpi;
	grinliz::Ray ray;
	grinliz::Vector3F p;

	engine->CalcModelViewProjectionInverse( &mvpi );
	engine->RayFromScreenToYPlane( x, y, mvpi, &ray, &p );

	int newX = (int)( p.x );
	int newZ = (int)( p.z );
	selection->SetPos( (float)newX + 0.5f, 0.0f, (float)newZ + 0.5f );
}

void BattleScene::RotateSelection()
{
	grinliz::Fixed rot = selection->GetYRotation() + grinliz::Fixed(90);
	selection->SetYRotation( rot );
}

void BattleScene::DeleteAtSelection()
{
	Vector3X pos = selection->Pos();
	engine->GetMap()->DeleteAt( (int)pos.x, (int)pos.z );
}


void BattleScene::DeltaCurrentMapItem( int d )
{
	currentMapItem += d;
	while ( currentMapItem < 0 ) { currentMapItem += Map::MAX_ITEM_DEF; }
	while ( currentMapItem >= Map::MAX_ITEM_DEF ) { currentMapItem -= Map::MAX_ITEM_DEF; }
	if ( currentMapItem == 0 ) currentMapItem = 1;
}

#endif