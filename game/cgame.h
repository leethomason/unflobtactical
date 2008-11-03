#ifndef GAME_ADAPTOR_INCLUDED
#define GAME_ADAPTOR_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH	480
#define SCREEN_HEIGHT	320
#define SCREEN_ASPECT   ((float)(SCREEN_WIDTH) / (float)(SCREEN_HEIGHT))


void* NewGame();
void DeleteGame( void* handle );

// Input
void GameDragStart( void* handle, int x, int y );
void GameDragMove( void* handle, int x, int y );
void GameDragEnd( void* handle, int x, int y );

void GameDoTick( void* handle, unsigned int timeInMSec );

// Debugging and adjustment
void GameTiltCamera( void* handle, float degrees );
void GameMoveCamera( void* handle, float dx, float dy, float dz );
void GameAdjustPerspective( void* handle, float dFOV );


#ifdef __cplusplus
}
#endif

#endif	// GAME_ADAPTOR_INCLUDED
