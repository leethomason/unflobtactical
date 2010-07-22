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

#ifdef UFO_IPHONE
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef ANDROID_NDK
extern "C" int androidResourceOffset;
extern "C" int androidResourceLen;
extern "C" char androidResourcePath[200];
#endif

#include "../grinliz/glstringutil.h"

void* NewGame( int width, int height, int rotation, const char* path )
{
	Game* game = new Game( width, height, rotation, path );
	GLOUTPUT(( "NewGame.\n" ));
	return game;
}


void DeleteGame( void* handle )
{
	if ( handle ) {
		Game* game = (Game*)handle;
		delete game;
	}
}


void GameResize( void* handle, int width, int height, int rotation ) {
	Game* game = (Game*)handle;
	game->Resize( width, height, rotation );
}


void GameSave( void* handle ) {
	Game* game = (Game*)handle;
	game->Save();
}


void GameDeviceLoss( void* handle )
{
	Game* game = (Game*)handle;
	game->DeviceLoss();
}


void GameZoom( void* handle, int action, int distance )
{
	Game* game = (Game*)handle;
	game->Zoom( action, distance );
}

void GameCameraRotate( void* handle, int action, float degrees )
{
	Game* game = (Game*)handle;
	game->Rotate( action, degrees );
}


void GameTap( void* handle, int action, int x, int y )
{
	Game* game = (Game*)handle;
	game->Tap( action, x, y );
}


void GameDoTick( void* handle, unsigned int timeInMSec )
{
	Game* game = (Game*)handle;
	game->DoTick( timeInMSec );
}


void GameCameraGet( void* handle, int param, float* value ) 
{
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


void GameCameraSet( void* handle, int param, float value ) 
{
	Game* game = (Game*)handle;
	switch( param ) {
		case GAME_CAMERA_TILT:
			game->engine->camera.SetTilt( value );
			break;
		case GAME_CAMERA_YROTATE:
			game->engine->camera.SetYRotation( value );
			break;
		case GAME_CAMERA_ZOOM:
			game->engine->SetZoom( value );
			break;
		default:
			GLASSERT( 0 );
	}
}


void GameMoveCamera( void* handle, float dx, float dy, float dz )
{
	Game* game = (Game*)handle;
	game->engine->camera.DeltaPosWC( dx, dy, dz );
}


void GameHotKey( void* handle, int mask )
{
	Game* game = (Game*)handle;
	game->HandleHotKeyMask( mask );
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
	grinliz::StrNCpy( buffer, "./res/uforesource.db", bufferLen );
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




void PlayWAVSound( const void* wavFile, int nBytes )
{
	GLOUTPUT(( "Wav sound called.\n" ));
#if defined( UFO_WIN32_SDL )
	extern void Audio_PlayWav( const void* mem, int size );

	Audio_PlayWav( wavFile, nBytes );
#elif defined (ANDROID_NDK)
	// do nothing for now.
#else
#	error UNDEFINED
#endif
}
