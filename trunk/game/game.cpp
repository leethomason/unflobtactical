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

#include "game.h"
#include "cgame.h"
#include "scene.h"

#include "battlescene.h"
#include "characterscene.h"
#include "tacticalintroscene.h"
#include "tacticalendscene.h"
#include "helpscene.h"

#include "../engine/platformgl.h"
#include "../engine/text.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"
#include "../engine/particle.h"

#include "../grinliz/glmatrix.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glperformance.h"
#include "../grinliz/glstringutil.h"
#include "../tinyxml/tinyxml.h"
#include "../version.h"

using namespace grinliz;

extern int trianglesRendered;	// FIXME: should go away once all draw calls are moved to the enigine
extern int drawCalls;			// ditto
extern long memNewCount;

Game::Game( int width, int height, int rotation, const char* path ) :
	screenport( width, height, rotation ),
	engine( &screenport, engineData ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 ),
	trianglesPerSecond( 0 ),
	trianglesSinceMark( 0 ),
	previousTime( 0 ),
	isDragging( false ),
	sceneStack(),
	scenePopQueued( false ),
	scenePushQueued( -1 )
{
	savePath = path;
	char c = savePath[savePath.size()-1];
	if ( c != '\\' && c != '/' ) {	
#ifdef WIN32
		savePath += '\\';
#else
		savePath += '/';
#endif
	}	
	
	Init();
	Map* map = engine.GetMap();
	ImageManager* im = ImageManager::Instance();

//#ifndef MAPMAKER
//	map->SetSize( 40, 40 );
//	map->SetTexture( ImageManager::Instance()->GetImage("farmland" ), 0, 0 );
//	map->SetLightMaps( im->GetImage( "farmlandD" ),
//					   im->GetImage( "farmlandN" ),
//					   0, 0 );
//#endif

	engine.camera.SetPosWC( -12.f, 45.f, 52.f );	// standard test

	scenePushQueued = INTRO_SCENE;
	loadRequested = -1;
	loadCompleted = false;
	PushPopScene();
}


#ifdef MAPMAKER
Game::Game( int width, int height, int rotation, const char* path, const TileSetDesc& base ) :
	screenport( width, height, rotation ),
	engine( &screenport, engineData ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 ),
	trianglesPerSecond( 0 ),
	trianglesSinceMark( 0 ),
	previousTime( 0 ),
	isDragging( false ),
	sceneStack(),
	scenePopQueued( false ),
	scenePushQueued( -1 )
{
	savePath = path;
	char c = savePath[savePath.size()-1];
	if ( c != '\\' && c != '/' ) {	
#ifdef WIN32
		savePath += '\\';
#else
		savePath += '/';
#endif
	}	
	
	Init();
	Map* map = engine.GetMap();
	ImageManager* im = ImageManager::Instance();

	map->SetSize( base.size, base.size );

	char buffer[128];
	SNPrintf( buffer, 128, "%4s_%2d_%4s_%02d", base.set, base.size, base.type, base.variation );

	xmlFile  = std::string( path ) + std::string( buffer ) + std::string( ".xml" );
	std::string texture  = std::string( buffer ) + std::string( "_TEX" );
	std::string dayMap   = std::string( buffer ) + std::string( "_DAY" );
	std::string nightMap = std::string( buffer ) + std::string( "_NGT" );

//	map->SetTexture( im->GetImage( texture.c_str() ), 0, 0 );
//	map->SetLightMaps( im->GetImage( dayMap.c_str() ),
//					   im->GetImage( nightMap.c_str() ),
//					   0, 0 );

	engine.camera.SetPosWC( -25.f, 45.f, 30.f );	// standard test
	engine.camera.SetYRotation( -60.f );

	scenePushQueued = BATTLE_SCENE;
	loadRequested = -1;
	loadCompleted = false;
	PushPopScene();

	showPathing = false;

	TiXmlDocument doc( xmlFile );
	doc.LoadFile();
	if ( !doc.Error() )
		engine.GetMap()->Load( doc.FirstChildElement( "Map" ), GetItemDefArr() );
}
#endif


