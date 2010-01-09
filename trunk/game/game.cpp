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

#include "../engine/platformgl.h"
#include "../engine/text.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"
#include "../engine/particle.h"

#include "../grinliz/glmatrix.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glperformance.h"
#include "../sqlite3/sqlite3.h"
#include "../shared/gldatabase.h"
#include "../version.h"

using namespace grinliz;

Game::Game( const Screenport& sp, const char* _savePath ) :
	engine( sp, engineData ),
	screenport( sp ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 ),
	trianglesPerSecond( 0 ),
	trianglesSinceMark( 0 ),
	previousTime( 0 ),
	isDragging( false ),
	currentScene( 0 ),
	sceneStack(),
	scenePopQueued( false ),
	scenePushQueued( -1 )
{
	savePath = _savePath;
	char c = savePath[savePath.size()-1];
	if ( c != '\\' && c != '/' ) {	
#ifdef WIN32
		savePath += '\\';
#else
		savePath += '/';
#endif
	}	
	
	rootStream = 0;
	currentFrame = 0;
	memset( &profile, 0, sizeof( ProfileData ) );
	surface.Set( Surface::RGBA16, 256, 256 );		// All the memory we will ever need (? or that is the intention)

	// Load the database.
	char buffer[260];
	PlatformPathToResource( "uforesource", "db", buffer, 260 );
	int sqlResult = sqlite3_open_v2( buffer, &database, SQLITE_OPEN_READONLY, 0 );
	GLASSERT( sqlResult == SQLITE_OK );
	(void) sqlResult;

	LoadTextures();
	modelLoader = new ModelLoader();
	LoadModels();
	LoadLightMaps();
	LoadMapResources();
	LoadItemResources();

	delete modelLoader;
	modelLoader = 0;

	const Texture* textTexture = TextureManager::Instance()->GetTexture( "stdfont2" );
	GLASSERT( textTexture );
	UFOText::InitTexture( textTexture->glID );
	UFOText::InitScreen( engine.GetScreenport() );
	engine.GetMap()->SetLightObjects( GetLightMap( "objectLightMaps" ) );

#ifdef MAPMAKER
	showPathing = false;
#else
	// If we aren't the map maker, then we need to load a map.
	// For now, always create a new one.
	sqlite3* mapDB = Map::CreateMap( savePath, database );
	engine.GetMap()->SyncToDB( mapDB, "farmland" );


#endif

	engine.GetMap()->SetSize( 40, 40 );
	engine.GetMap()->SetTexture( TextureManager::Instance()->GetTexture("farmland" ) );
	engine.SetDayNight( true, 0 );

	engine.camera.SetPosWC( -12.f, 45.f, 52.f );	// standard test

	currentScene = 0;
	scenePushQueued = BATTLE_SCENE;
	PushPopScene();
}


Game::~Game()
{
	delete currentScene;
	currentScene = 0;

	while( rootStream ) {
		UFOStream* temp = rootStream;
		rootStream = rootStream->next;
		delete temp;
	}
	for( unsigned i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		delete itemDefArr[i];
	}
	sqlite3_close(database);
	database = 0;
}


UFOStream* Game::FindStream( const char* name )
{
	GLASSERT( strlen( name ) < EL_FILE_STRING_LEN );

	UFOStream* s = rootStream;

	while ( s ) {
		if ( strcmp( s->Name(), name ) == 0 ) {
			return s;
		}
		s = s->next;
	}
	return 0;
}


UFOStream* Game::OpenStream( const char* name, bool create )
{
	UFOStream* s = FindStream( name );
	if ( !s && create ) {
		s = new UFOStream( name );
		s->next = rootStream;
		rootStream = s;
	}
	if ( s ) {
		s->SeekSet( 0 );
	}
	return s;
}


void Game::PushScene( int id ) 
{
	GLASSERT( scenePushQueued == -1 );
	scenePushQueued = id;
}


void Game::PopScene()
{
	GLASSERT( scenePopQueued == false );
	scenePopQueued = true;
}

void Game::PushPopScene() 
{
	if ( scenePushQueued >= 0 ) {
		GLASSERT( scenePushQueued < NUM_SCENES );

		if ( currentScene ) {
			GLASSERT( !sceneStack.Empty() );
			delete currentScene;
			currentScene = 0;
		}
		sceneStack.Push( scenePushQueued );
		CreateScene( scenePushQueued );
	}
	if ( scenePopQueued ) {
		GLASSERT( !sceneStack.Empty() );
		GLASSERT( currentScene );

		delete currentScene;
		currentScene = 0;

		sceneStack.Pop();
		GLASSERT( !sceneStack.Empty() );
		CreateScene( sceneStack.Top() );
	}
	scenePushQueued = -1;
	scenePopQueued = false;
}


void Game::CreateScene( int id )
{
	switch ( id ) {
		case BATTLE_SCENE:		currentScene = new BattleScene( this );			break;
		case CHARACTER_SCENE:	currentScene = new CharacterScene( this );		break;
		default:
			GLASSERT( 0 );
			break;
	}
}


