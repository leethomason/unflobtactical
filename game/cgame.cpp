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

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "../grinliz/glstringutil.h"

void* NewGame( int width, int height, int rotation, const char* savePath )
{
	Screenport port( width, height, rotation );
	Game* game = new Game( port, savePath );
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

void GameDrag( void* handle, int action, int x, int y )
{
	Game* game = (Game*)handle;
	game->Drag( action, x, y );
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


// count is 1 or 2
void GameTap( void* handle, int count, int x, int y )
{
	Game* game = (Game*)handle;
	game->Tap( count, x, y );
}

void GameInputCancelled( void* handle )
{
	Game* game = (Game*)handle;
	game->CancelInput();
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
			*value = game->engine.camera.GetTilt();
			break;
		case GAME_CAMERA_YROTATE:
			*value = game->engine.camera.GetYRotation();
			break;
		case GAME_CAMERA_ZOOM:
			*value = game->engine.GetZoom();
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
			game->engine.camera.SetTilt( value );
			break;
		case GAME_CAMERA_YROTATE:
			game->engine.camera.SetYRotation( value );
			break;
		case GAME_CAMERA_ZOOM:
			game->engine.SetZoom( value );
			break;
		default:
			GLASSERT( 0 );
	}
}


void GameMoveCamera( void* handle, float dx, float dy, float dz )
{
	Game* game = (Game*)handle;
	game->engine.camera.DeltaPosWC( dx, dy, dz );
}

/*
void GamePathToSave( void* handle, const char* path )
{
	Game* game = (Game*)handle;
	game->SetPathToSave( path );
}
*/

void PlatformPathToResource( const char* name, const char* extension, char* buffer, int bufferLen )
{
#ifdef __APPLE__
	CFStringRef nameRef = CFStringCreateWithCString( 0, name, kCFStringEncodingWindowsLatin1 );
	CFStringRef extensionRef = CFStringCreateWithCString( 0, extension, kCFStringEncodingWindowsLatin1 );
	
	CFBundleRef mainBundle = CFBundleGetMainBundle();	
	CFURLRef imageURL = CFBundleCopyResourceURL( mainBundle, nameRef, extensionRef, NULL );
	if ( !imageURL ) {
		GLOUTPUT(( "Error loading '%s' '%s'\n", name, extension ));
	}
	GLASSERT( imageURL );
		
	CFURLGetFileSystemRepresentation( imageURL, true, (unsigned char*)buffer, bufferLen );
#else
	std::string fullname = "./res/";
	fullname += name;
	fullname += ".";
	fullname += extension;
	StrNCpy( buffer, fullname.c_str(), bufferLen );
#endif
}
