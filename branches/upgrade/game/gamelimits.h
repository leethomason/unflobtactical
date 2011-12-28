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

#ifndef GAMELIMITS_INCLUDED
#define GAMELIMITS_INCLUDED

#if 0
        #define AI_LOG GLOUTPUT
#else
        #define AI_LOG( x )      {}
#endif

#include "../engine/enginelimits.h"

#define IMMEDIATE_BUY
#define LANDER_RESCUE

enum SavePathType {
	SAVEPATH_NONE,
	SAVEPATH_GEO,
	SAVEPATH_TACTICAL
};
enum SavePathMode {
	SAVEPATH_READ,
	SAVEPATH_WRITE
};


enum {
	ICON_GREEN_BUTTON		= 0,
	ICON_BLUE_BUTTON		= 1,
	ICON_RED_BUTTON			= 2,

	ICON_AWARD_PURPLE_CIRCLE = 4,
	ICON_AWARD_ALIEN_1		= 5,
	ICON_AWARD_ALIEN_5		= 6,
	ICON_AWARD_ALIEN_10		= 7,

	ICON_GREEN_BUTTON_DOWN	= 8,
	ICON_BLUE_BUTTON_DOWN	= 9,
	ICON_RED_BUTTON_DOWN	= 10,

	ICON_GREEN_STAND_MARK_OUTLINE	= 11,
	ICON_YELLOW_STAND_MARK_OUTLINE	= 12,
	ICON_ORANGE_STAND_MARK_OUTLINE	= 13,
	ICON_BLUE_TAB_DOWN		= 14,
	ICON_BLUE_TAB			= 15,

	ICON_GREEN_STAND_MARK	= 16,
	ICON_YELLOW_STAND_MARK	= 17,
	ICON_ORANGE_STAND_MARK	= 18,


	ICON_TARGET_STAND		= 19,
	ICON_TARGET_POINTER		= 20,
	ICON_STAND_HIGHLIGHT	= 21,
	ICON_ALIEN_TARGETS		= 22,
	ICON_LEVEL_UP			= 23,

	ICON_GREEN_WALK_MARK	= 24,
	ICON_YELLOW_WALK_MARK	= 25,
	ICON_ORANGE_WALK_MARK	= 26,

	ICON_RANK_0				= 27,
	ICON_RANK_1				= 28,
	ICON_RANK_2				= 29,
	ICON_RANK_3				= 30,
	ICON_RANK_4				= 31,

	ICON2_UFO_CROP_CIRCLING		= 0,
	ICON2_UFO_CITY_ATTACKING	= 1,
	ICON2_UFO_OCCUPYING			= 2,
	ICON2_WARNING				= 3,
	ICON2_CAP					= 12,
	ICON2_MIL					= 13,
	ICON2_SCI					= 14,
	ICON2_NAT					= 15,
	ICON2_TEC					= 8,
	ICON2_MAN					= 9,

	DECO_CHARACTER		= 0,
	DECO_PISTOL			= 1,
	DECO_RIFLE			= 2,
	DECO_SHELLS			= 3,
	DECO_ROCKET			= 4,
	DECO_CELL			= 5,
	DECO_RAYGUN			= 6,
	DECO_SAVE_LOAD		= 7,

	DECO_AIMED			= 8,
	DECO_BASE			= 9,
	DECO_RESEARCH		= 10,
	DECO_SETTINGS		= 11,
	DECO_TERRAN_TURN	= 12,
	DECO_ALIEN			= 13,
	DECO_ARMOR			= 14,
	DECO_OKAY_CHECK		= 15,

	DECO_CORE			= 16,
	DECO_BUTTON_PREV	= 17,
	DECO_BUTTON_NEXT	= 18,
	DECO_ORBIT			= 19,
	DECO_HELP			= 20,
	DECO_LAUNCH			= 21,
	DECO_CANCEL			= 22,
	DECO_HUMAN			= 23,

	DECO_UNIT_PREV		= 24,
	DECO_UNIT_NEXT		= 25,
	DECO_ROTATE_CCW		= 26,
	DECO_ROTATE_CW		= 27,
	DECO_SHIELD			= 28,
	DECO_AUDIO			= 29,
	DECO_MUTE			= 30,
	DECO_ALIEN_RIFLE	= 31,

	DECO_NONE			= 32,

