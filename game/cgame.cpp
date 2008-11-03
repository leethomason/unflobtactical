#include "cgame.h"
#include "game.h"

void* NewGame()
{
	Game* game = new Game();
	return game;
}

void DeleteGame( void* _game )
{
	Game* game = (Game*)_game;
	delete game;
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
