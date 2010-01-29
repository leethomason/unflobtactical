#ifndef GAMELIMITS_INCLUDED
#define GAMELIMITS_INCLUDED

const float FOV = 45.0f;

const int SUBTURNS_PER_TURN = 3;
const float FIRE_DAMAGE_PER_SUBTURN = 20.0f;

const int TERRAN_TEAM			= 0;
const int CIV_TEAM				= 1;
const int ALIEN_TEAM			= 2;

const int TERRAN_UNITS_START	= 0;
const int TERRAN_UNITS_END		= 8;
const int CIV_UNITS_START		= 8;
const int CIV_UNITS_END			= 24;
const int ALIEN_UNITS_START		= 24;
const int ALIEN_UNITS_END		= 40;
const int MAX_UNITS	= 40;


const int MAX_MODELS = 256;
const int MAP_SIZE = 64;
const int MAX_EYESIGHT_RANGE    = 13;
const int MAX_EYESIGHT_RANGE_45 = 10;	// MAX * cos(45deg), rounded up
const float EYE_HEIGHT = 1.2f;

const float NORMAL_TU		= 8.0f;
const int	MAX_TU			= 16;		// enforced: used for memory allocation

const float TU_SNAP_SHOT	= 3.0f;
const float TU_AUTO_SHOT	= 5.0f;
const float TU_AIMED_SHOT	= 4.0f;
const float TU_TURN			= 0.4f;
const float SECONDARY_SHOT_SPEED_MULT = 1.4f;

const int SNAP_SHOT			= 0;
const int AUTO_SHOT			= 1;
const int AIMED_SHOT		= 2;
const int FIRE_MODE_START	= 0;
const int FIRE_MODE_END		= 3;

const int TRAIT_SOLDIER_LOW = 20;
const int TRAIT_SOLDIER_HIGH = 80;
const int TRAIT_LEVEL_BONUS = 5;

const float STANDARD_TARGET_AREA = 1.0f;	// roughly meters
const float ACC_GOOD_SHOT = 0.10f;
const float ACC_BAD_SHOT  = 0.30f;
const float REACTION_FAST = 0.80f;
const float REACTION_SLOW = 0.20f;

const float ACC_AIMED_SHOT_MULTIPLIER = 1.0f;	// LOWER accuracy is better
const float ACC_AUTO_SHOT_MULTIPLIER  = 1.4f;
const float ACC_SNAP_SHOT_MULTIPLIER  = 1.8f;

#endif // GAMELIMITS_INCLUDED