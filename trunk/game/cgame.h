#ifndef GAME_ADAPTOR_INCLUDED
#define GAME_ADAPTOR_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


// --- Platform to Core --- //
void* NewGame( int width, int height );
void DeleteGame( void* handle );

// Input
void GameDragStart( void* handle, int x, int y );
void GameDragMove( void* handle, int x, int y );
void GameDragEnd( void* handle, int x, int y );
void GameRotate( void* handle, int rotation );

void GameDoTick( void* handle, unsigned int timeInMSec );


// Debugging and adjustment
void GameTiltCamera( void* handle, float degrees );
void GameMoveCamera( void* handle, float dx, float dy, float dz );
void GameAdjustPerspective( void* handle, float dFOV );
	
// --- Core to platform --- //
void PlatformPathToResource( const char* name, const char* extension, char* buffer, int bufferLen );


#ifdef __cplusplus
}
#endif

#endif	// GAME_ADAPTOR_INCLUDED
