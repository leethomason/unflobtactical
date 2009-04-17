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

using namespace grinliz;




Game::Game( int width, int height ) :
	engine( width, height, engineData ),
	rotation( 0 ),
	nTexture( 0 ),
	nModelResource( 0 ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 ),
	trianglesPerSecond( 0 ),
	trianglesSinceMark( 0 ),
	previousTime( 0 ),
	isDragging( false ),
	nSceneStack( 0 )
{
	memset( memStream, 0, sizeof(MemStream)*MAX_STREAMS );
	surface.Set( 256, 256, 4 );		// All the memory we will ever need (? or that is the intention)

	LoadTextures();
	modelLoader = new ModelLoader( texture, nTexture );
	LoadModels();
	LoadLightMaps();
	LoadMapResources();

	delete modelLoader;
	modelLoader = 0;

	Texture* textTexture = GetTexture( "stdfont2" );
	GLASSERT( textTexture );
	UFOText::InitTexture( textTexture->glID );
	UFOText::InitScreen( engine.Width(), engine.Height(), rotation );

#ifdef MAPMAKER
	showPathing = false;
#else
	// If we aren't the map maker, then we need to load a map.
	LoadMap( "farmland" );
#endif

	engine.GetMap()->SetSize( 40, 40 );
	engine.GetMap()->SetTexture( GetTexture("farmland" ) );
	engine.GetMap()->SetLightMap( &lightMaps[0] );
	
	//engine.camera.SetPosWC( -19.4f, 62.0f, 57.2f );
	engine.camera.SetPosWC( -12.f, 45.f, 52.f );	// standard test
	//engine.camera.SetPosWC( -5.0f, engineData.cameraHeight, mz + 5.0f );

	particleSystem = new ParticleSystem();
	particleSystem->InitPoint( GetTexture( "particleSparkle" ) );
	particleSystem->InitQuad( GetTexture( "particleQuad" ) );

	currentScene = 0;
	scenes[BATTLE_SCENE] = new BattleScene( this );
	scenes[CHARACTER_SCENE] = new CharacterScene( this );

	PushScene( BATTLE_SCENE );
}


Game::~Game()
{
	while ( nSceneStack ) {
		PopScene();
	}
	for( int i=0; i<NUM_SCENES; ++i ) {
		delete scenes[i];
	}

	delete particleSystem;
	FreeModels();
	FreeTextures();

	for( int i=0; i<MAX_STREAMS; ++i ) {
		if ( memStream[i].stream ) {
			memStream[i].stream->Dump( memStream[i].name );
			delete memStream[i].stream;
		}
	}
}


UFOStream* Game::OpenStream( const char* name, bool create )
{
	GLASSERT( strlen( name ) < EL_FILE_STRING_LEN );
	int i=0;
	for( ; i<MAX_STREAMS; ++i ) {
		if ( memStream[i].stream == 0 )
			break;
		if ( strcmp( name, memStream[i].name ) == 0 ) {
			GLASSERT( memStream[i].stream );
			memStream[i].stream->SeekSet( 0 );
			return memStream[i].stream;
		}
	}
	if ( create ) {
		GLASSERT( i < MAX_STREAMS );
		strcpy( memStream[i].name, name );
		memStream[i].stream = new UFOStream();
		return memStream[i].stream;
	}
	return 0;
}


void Game::PushScene( int id ) 
{
	GLASSERT( id >= 0 && id < NUM_SCENES );
	GLASSERT( nSceneStack < MAX_SCENE_STACK );
	if ( nSceneStack > 0 ) {
		sceneStack[nSceneStack-1]->DeActivate();
	}
	sceneStack[nSceneStack] = scenes[id];
	sceneStack[nSceneStack]->Activate();
	currentScene = sceneStack[nSceneStack];
	nSceneStack++;
}


void Game::PopScene()
{
	GLASSERT( nSceneStack > 0 );
	nSceneStack--;
	sceneStack[nSceneStack]->DeActivate();
	currentScene = 0;

	if ( nSceneStack > 0 ) {
		sceneStack[nSceneStack-1]->Activate();
		currentScene = sceneStack[nSceneStack-1];
	}
}


