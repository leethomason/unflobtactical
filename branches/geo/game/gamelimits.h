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

#if 1
        #define AI_LOG GLOUTPUT
#else
        #define AI_LOG( x )      {}
#endif

#include "../engine/enginelimits.h"

static const int GAME_BUTTON_SIZE = 60;
static const float GAME_BUTTON_SIZE_F = (float)GAME_BUTTON_SIZE;

static const int SUBTURNS_PER_TURN = 3;
static const float FIRE_DAMAGE_PER_SUBTURN = 20.0f;
static const int MAP_SIZE = EL_MAP_SIZE;

static const int TERRAN_TEAM			= 0;
static const int CIV_TEAM				= 1;
static const int ALIEN_TEAM				= 2;
static const int NUM_TEAMS				= 3;

static const int TERRAN_UNITS_START	= 0;
static const int TERRAN_UNITS_END		= 8;
static const int CIV_UNITS_START		= 8;
static const int CIV_UNITS_END			= 24;
static const int ALIEN_UNITS_START		= 24;
static const int ALIEN_UNITS_END		= 40;
static const int MAX_ALIENS = ALIEN_UNITS_END - ALIEN_UNITS_START;
static const int MAX_TERRANS = TERRAN_UNITS_END - TERRAN_UNITS_START;
static const int MAX_CIVS = CIV_UNITS_END - CIV_UNITS_START;
static const int MAX_UNITS	= 40;

static const int MAX_MODELS = 256;
static const float EYE_HEIGHT = 1.2f;
static const float EXPLOSIVE_RANGE = 2.0f;

static const float TU_SNAP_SHOT	= 3.0f;
static const float TU_AUTO_SHOT	= 5.0f;
static const float TU_AIMED_SHOT	= 4.0f;
static const float SECONDARY_SHOT_SPEED_MULT = 1.4f;

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
static const float ARM_EXTRA			= 0.20f;

static const float STANDARD_TARGET_H			= 2.0f;		// roughly meters
static const float STANDARD_TARGET_W			= 0.6f;
static const float ACC_BEST_SHOT				= 0.005f;	// AREA of possible shot at distance=1.0
static const float ACC_WORST_SHOT				= 0.100f;	// AREA
static const float ACC_AIMED_SHOT_MULTIPLIER	= 1.0f;		// LOWER accuracy is better
static const float ACC_AUTO_SHOT_MULTIPLIER		= 2.0f;
static const float ACC_SNAP_SHOT_MULTIPLIER		= 4.0f;
 
static const float REACTION_FAST = 0.90f;
static const float REACTION_SLOW = 0.40f;

#endif // GAMELIMITS_INCLUDED