/*
void Game::LoadMap( const char* name )
{
	sqlite3_stmt* stmt = 0;
	sqlite3_prepare_v2(database, "SELECT * FROM map WHERE name=?;", -1, &stmt, 0 );
	sqlite3_bind_text( stmt, 1, name, -1, 0 );

	int id=0;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		id = sqlite3_column_int(  stmt, 1 );
	}
	else {
		GLASSERT( 0 );
	}
	sqlite3_finalize(stmt);

	UFOStream s( "map" );

	int size;
	DBReadBinarySize( database, id, &size );
	U8* mem = s.WriteMem( size );
	DBReadBinaryData( database, id, size, mem );
	s.SeekSet( 0 );

	engine.GetMap()->Load( &s, this );
}
*/

/*
#ifdef MAPMAKER
void Game::SaveMap( const char* name )
{
	UFOStream s( "map" );
	engine.GetMap()->Save( &s );

	FILE* fp = fopen( "currentMap.map", "wb" );
	fwrite( s.MemBase(), 1, s.Size(), fp );
	fclose( fp );
}
#endif
*/

void Game::DoTick( U32 _currentTime )
{
	GLASSERT( currentTime > 0 );
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

#ifdef EL_SHOW_MODELS
	for( int i=0; i<nModelResource; ++i ) {
		for ( unsigned k=0; k<modelResource[i].nGroups; ++ k ) {
			modelResource[i].atom[k].trisRendered = 0;
		}
	}
#endif

	currentScene->DoTick( currentTime, deltaTime );

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	int triCount = 0;

#ifdef MAPMAKER
	if ( showPathing ) 
		engine.EnableMap( false );
	engine.Draw( &triCount );
	if ( showPathing ) {
		engine.GetMap()->DrawPath();
	}
	if ( showPathing ) 
		engine.EnableMap( true );
#else
	engine.Draw( &triCount );
#endif
	
	const grinliz::Vector3F* eyeDir = engine.camera.EyeDir3();
	ParticleSystem* particleSystem = ParticleSystem::Instance();
	particleSystem->Update( deltaTime, currentTime );
	particleSystem->Draw( eyeDir );

	trianglesSinceMark += triCount;

	currentScene->DrawHUD();

	UFOText::Draw(	0,  0, "UFOAttack#d %4.1ffps %5.1fK/f %4dK/s", 
					VERSION,
					framesPerSecond, 
					(float)triCount/1000.0f,
					trianglesPerSecond );

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
	screenport.ScreenToView( sx, sy, &view.x, &view.y );

	grinliz::Matrix4 mvpi;
	grinliz::Ray world;

	engine.CalcModelViewProjectionInverse( &mvpi );
	engine.RayFromScreen( view.x, view.y, mvpi, &world );

	currentScene->Tap( tap, view, world );
}


void Game::Drag( int action, int sx, int sy )
{
	Vector2I view;
	screenport.ScreenToView( sx, sy, &view.x, &view.y );
	//GLOUTPUT(( "View %d,%d\n", view.x, view.y ));

	switch ( action ) 
	{
		case GAME_DRAG_START:
		{
			GLASSERT( !isDragging );
			isDragging = true;
			currentScene->Drag( action, view );
		}
		break;

		case GAME_DRAG_MOVE:
		{
			GLASSERT( isDragging );
			currentScene->Drag( action, view );
		}
		break;

		case GAME_DRAG_END:
		{
			GLASSERT( isDragging );
			currentScene->Drag( GAME_DRAG_MOVE, view );
			currentScene->Drag( GAME_DRAG_END, view );
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
	currentScene->Zoom( action, distance );
}


void Game::Rotate( int action, float degrees )
{
	currentScene->Rotate( action, degrees );
}


void Game::CancelInput()
{
	isDragging = false;
}


void Game::MouseMove( int sx, int sy )
{
	Vector2I view;
	screenport.ScreenToView( sx, sy, &view.x, &view.y );

	grinliz::Matrix4 mvpi;
	grinliz::Ray world;

	engine.CalcModelViewProjectionInverse( &mvpi );
	engine.RayFromScreen( view.x, view.y, mvpi, &world );
	Vector3F p;
	IntersectRayPlane( world.origin, world.direction, 1, 0.0f, &p );

//	GLOUTPUT(( "world (%.1f,%.1f,%.1f)  plane (%.1f,%.1f,%.1f)\n", 
//				world.origin.x, world.origin.y, world.origin.z,
//				p.x, p.y, p.z ));

#ifdef MAPMAKER
	if ( sceneStack.Top() == BATTLE_SCENE ) {
		((BattleScene*)currentScene)->MouseMove( view.x, view.y );
	}
#endif
}

/*
void Game::SetPathToSave( const char* path )
{
	GLASSERT( path && *path );
	std::string str = path;
	savePath = str;
	char c = savePath[savePath.size()-1];
	if ( c != '\\' && c != '/' ) {	
#ifdef WIN32
		savePath += '\\';
#else
		savePath += '/';
#endif
	}
}
*/

#ifdef MAPMAKER

void Game::RotateSelection( int delta )
{
	if ( sceneStack.Top() == BATTLE_SCENE ) {
		((BattleScene*)currentScene)->RotateSelection( delta );
	}
}

void Game::DeleteAtSelection()
{
	if ( sceneStack.Top() == BATTLE_SCENE ) {
		((BattleScene*)currentScene)->DeleteAtSelection();
	}
}


void Game::DeltaCurrentMapItem( int d )
{
	if ( sceneStack.Top() == BATTLE_SCENE ) {
		((BattleScene*)currentScene)->DeltaCurrentMapItem(d);
	}
}

#endif
