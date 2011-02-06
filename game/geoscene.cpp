#include "geoscene.h"
#include "game.h"
#include "cgame.h"
#include "areaWidget.h"
#include "geomap.h"

#include "../engine/loosequadtree.h"

using namespace grinliz;
using namespace gamui;

// t: tundra
// a: farm
// d: desert
// f: forest
// c: city


static const char* MAP =
	"w. 0t 0t 0t 0t 0t 0t w. w. 2c 2t 2t 2t 2t 2t 2t 2t 2t 2t 2t"
	"w. 0t 0t 0t 0t 0t 0t 2c 2c 2a 2a 2t 2t 2t 2t 2t 2t 2t w. w."
	"w. 0c 0d 0a 0c w. w. 2c w. 2c 2a 2d 3d 2a 3a 3a 3a 3a w. w."
	"w. 0c 0d 0a w. w. w. 4c 4d 4d 4c 3d 3d 3a 3a 3a 3a 3c w. w."
	"w. w. 0c w. 0f w. w. 4f 4f 4d 4d 3d 3c w. 3f w. 3c w. w. w."
	"w. w. w. 0f 1c w. w. 4f 4f 4f 4d 4f w. w. 3c w. w. w. w. w."
	"w. w. w. w. 1f 1f 1f w. w. 4f 4f w. w. w. w. w. 3f 3f w. w."
	"w. w. w. w. 1f 1a 1c w. w. 4f 4f w. w. w. w. w. w. w. 5c w."
	"w. w. w. w. w. 1f 1f w. w. 4f 4f 4f w. w. w. w. w. 5f 5d 5a"
	"w. w. w. w. w. 1f w. w. w. 4c w. w. w. w. w. w. w. 5f 5d 5c"
;


GeoScene::GeoScene( Game* _game ) : Scene( _game )
{
	RenderAtom atom = _game->GetRenderAtom( Game::ATOM_GEO_BACKGROUND );
	const Screenport& port = GetEngine()->GetScreenport();

	geoMap = new GeoMap( GetEngine()->GetSpaceTree() );
	tree = GetEngine()->GetSpaceTree();

	mapOffset = 0;
	WIDTH = 2.0f * port.UIHeight();
	HEIGHT = port.UIHeight();


	static const char* names[NUM_AREAS] = { "North", "South", "Europa", "Asia", "Africa", "Under" };
	for( int i=0; i<NUM_AREAS; ++i ) {
		areaWidget[i] = new AreaWidget( _game, &gamui2D, names[i] );
	}

}


GeoScene::~GeoScene()
{
	for( int i=0; i<chitArr.Size(); ++i ) {
		chitArr[i].Free( tree );
	}

	for( int i=0; i<NUM_AREAS; ++i ) {
		delete areaWidget[i];
	}
	delete geoMap;
}


void GeoScene::Activate()
{
	GetEngine()->CameraIso( false, false, (float)GeoMap::MAP_X, (float)GeoMap::MAP_Y );
	SetMapLocation();
	GetEngine()->SetIMap( geoMap );

	Chit* test = chitArr.Push();
	test->InitAlienShip( tree, Chit::SCOUT, 5.0f, 5.0f, 12.5f, 5.5f );
}


void GeoScene::DeActivate()
{
	GetEngine()->CameraIso( true, false, 0, 0 );
	GetEngine()->SetIMap( 0 );
}

void GeoScene::Tap(	int action, 
					const grinliz::Vector2F& view,
					const grinliz::Ray& world )
{
	Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( view, &ui );

	if ( action == GAME_TAP_DOWN || action == GAME_TAP_DOWN_PANNING ) {
		if ( action == GAME_TAP_DOWN ) {
			// First check buttons.
			gamui2D.TapDown( ui.x, ui.y );
		}

		dragStart.Set( -1, -1 );

		if ( !gamui2D.TapCaptured() ) {
			dragStart = ui;
		}
	}
	else if ( action == GAME_TAP_MOVE || action == GAME_TAP_MOVE_PANNING ) {
		if ( dragStart.x >= 0 ) {
			mapOffset += ui.x - dragStart.x;
			dragStart = ui;
			SetMapLocation();
		}
	}
	else if ( action == GAME_TAP_UP || action == GAME_TAP_UP_PANNING ) {
		if ( dragStart.x >= 0 ) {
			mapOffset += ui.x - dragStart.x;
			dragStart = ui;
			SetMapLocation();
		}
		
		if ( gamui2D.TapCaptured() ) {
			//HandleIconTap( item );
		}
	}
	else if ( action == GAME_TAP_CANCEL || action == GAME_TAP_CANCEL_PANNING ) {
		gamui2D.TapUp( ui.x, ui.y );
		dragStart.Set( -1, -1 );
	}

}