void Game::Init()
{
	currentFrame = 0;
	memset( &profile, 0, sizeof( ProfileData ) );
	surface.Set( Surface::RGBA16, 256, 256 );		// All the memory we will ever need (? or that is the intention)

	// Load the database.
	char buffer[260];
	PlatformPathToResource( "uforesource", "db", buffer, 260 );
	database = new gamedb::Reader();
	bool okay = database->Init( buffer );
	GLASSERT( okay );

	LoadTextures();
	LoadImages();
	modelLoader = new ModelLoader();
	LoadModels();
	LoadMapResources();
	LoadItemResources();

	delete modelLoader;
	modelLoader = 0;

	const Texture* textTexture = TextureManager::Instance()->GetTexture( "stdfont2" );
	GLASSERT( textTexture );
	UFOText::InitTexture( textTexture->glID );
	UFOText::InitScreen( &screenport );

	Map* map = engine.GetMap();
	ImageManager* im = ImageManager::Instance();
	map->SetLightObjects( im->GetImage( "objectLightMaps" ) );
}


Game::~Game()
{
#ifdef MAPMAKER
	TiXmlDocument doc( xmlFile );
	TiXmlElement map( "Map" );
	doc.InsertEndChild( map );
	engine.GetMap()->Save( doc.FirstChildElement( "Map" ) );
	doc.SaveFile();
#else
	if ( loadCompleted ) {
		TiXmlDocument doc;
		Save( &doc );
		std::string path = GameSavePath();
		doc.SaveFile( path );
	}
#endif

	while( !sceneStack.Empty() ) {
		delete sceneStack.Top();
		sceneStack.Pop();
	}
	for( unsigned i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		delete itemDefArr[i];
	}
	delete database;
}

void Game::ProcessLoadRequest()
{
	if ( loadRequested == 0 )	// continue
	{
		std::string path = GameSavePath();
		TiXmlDocument doc;
		doc.LoadFile( path.c_str() );
		GLASSERT( !doc.Error() );
		if ( !doc.Error() ) {
			Load( doc );
			loadCompleted = true;
		}
	}
	else if ( loadRequested == 1 )	// new game
	{
		Load( newGameXML );
		loadCompleted = true;
	}
	else if ( loadRequested == 2 )	// test
	{
		const gamedb::Item* item = database->Root()->Child( "data" )->Child( "testgame" );
		GLASSERT( item );

		TiXmlDocument doc;
		// pull the default game from the resource
		const char* testGame = (const char*)database->AccessData( item, "binary" );
		doc.Parse( testGame );

		if ( !doc.Error() ) {
			Load( doc );
			loadCompleted = true;
		}
	}
	loadRequested = -1;
}


void Game::PushScene( int id, void* data ) 
{
	GLASSERT( scenePushQueued == -1 );
	scenePushQueued = id;
	scenePushQueuedData = data;
}


void Game::PopScene()
{
	GLASSERT( scenePopQueued == false );
	scenePopQueued = true;
}

void Game::PushPopScene() 
{
	if ( scenePopQueued ) {
		GLASSERT( !sceneStack.Empty() );

		sceneStack.Top()->DeActivate();
		delete sceneStack.Top();
		sceneStack.Pop();
		GLASSERT( scenePushQueued>=0 || !sceneStack.Empty() );
		if ( sceneStack.Size() ) {
			sceneStack.Top()->Activate();
		}
	}
	if ( scenePushQueued >= 0 ) {
		GLASSERT( scenePushQueued < NUM_SCENES );

		if ( sceneStack.Size() ) {
			sceneStack.Top()->DeActivate();
		}
		Scene* scene = CreateScene( scenePushQueued, scenePushQueuedData );
		sceneStack.Push( scene );
		scene->Activate();
		if ( loadRequested >= 0 ) {
			ProcessLoadRequest();
		}
	}
	scenePushQueued = -1;
	scenePopQueued = false;
}


Scene* Game::CreateScene( int id, void* data )
{
	Scene* scene = 0;
	switch ( id ) {
		case BATTLE_SCENE:		scene = new BattleScene( this );											break;
		case CHARACTER_SCENE:	scene = new CharacterScene( this, (CharacterSceneInput*)data );				break;
		case INTRO_SCENE:		scene = new TacticalIntroScene( this );										break;
		case END_SCENE:			scene = new TacticalEndScene( this, (const TacticalEndSceneData*) data );	break;
		case HELP_SCENE:		scene = new HelpScene( this );												break;
		default:
			GLASSERT( 0 );
			break;
	}
	return scene;
}


