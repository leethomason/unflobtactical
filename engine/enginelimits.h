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

#ifndef ENGINELIMITS_INCLUDED
#define ENGINELIMITS_INCLUDED


#define XENOENGINE_OPENGL 2

enum HitTestMethod 
{
	TEST_HIT_AABB,
	TEST_TRI,
};


enum {
	EL_MAX_VERTEX_IN_GROUP	= 4096,
	EL_MAX_INDEX_IN_GROUP	= 4096,
	EL_MAX_MODEL_GROUPS		= 4,
	EL_MAX_VERTEX_IN_MODEL	= EL_MAX_VERTEX_IN_GROUP * EL_MAX_MODEL_GROUPS,
	EL_MAX_INDEX_IN_MODEL	= EL_MAX_INDEX_IN_GROUP * EL_MAX_MODEL_GROUPS,
	EL_ALLOCATED_MODELS		= 1024,
	EL_FILE_STRING_LEN		= 24,
	EL_MAX_ITEM_DEFS		= 100,		// can be any size, but a smaller value is less memory scanning
	EL_NIGHT_RED_U8			= 131,
	EL_NIGHT_GREEN_U8		= 125,
	EL_NIGHT_BLUE_U8		= 255,
	EL_MAP_SIZE				= 64,		// maximum size.
	EL_MAP_MAX_PATH			= 12,		// longest path anything can travel in one turn. Used to limit display memory.
	EL_MAP_TEXTURE_SIZE		= 512
};

static const float EL_NIGHT_RED		= ( (float)EL_NIGHT_RED_U8/255.f );
static const float EL_NIGHT_GREEN	= ( (float)EL_NIGHT_GREEN_U8/255.f );
static const float EL_NIGHT_BLUE	= ( (float)EL_NIGHT_BLUE_U8/255.f );

static const float EL_FOV  = 40.0f;
static const float EL_NEAR = 2.0f;
static const float EL_FAR  = 240.0f;
static const float EL_CAMERA_MIN = 8.0f;
static const float EL_CAMERA_MAX = 140.0f;

static const float EL_LIGHT_X = 0.7f;
static const float EL_LIGHT_Y = 3.0f;
static const float EL_LIGHT_Z = 1.4f;

// Set the zoom. The zoom is in a range of 0.1-5.0, based on
// distance from a hypothetical bitmap. Zoom is passed in
// as a relative value.
static const float GAME_ZOOM_MIN = 0.1f;
static const float GAME_ZOOM_MAX = 5.0f;


// --- Debugging --- //
//#define SHOW_FOW			// visual debugging
//#define EL_SHOW_MODELS
//#define EL_SHOW_ALL_UNITS

// --- Performance -- //
#define EL_USE_VBO			// Use VBOs: a good thing, everywhere but the original iPhone

#endif