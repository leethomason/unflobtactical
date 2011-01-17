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



enum HitTestMethod 
{
	TEST_HIT_AABB,
	TEST_TRI,
};


enum {
	EL_MAX_VERTEX_IN_GROUP	= 4096,
	EL_MAX_INDEX_IN_GROUP	= 4096,
	EL_MAX_MODEL_GROUPS		= 8,
	EL_MAX_VERTEX_IN_MODEL	= EL_MAX_VERTEX_IN_GROUP * EL_MAX_MODEL_GROUPS,
	EL_MAX_INDEX_IN_MODEL	= EL_MAX_INDEX_IN_GROUP * EL_MAX_MODEL_GROUPS,
	EL_ALLOCATED_MODELS		= 1024,
	EL_FILE_STRING_LEN		= 24,
	EL_MAX_ITEM_DEFS		= 100,		// can be any size, but a smaller value is less memory scanning
	EL_NIGHT_RED_U8			= 131,
	EL_NIGHT_GREEN_U8		= 125,
	EL_NIGHT_BLUE_U8		= 255,
};

static const float EL_NIGHT_RED		= ( (float)EL_NIGHT_RED_U8/255.f );
static const float EL_NIGHT_GREEN	= ( (float)EL_NIGHT_GREEN_U8/255.f );
static const float EL_NIGHT_BLUE	= ( (float)EL_NIGHT_BLUE_U8/255.f );


//#define EL_SHOW_MODELS
#define EL_USE_VBO
//#define SHOW_FOW			// visual debugging

#endif