void Game::SetScreenRotation( int value ) 
{
	rotation = ((unsigned)value)%4;

	UFOText::InitScreen( engine.Width(), engine.Height(), rotation );
	engine.camera.SetViewRotation( rotation );
}


void Game::LoadMap( const char* name )
{
	char buffer[512];

	PlatformPathToResource( name, "map", buffer, 512 );
	FILE* fp = fopen( buffer, "rb" );
	GLASSERT( fp );

	LoadMap( fp );
	fclose( fp );
}


void Game::FreeTextures()
{
	for( int i=0; i<nTexture; ++i ) {
		if ( texture[i].glID ) {
			glDeleteTextures( 1, (const GLuint*) &texture[i].glID );
			texture[i].name[0] = 0;
		}
	}
}


void Game::FreeModels()
{
	for( int i=0; i<nModelResource; ++i ) {
		for( U32 j=0; j<modelResource[i].header.nGroups; ++j ) {
			glDeleteBuffers( 1, (const GLuint*) &modelResource[i].atom[i].indexID );		
		}
		glDeleteBuffers( 1, (const GLuint*) &modelResource[i].atom[i].vertexID );
	}
}


void Game::DoTick( U32 currentTime )
{
	GLASSERT( currentTime > 0 );
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
	particleSystem->Update( deltaTime );
	particleSystem->Draw( eyeDir );

	trianglesSinceMark += triCount;

	currentScene->DrawHUD();

	UFOText::Draw(	0,  0, "UFO Attack! %4.1ffps %5.1fK/f %4dK/s", 
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

	glEnable( GL_DEPTH_TEST );
	previousTime = currentTime;
}


void Game::TransformScreen( int x0, int y0, int *x1, int *y1 )
{
	switch ( rotation ) {
		case 0:
			*x1 = x0;
			*y1 = y0;
			break;

		case 1:
			*x1 = y0;
			*y1 = engine.Width() - 1 - x0;
			break;

		case 2:
			*x1 = engine.Width() - 1 - x0;
			*y1 = y0;
			break;

		case 3:
			*x1 = engine.Height() - 1 - y0;
			*y1 = x0;
			break;

		default:
			GLASSERT( 0 );
			break;
	}
}


void Game::Tap( int tap, int x, int y )
{
	grinliz::Matrix4 mvpi;
	grinliz::Ray world;

	grinliz::Vector2I screen;
	TransformScreen( x, y, &screen.x, &screen.y );

	engine.CalcModelViewProjectionInverse( &mvpi );
	engine.RayFromScreen( x, y, mvpi, &world );

	currentScene->Tap( tap, screen, world );
}


void Game::Drag( int action, int x, int y )
{
	Vector2I screenRaw = { x, y };

	switch ( action ) 
	{
		case GAME_DRAG_START:
		{
			GLASSERT( !isDragging );
			isDragging = true;
			currentScene->Drag( action, screenRaw );
		}
		break;

		case GAME_DRAG_MOVE:
		{
			GLASSERT( isDragging );
			currentScene->Drag( action, screenRaw );
		}
		break;

		case GAME_DRAG_END:
		{
			GLASSERT( isDragging );
			currentScene->Drag( GAME_DRAG_MOVE, screenRaw );
			currentScene->Drag( GAME_DRAG_END, screenRaw );
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


void Game::CancelInput()
{
	isDragging = false;
}


#ifdef MAPMAKER

void Game::MouseMove( int x, int y )
{
	if ( currentScene == scenes[BATTLE_SCENE] ) {
		((BattleScene*)currentScene)->MouseMove( x, y );
	}
}

void Game::RotateSelection( int delta )
{
	if ( currentScene == scenes[BATTLE_SCENE] ) {
		((BattleScene*)currentScene)->RotateSelection( delta );
	}
}

void Game::DeleteAtSelection()
{
	if ( currentScene == scenes[BATTLE_SCENE] ) {
		((BattleScene*)currentScene)->DeleteAtSelection();
	}
}


void Game::DeltaCurrentMapItem( int d )
{
	if ( currentScene == scenes[BATTLE_SCENE] ) {
		((BattleScene*)currentScene)->DeltaCurrentMapItem(d);
	}
}

#endif
