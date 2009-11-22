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

#ifndef GAME_ADAPTOR_INCLUDED
#define GAME_ADAPTOR_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


// --- Platform to Core --- //
void* NewGame( int width, int height, int rotation, const char* savePath );
void DeleteGame( void* handle );

// Input
// Mimics the iPhone input. UFOAttack procesess:
//		Touch and drag. (scrolling, movement)
//		2 finger zoom in/out
//		Taps (buttons, UI)
//

// Coordinates.
// All coordinates (x,y) are in the iphone coordinate space.
// (see screenport.h). Rotation has no impact on the coordinates.

#define GAME_DRAG_START	0
#define GAME_DRAG_MOVE	1
#define GAME_DRAG_END	2
void GameDrag( void* handle, int action, int x, int y );

#define GAME_ZOOM_START	0
#define GAME_ZOOM_MOVE	1
//#define GAME_ZOOM_END	2
void GameZoom( void* handle, int action, int distance );

#define GAME_ROTATE_START 0
#define GAME_ROTATE_MOVE  1
void GameCameraRotate( void* handle, int action, float degreesFromStart );

// count is 1 or 2
void GameTap( void* handle, int count, int x, int y );

void GameInputCancelled( void* handle );	


// Screen rotation.
//void GameRotate( void* handle, int rotation );

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
	
void GamePathToSave( void* handle, const char* path );
	
// --- Core to platform --- //
void PlatformPathToResource( const char* name, const char* extension, char* buffer, int bufferLen );


#ifdef __cplusplus
}
#endif

#endif	// GAME_ADAPTOR_INCLUDED