void Game::Load( const TiXmlDocument& doc )
{
	// Already pushed the BattleScene. Note that the
	// BOTTOM of the stack saves and loads. (BattleScene or GeoScene).
	const TiXmlElement* game = doc.RootElement();
	GLASSERT( StrEqual( game->Value(), "Game" ) );
	sceneStack.Bottom()->Load( game );
}


void Game::Save( TiXmlDocument* doc )
{
	TiXmlElement sceneElement( "Scene" );
	sceneElement.SetAttribute( "id", 0 );

	TiXmlElement gameElement( "Game" );
	gameElement.InsertEndChild( sceneElement );

	doc->InsertEndChild( gameElement );
	sceneStack.Bottom()->Save( doc->RootElement() );
}


void Game::DoTick( U32 _currentTime )
{
	currentTime = _currentTime;
	if ( previousTime == 0 ) {
		previousTime = currentTime-1;
	}
	U32 deltaTime = currentTime - previousTime;

	if ( markFrameTime == 0 ) {
		markFrameTime			= currentTime;
		frameCountsSinceMark	= 0;
		framesPerSecond			= 0.0f;
		trianglesPerSecond		= 0;
		trianglesSinceMark		= 0;
	}
	else {
		++frameCountsSinceMark;
		if ( currentTime - markFrameTime > 500 ) {
			framesPerSecond		= 1000.0f*(float)(frameCountsSinceMark) / ((float)(currentTime - markFrameTime));
			// actually K-tris/second
			trianglesPerSecond  = trianglesSinceMark / (currentTime - markFrameTime);
			markFrameTime		= currentTime;
			frameCountsSinceMark = 0;
			trianglesSinceMark = 0;
		}
	}

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glDisable( GL_SCISSOR_TEST );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );

	Scene* scene = sceneStack.Top();
	scene->DoTick( currentTime, deltaTime );

	Rectangle2I clip2D, clip3D;
	int renderPass = scene->RenderPass( &clip3D, &clip2D );
	GLASSERT( renderPass );
	if ( !clip3D.IsValid() )
		screenport.UIBoundsClipped3D( &clip3D );
	if ( !clip2D.IsValid() )
		screenport.UIBoundsClipped2D( &clip2D );
	
	if ( renderPass & Scene::RENDER_3D ) {
		//	r.Set( 100, 50, 300, 50+200*320/480 );
		//	r.Set( 100, 50, 300, 150 );
		screenport.SetPerspective( 2.f, 240.f, 20.f, &clip3D );

#ifdef MAPMAKER
		if ( showPathing ) 
			engine.EnableMap( false );
		engine.Draw();
		if ( showPathing ) {
			engine.GetMap()->DrawPath();
		}
		if ( showPathing ) 
			engine.EnableMap( true );
#else
		engine.Draw();
#endif
		
		const grinliz::Vector3F* eyeDir = engine.camera.EyeDir3();
		ParticleSystem* particleSystem = ParticleSystem::Instance();
		particleSystem->Update( deltaTime, currentTime );
		particleSystem->Draw( eyeDir, &engine.GetMap()->GetFogOfWar() );
	}
	trianglesSinceMark += trianglesRendered;

	if ( renderPass & Scene::RENDER_2D ) {
		screenport.SetUI( &clip2D );
		scene->DrawHUD();
	}
#ifdef DEBUG
	UFOText::Draw(	0,  0, "UFO#%d %5.1ffps %4.1fK/f %3ddc/f %4dK/s %dnew", 
					VERSION,
					framesPerSecond, 
					(float)trianglesRendered/1000.0f,
					drawCalls,
					trianglesPerSecond,
					memNewCount );
#else
	UFOText::Draw(	0,  0, "UFO#%d %5.1ffps %4.1fK/f %3ddc/f %4dK/s", 
				  VERSION,
				  framesPerSecond, 
				  (float)trianglesRendered/1000.0f,
				  drawCalls,
				  trianglesPerSecond );
