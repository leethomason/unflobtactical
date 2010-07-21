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
void DeleteGame( void* handle );	// does not save! use GameSave if needed.

void GameDeviceLoss( void* handle );
void GameResize( void* handle, int width, int height, int rotation );
void GameSave( void* handle );

// Input
// Mimics the iPhone input. UFOAttack procesess:
//		Touch and drag. (scrolling, movement)
//		2 finger zoom in/out
//		Taps (buttons, UI)
//

// Coordinates.
// All coordinates (x,y) are in the iphone coordinate space.
// (see screenport.h). Rotation has no impact on the coordinates.

#define GAME_TAP_DOWN	0
#define GAME_TAP_MOVE	1
#define GAME_TAP_UP		2
#define GAME_TAP_CANCEL	3
void GameTap( void* handle, int action, int x, int y );

#define GAME_ZOOM_START	0
#define GAME_ZOOM_MOVE	1
void GameZoom( void* handle, int action, int distance );

#define GAME_ROTATE_START 0
#define GAME_ROTATE_MOVE  1
void GameCameraRotate( void* handle, int action, float degreesFromStart );

void GameDoTick( void* handle, unsigned int timeInMSec );

#define GAME_HK_NEXT_UNIT				0x0001
#define GAME_HK_PREV_UNIT				0x0002
#define GAME_HK_ROTATE_CCW				0x0004
#define GAME_HK_ROTATE_CW				0x0008
#define GAME_HK_TOGGLE_ROTATION_UI		0x0010
#define GAME_HK_TOGGLE_NEXT_UI			0x0020
#define GAME_HK_TOGGLE_DEBUG_TEXT		0x0040
void GameHotKey( void* handle, int mask );

// --- Core to platform --- //
void PlatformPathToResource( char* buffer, int bufferLen, int* offset, int* length );
void PlayWAVSound( const void* wavFile, int nBytes );

// ----------------------------------------------------------------
// Debugging and adjustment
enum {
	GAME_CAMERA_TILT,
	GAME_CAMERA_YROTATE,
	GAME_CAMERA_ZOOM
};
void GameCameraGet( void* handle, int param, float* value );
void GameCameraSet( void* handle, int param, float value );

void GameMoveCamera( void* handle, float dx, float dy, float dz );
	
#ifdef __cplusplus
}
#endif

#endif	// GAME_ADAPTOR_INCLUDED
