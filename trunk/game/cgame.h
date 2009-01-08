#ifndef GAME_ADAPTOR_INCLUDED
#define GAME_ADAPTOR_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


// --- Platform to Core --- //
void* NewGame( int width, int height );
void DeleteGame( void* handle );

// Input
// Mimics the iPhone input. UFOAttack procesess:
//		Touch and drag. (scrolling, movement)
//		2 finger zoom in/out
//		Taps (buttons, UI)
//

#define GAME_DRAG_START	0
#define GAME_DRAG_MOVE	1
#define GAME_DRAG_END	2
void GameDrag( void* handle, int action, int x, int y );

#define GAME_ZOOM_START	0
#define GAME_ZOOM_MOVE	1
//#define GAME_ZOOM_END	2
void GameZoom( void* handle, int action, int distance );

// count is 1 or 2
void GameTap( void* handle, int count, int x, int y );

void GameInputCancelled( void* handle );	


// Screen rotation.
void GameRotate( void* handle, int rotation );

void GameDoTick( void* handle, unsigned int timeInMSec );


// Debugging and adjustment
enum {
	GAME_CAMERA_TILT,
	GAME_CAMERA_YROTATE,
	GAME_CAMERA_ZOOM
};
void GameCameraGet( void* handle, int param, float* value );
void GameCameraSet( void* handle, int param, float value );

void GameMoveCamera( void* handle, float dx, float dy, float dz );
void GameAdjustPerspective( void* handle, float dFOV );
void GameShadowMode( void* handle );
	
// --- Core to platform --- //
void PlatformPathToResource( const char* name, const char* extension, char* buffer, int bufferLen );


#ifdef __cplusplus
}
#endif

#endif	// GAME_ADAPTOR_INCLUDED
