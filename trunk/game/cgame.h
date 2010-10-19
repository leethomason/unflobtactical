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


#define GAME_TAP_DOWN		0
#define GAME_TAP_MOVE		1
#define GAME_TAP_UP			2
#define GAME_TAP_CANCEL		3

#define GAME_TAP_MASK		0x00ff
#define GAME_TAP_PANNING	0x0100
#define GAME_TAP_DOWN_PANNING		(GAME_TAP_DOWN | GAME_TAP_PANNING)
#define GAME_TAP_MOVE_PANNING		(GAME_TAP_MOVE | GAME_TAP_PANNING)
#define GAME_TAP_UP_PANNING			(GAME_TAP_UP | GAME_TAP_PANNING)
#define GAME_TAP_CANCEL_PANNING		(GAME_TAP_CANCEL | GAME_TAP_PANNING)

void GameTap( void* handle, int action, int x, int y );

// Set the zoom. The zoom is in a range of 0.1-5.0, based on
// distance from a hypothetical bitmap. Zoom is passed in
// as a relative value.
#define GAME_ZOOM_MIN	0.1f
#define GAME_ZOOM_MAX	5.0f
void GameZoom( void* handle, float zoom );

// Relative rotation, in degrees.
void GameCameraRotate( void* handle, float degrees );

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
const char* PlatformName();
void PlayWAVSound( const void* wavFile, int nBytes );

// ----------------------------------------------------------------
// Debugging and adjustment
enum {
	GAME_CAMERA_TILT,
	GAME_CAMERA_YROTATE,
	GAME_CAMERA_ZOOM
};
void GameCameraGet( void* handle, int param, float* value );
//void GameCameraSet( void* handle, int param, float value );

void GameMoveCamera( void* handle, float dx, float dy, float dz );
	
#ifdef __cplusplus
}
#endif

#endif	// GAME_ADAPTOR_INCLUDED
