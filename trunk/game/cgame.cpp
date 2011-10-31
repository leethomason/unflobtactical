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

#include "../grinliz/gldebug.h"
#include "cgame.h"
#include "game.h"

#if defined( UFO_WIN32_SDL )
static const char* winResourcePath = "./res/uforesource.db";
#endif

#ifdef UFO_IPHONE
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef ANDROID_NDK
extern "C" int androidResourceOffset;
extern "C" int androidResourceLen;
extern "C" char androidResourcePath[200];
#endif

#include "../grinliz/glstringutil.h"


class CheckThread
{
public:
	CheckThread()	{ GLASSERT( active == false ); active = true; }
	~CheckThread()	{ GLASSERT( active == true ); active = false; }
private:
	static bool active;
};

bool CheckThread::active = false;

static char* modDatabases[ GAME_MAX_MOD_DATABASES ] = { 0 };


void* NewGame( int width, int height, int rotation, const char* path )
{
	CheckThread check;

	Game* game = new Game( width, height, rotation, path );
	GLOUTPUT(( "NewGame.\n" ));

	for( int i=0; i<GAME_MAX_MOD_DATABASES; ++i ) {
		if ( modDatabases[i] ) {
			//GLOUTPUT(( "adding database '%s'\n", modDatabases[i] ));
			game->AddDatabase( modDatabases[i] );
		}
	}

	return game;
}


void DeleteGame( void* handle )
{
	CheckThread check;

	GLOUTPUT(( "DeleteGame. handle=%x\n", handle ));
	if ( handle ) {
		Game* game = (Game*)handle;
		delete game;
	}
	for( int i=0; i<GAME_MAX_MOD_DATABASES; ++i ) {
		if ( modDatabases[i] ) {
			free( modDatabases[i] );
			modDatabases[i] = 0;
		}
	}
}



void GameResize( void* handle, int width, int height, int rotation ) {
	CheckThread check;

	GLOUTPUT(( "GameResize. handle=%x\n", handle ));
	Game* game = (Game*)handle;
	game->Resize( width, height, rotation );
}


void GameSave( void* handle ) {
	CheckThread check;

	GLOUTPUT(( "GameSave. handle=%x\n", handle ));
	Game* game = (Game*)handle;
	game->Save( 0, false );
}


void GameDeviceLoss( void* handle )
{
	CheckThread check;

	GLOUTPUT(( "GameDeviceLoss. handle=%x\n", handle ));
	Game* game = (Game*)handle;
	game->DeviceLoss();
}


void GameZoom( void* handle, int style, float delta )
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->Zoom( style, delta );
}


void GameCameraRotate( void* handle, float degrees )
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->Rotate( degrees );
}


void GameTap( void* handle, int action, int x, int y )
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->Tap( action, x, y );
}


void GameDoTick( void* handle, unsigned int timeInMSec )
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->DoTick( timeInMSec );
}


void GameCameraGet( void* handle, int param, float* value ) 
{
	CheckThread check;

	Game* game = (Game*)handle;
	switch( param ) {
		case GAME_CAMERA_TILT:
			*value = game->engine->camera.GetTilt();
			break;
		case GAME_CAMERA_YROTATE:
			*value = game->engine->camera.GetYRotation();
			break;
		case GAME_CAMERA_ZOOM:
			*value = game->engine->GetZoom();
			break;
		default:
			GLASSERT( 0 );
	}
}


void GameMoveCamera( void* handle, float dx, float dy, float dz )
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->engine->camera.DeltaPosWC( dx, dy, dz );
}


void GameHotKey( void* handle, int mask )
{
	CheckThread check;

	Game* game = (Game*)handle;
	return game->HandleHotKeyMask( mask );
}



void PlatformPathToResource( char* buffer, int bufferLen, int* offset, int* length )
{
#if defined( UFO_IPHONE )
	CFStringRef nameRef = CFStringCreateWithCString( 0, name, kCFStringEncodingWindowsLatin1 );
	CFStringRef extensionRef = CFStringCreateWithCString( 0, extension, kCFStringEncodingWindowsLatin1 );
	
	CFBundleRef mainBundle = CFBundleGetMainBundle();	
	CFURLRef imageURL = CFBundleCopyResourceURL( mainBundle, nameRef, extensionRef, NULL );
	if ( !imageURL ) {
		GLOUTPUT(( "Error loading '%s' '%s'\n", name, extension ));
	}
	GLASSERT( imageURL );
		
	CFURLGetFileSystemRepresentation( imageURL, true, (unsigned char*)buffer, bufferLen );
#elif defined( UFO_WIN32_SDL )
	grinliz::StrNCpy( buffer, winResourcePath, bufferLen );
	*offset = 0;
	*length = 0;
#elif defined (ANDROID_NDK)
	grinliz::StrNCpy( buffer, androidResourcePath, bufferLen );
	*offset = androidResourceOffset;
	*length = androidResourceLen;
#else
#	error UNDEFINED
#endif
}


const char* PlatformName()
{
#if defined( UFO_WIN32_SDL )
	return "pc";
#elif defined (ANDROID_NDK)
	return "android";
#else
#	error UNDEFINED
#endif
}


void GameAddDatabase( const char* path )
{
	for( int i=0; i<GAME_MAX_MOD_DATABASES; ++i ) {
		if ( modDatabases[i] == 0 ) {
			int len = strlen( path ) + 1;
			modDatabases[i] = (char*)malloc( len );
			strcpy( modDatabases[i], path );
			break;
		}
	}
}


int GamePopSound( void* handle, int* database, int* offset, int* size )
{
	CheckThread check;

	Game* game = (Game*)handle;
	bool result = game->PopSound( database, offset, size );	
	return (result) ? 1 : 0;
}


/*
void PlayWAVSound( int offset, int nBytes )
{
	//GLOUTPUT(( "Wav sound called.\n" ));
#if defined( UFO_WIN32_SDL )
	extern void Audio_PlayWav( const char* path, int offset, int size );

	Audio_PlayWav( winResourcePath, offset, nBytes );

#elif defined (ANDROID_NDK)
	// do nothing for now.
#else
#	error UNDEFINED
#endif
}
*/