#endif
	trianglesRendered = 0;
	drawCalls = 0;

#ifdef EL_SHOW_MODELS
	int k=0;
	while ( k < nModelResource ) {
		int total = 0;
		for( unsigned i=0; i<modelResource[k].nGroups; ++i ) {
			total += modelResource[k].atom[i].trisRendered;
		}
		UFODrawText( 0, 12+12*k, "%16s %5d K", modelResource[k].name, total );
		++k;
	}
#endif

#ifdef GRINLIZ_PROFILE
	const int SAMPLE = 8;
	if ( (currentFrame & (SAMPLE-1)) == 0 ) {
		memcpy( &profile, &Performance::GetData( false ), sizeof( ProfileData ) );
		Performance::Clear();
	}
	for( unsigned i=0; i<profile.count; ++i ) {
		UFOText::Draw( 0, 20+i*12, "%20s %6.1f %4d", 
					  profile.item[i].name, 
					  100.0f*profile.NormalTime( profile.item[i].functionTime ),
					  profile.item[i].functionCalls/SAMPLE );
	}
#endif

	glEnable( GL_DEPTH_TEST );
	previousTime = currentTime;
	++currentFrame;

	PushPopScene();
}


void Game::Tap( int tap, int sx, int sy )
{
	Vector2I view;
	grinliz::Ray world;

	screenport.ScreenToView( sx, sy, &view );
	screenport.ScreenToWorld( sx, sy, &world );

	sceneStack.Top()->Tap( tap, view, world );
}


void Game::Drag( int action, int sx, int sy )
{
	Vector2I view;
	grinliz::Ray world;

	screenport.ScreenToView( sx, sy, &view );
	screenport.ScreenToWorld( sx, sy, &world );

	switch ( action ) 
	{
		case GAME_DRAG_START:
		{
			GLASSERT( !isDragging );
			isDragging = true;
			sceneStack.Top()->Drag( action, view );
		}
		break;

		case GAME_DRAG_MOVE:
		{
			GLASSERT( isDragging );
			sceneStack.Top()->Drag( action, view );
		}
		break;

		case GAME_DRAG_END:
		{
			GLASSERT( isDragging );
			sceneStack.Top()->Drag( GAME_DRAG_MOVE, view );
			sceneStack.Top()->Drag( GAME_DRAG_END, view );
			isDragging = false;
		}
		break;

		default:
			GLASSERT( 0 );
			break;
	}
}


void Game::Zoom( int action, int distance )
{
	sceneStack.Top()->Zoom( action, distance );
}


void Game::Rotate( int action, float degrees )
{
	sceneStack.Top()->Rotate( action, degrees );
}


void Game::CancelInput()
{
	isDragging = false;
}


void Game::HandleHotKeyMask( int mask )
{
	sceneStack.Top()->HandleHotKeyMask( mask );
}


void Game::MouseMove( int sx, int sy )
{
#ifdef MAPMAKER
//	Vector2I view;
//	screenport.ScreenToView( sx, sy, &view );

//	grinliz::Matrix4 mvpi;
//	grinliz::Ray world;

//	screenport.ViewProjectionInverse3D( &mvpi );
//	engine.RayFromScreenToYPlane( sx, sy, mvpi, &world, 0 );

//	GLOUTPUT(( "world (%.1f,%.1f,%.1f)  plane (%.1f,%.1f,%.1f)\n", 
//				world.origin.x, world.origin.y, world.origin.z,
//				p.x, p.y, p.z ));

	// Can only be battlescene:
	((BattleScene*)sceneStack.Top())->MouseMove( sx, sy );
#endif
}


#ifdef MAPMAKER

void Game::RotateSelection( int delta )
{
	((BattleScene*)sceneStack.Top())->RotateSelection( delta );
}

void Game::DeleteAtSelection()
{
	((BattleScene*)sceneStack.Top())->DeleteAtSelection();
}


void Game::DeltaCurrentMapItem( int d )
{
	((BattleScene*)sceneStack.Top())->DeltaCurrentMapItem(d);
}

#endif