	CROP_CIRCLES_X		= 2,
	CROP_CIRCLES_Y		= 2,
};


static const int	GAME_BUTTON_SIZE = 60;
static const float	GAME_BUTTON_SIZE_F = (float)GAME_BUTTON_SIZE;
static const float	GAME_BUTTON_SIZE_B = 50.0f;
static const float	GAME_GUTTER = 20.0f;
static const float	GAME_BUTTON_SPACING = 10.0f;
static const int	SUBTURNS_PER_TURN = 3;
static const float	FIRE_DAMAGE_PER_SUBTURN = 20.0f;
static const int	MAP_SIZE = EL_MAP_SIZE;

static const int TERRAN_TEAM			= 0;
static const int CIV_TEAM				= 1;
static const int ALIEN_TEAM				= 2;
static const int NUM_TEAMS				= 3;

static const int TERRAN_UNITS_START	= 0;
static const int TERRAN_UNITS_END		= 8;
static const int CIV_UNITS_START		= 8;
static const int CIV_UNITS_END			= 24;
static const int ALIEN_UNITS_START		= 24;
static const int ALIEN_UNITS_END		= 44;
static const int MAX_ALIENS				= ALIEN_UNITS_END - ALIEN_UNITS_START;
static const int MAX_TERRANS			= TERRAN_UNITS_END - TERRAN_UNITS_START;
static const int MAX_CIVS				= CIV_UNITS_END - CIV_UNITS_START;
static const int MAX_UNITS				= ALIEN_UNITS_END;
static const int MAX_SCIENTISTS = 8;

static const int MAX_MODELS = 256;
static const float EYE_HEIGHT = 1.2f;
static const float EXPLOSIVE_RANGE = 2.0f;

static const float TU_SNAP_SHOT			= 3.0f;
static const float TU_AUTO_SHOT			= 5.0f;
static const float TU_AIMED_SHOT		= 4.0f;
static const float TU_SECONDARY_SHOT	= 1.4f*4.0f;
static const float TU_PSI				= 3.5f;

static const int SNAP_SHOT			= 0;
static const int AUTO_SHOT			= 1;
static const int AIMED_SHOT		    = 2;

static const int MAX_EYESIGHT_RANGE     = 14;
static const int MIN_TU					= EL_MAP_MAX_PATH / 2;
static const int MAX_TU					= EL_MAP_MAX_PATH;		// enforced: used for memory allocation, since this is the max walking

static const int	TRAIT_MAX			= 100;
static const int	TRAIT_RANK_BONUS	= 6;		// how much a trait increases per rank
static const int	NUM_RANKS			= 5;		// 0-4
static const int	NUM_ARMOR			= 3;
static const float  ARM0				= 1.0f;
static const float	ARM1				= 0.85f;
static const float	ARM2				= 0.70f;
static const float	ARM3				= 0.60f;
static const float  ARM_EXTRA			= 0.20f;
static const int	PSI_ARM				= 40;				
static const float  CRAWLER_GROWTH		= 0.40f;

static const float STANDARD_TARGET_H			= 2.0f;		// roughly meters
static const float STANDARD_TARGET_W			= 0.6f;
static const float ACC_BEST_SHOT				= 0.005f;	// AREA of possible shot at distance=1.0
static const float ACC_WORST_SHOT				= 0.100f;	// AREA
static const float ACC_AIMED_SHOT_MULTIPLIER	= 1.0f;		// LOWER accuracy is better
static const float ACC_AUTO_SHOT_MULTIPLIER		= 2.0f;
static const float ACC_SNAP_SHOT_MULTIPLIER		= 4.0f;
 
static const float REACTION_FAST = 0.90f;
static const float REACTION_SLOW = 0.40f;

enum {
	FARM_SCOUT=11,	// coded in from the beginning of the game, when this was button order
	TNDR_SCOUT,
	FRST_SCOUT,
	DSRT_SCOUT,
	FARM_DESTROYER,
	TNDR_DESTROYER,
	FRST_DESTROYER,
	DSRT_DESTROYER,
	CITY,
	BATTLESHIP,
	ALIEN_BASE,
	TERRAN_BASE,

	FIRST_SCENARIO = FARM_SCOUT,
	LAST_SCENARIO = TERRAN_BASE,
};

#endif // GAMELIMITS_INCLUDED