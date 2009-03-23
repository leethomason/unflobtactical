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

	int n = 0;
	ModelResource* resource = 0;
	float mz = (float)engine->GetMap()->Height();
	memset( testModel, 0, NUM_TEST_MODEL*sizeof(Model*) );

#ifdef MAPMAKER
	resource = game->GetResource( "selection" );
	selection = engine->GetModel( resource );
	selection->SetPos( 0.5f, 0.0f, 0.5f );
#endif

	resource = game->GetResource( "crate" );
	testModel[n] = engine->GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 3.5f, 0.0f, mz-5.5f );

	resource = game->GetResource( "maleMarine" );
	for( int i=0; i<4; ++i ) {
		testModel[n] = engine->GetModel( resource );
		testModel[n]->SetPos( (float)(i*2)+10.5f, 0.0f, mz-7.5f );
		testModel[n]->SetDraggable( true );
		++n;
	}
	testModel[n-4]->SetSkin( 1, 1, 1 );
	testModel[n-3]->SetSkin( 2, 1, 1 );
	testModel[n-2]->SetSkin( 1, 0, 0 );
	testModel[n-1]->SetSkin( 1, 0, 2 );

	resource = game->GetResource( "femaleMarine" );
	testModel[n] = engine->GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 19.5f, 0.0f, mz-9.5f );
	
	resource = game->GetResource( "alien0" );
	testModel[n] = engine->GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 19.5f, 0.0f, mz-7.5f );

	resource = game->GetResource( "alien1" );
	testModel[n] = engine->GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 19.5f, 0.0f, mz-5.5f );

	resource = game->GetResource( "alien2" );
	testModel[n] = engine->GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 19.5f, 0.0f, mz-3.5f );

	resource = game->GetResource( "alien3" );
	testModel[n] = engine->GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 19.5f, 0.0f, mz-1.5f );
}


BattleScene::~BattleScene()
{
#ifdef MAPMAKER
	engine->ReleaseModel( selection );
#endif

	delete widgets;
	for( int i=0; i<NUM_TEST_MODEL; ++i ) {
		if ( testModel[i] ) {
			engine->ReleaseModel( testModel[i] );
		}
	}
}


void BattleScene::Activate()
{
}


void BattleScene::DeActivate()
{
}


void BattleScene::DoTick( U32 currentTime, U32 deltaTime )
{
	grinliz::Vector3F v;
	ConvertVector3( testModel[7]->Pos(), &v );
	game->particleSystem->EmitDecal( 1, v, 0.7f, testModel[7]->GetYRotation() );
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
				int dx = LRintf( delta.x );
				int dz = LRintf( delta.z );
				draggingModel->SetPos(	draggingModelOrigin.x + Fixed(dx),
										draggingModelOrigin.y,
										draggingModelOrigin.z + Fixed(dz) );
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