void GeoScene::SetMapLocation()
{
	const Screenport& port = GetEngine()->GetScreenport();

	while( mapOffset >= WIDTH )
		mapOffset -= WIDTH;
	while( mapOffset < 0 )
		mapOffset += WIDTH;

	float dx = mapOffset / WIDTH;
	geoMap->SetScrolling( dx );

	float border = 1.0f - port.UIWidth() / ( port.UIHeight()*2.0f );
	float x0 = (float)GeoMap::MAP_X * (border / 2.0f );
	float x1 = (float)GeoMap::MAP_X * ( 1.0f - border/2.0f );

	for( int i=0; i<chitArr.Size(); ++i ) {
		chitArr[i].SetScroll( dx*(float)GeoMap::MAP_X, x0, x1 );
	}

	static const Vector2F pos[NUM_AREAS] = {
		{ 64.f/1000.f, 9.f/500.f },
		{ 97.f/1000.f, 455.f/500.f },
		{ 502.f/1000.f, 9.f/500.f },
		{ 718.f/1000.f, 106.f/500.f },
		{ 503.f/1000.f, 459.f/500.f },
		{ 744.f/1000.f, 459.f/500.f },
	};

	for( int i=0; i<NUM_AREAS; ++i ) {
		float x = pos[i].x*WIDTH + mapOffset;
		if ( x > port.UIWidth() )
			x -= WIDTH;

		areaWidget[i]->SetOrigin( x, pos[i].y*port.UIHeight() );
	}
}


void GeoScene::DoTick( U32 currentTime, U32 deltaTime )
{
	geoMap->DoTick( currentTime, deltaTime );

	for( int i=0; i<chitArr.Size(); ++i ) {
		chitArr[i].DoTick( currentTime, deltaTime );
	}

	const Screenport& port = GetEngine()->GetScreenport();
	float dx = mapOffset / WIDTH;
	float border = 1.0f - port.UIWidth() / ( port.UIHeight()*2.0f );
	float x0 = (float)GeoMap::MAP_X * (border / 2.0f );
	float x1 = (float)GeoMap::MAP_X * ( 1.0f - border/2.0f );

	for( int i=0; i<chitArr.Size(); ++i ) {
		chitArr[i].SetScroll( dx*(float)GeoMap::MAP_X, x0, x1 );
	}
}


static const float SHIP_HEIGHT = 1.0f;

void Chit::InitAlienShip( SpaceTree* tree, int type, const grinliz::Vector2F& start, const grinliz::Vector2F& dest )
{
	this->pos = start;
	this->dest = dest;
	this->type = type;

	static const char* name[3] = { "geo_scout", "geo_frigate", "geo_battleship" };
	GLASSERT( type >= 0 && type < 3 );

	model = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( name[type] ) );
	model->SetPos( start.x, SHIP_HEIGHT, start.y );
}


void Chit::Free( SpaceTree* tree )
{
	if ( model ) {
		tree->FreeModel( model );
		model = 0;
	}
}


void Chit::SetScroll( float deltaMap, float x0, float x1 )
{
	if ( model ) {
		Vector2F p = pos;
		p.x += deltaMap;
		while ( p.x > x1 )
			p.x -= (float)GeoMap::MAP_X;
		while ( p.x < x0 )
			p.x += (float)GeoMap::MAP_X;
		model->SetPos( p.x, SHIP_HEIGHT, p.y );
	}
}


void Chit::DoTick( U32 currentTime, U32 deltaTime )
{
	static const float SPEED[NUM_TYPES] = { 1.0f, 0.8f, 0.5f, 1.5f };

	float travel = SPEED[type] * (float)deltaTime / 1000.0f;

	Vector2F normal = dest - pos;
	if ( normal.Length() >= travel ) {
		normal.Normalize();
		normal = normal * travel;

		pos += normal;

		if ( model ) {
			const Vector3F& mPos = model->Pos();
			//SetScroll will sort these out. Called after doTick
			//model->SetPos( mPos.x+normal.x, mPos.y, mPos.z+normal.y );
		}
	}
}

