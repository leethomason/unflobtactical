#include "../grinliz/gldebug.h"
#include "cgame.h"
#include "game.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

void* NewGame( int width, int height )
{
	Game* game = new Game( width, height );
	return game;
}

void DeleteGame( void* handle )
{
	if ( handle ) {
		Game* game = (Game*)handle;
		delete game;
	}
}

void GameDragStart( void* handle, int x, int y )
{
	Game* game = (Game*)handle;
	game->engine.DragStart( x, y );
}

void GameDragMove( void* handle, int x, int y )
{
	Game* game = (Game*)handle;
	game->engine.DragMove( x, y );

}

void GameDragEnd( void* handle, int x, int y )
{
	Game* game = (Game*)handle;
	game->engine.DragEnd( x, y );
}

void GameDoTick( void* handle, unsigned int timeInMSec )
{
	Game* game = (Game*)handle;
	game->DoTick( timeInMSec );
}

void GameTiltCamera( void* handle, float degrees )
{
	Game* game = (Game*)handle;
	game->engine.camera.DeltaTilt( degrees );
}

void GameMoveCamera( void* handle, float dx, float dy, float dz )
{
	Game* game = (Game*)handle;
	game->engine.camera.DeltaPosWC( dx, dy, dz );
}

void GameAdjustPerspective( void* handle, float dFOV )
{
	Game* game = (Game*)handle;
	game->engine.fov += dFOV;
	game->engine.SetPerspective();
}

void GameRotate( void* handle, int rotation )
{
 	Game* game = (Game*)handle;
	game->SetRotation( rotation );
}


void PlatformPathToResource( const char* name, const char* extension, char* buffer, int bufferLen )
{
#ifdef __APPLE__
#ifdef DEBUG
	int debug = 1;
#endif
	CFStringRef nameRef = CFStringCreateWithCString( 0, name, kCFStringEncodingWindowsLatin1 );
	CFStringRef extensionRef = CFStringCreateWithCString( 0, extension, kCFStringEncodingWindowsLatin1 );
	
	CFBundleRef mainBundle = CFBundleGetMainBundle();	
	CFURLRef imageURL = CFBundleCopyResourceURL( mainBundle, nameRef, extensionRef, NULL );
	GLASSERT( imageURL );
		
	CFURLGetFileSystemRepresentation( imageURL, true, (unsigned char*)buffer, bufferLen );
#else
	std::string fullname = "./res/";
	fullname += name;
	fullname += ".";
	fullname += extension;
	strncpy( buffer, fullname.c_str(), bufferLen );
#endif
}

