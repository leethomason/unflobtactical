#ifndef GAMELIMITS_INCLUDED
#define GAMELIMITS_INCLUDED

const float FOV = 45.0f;
const int MAX_MODELS = 256;
const int MAX_UNITS	= 40;
const int MAP_SIZE = 64;
const int MAX_EYESIGHT_RANGE = 12;
const float EYE_HEIGHT = 1.2f;

const float NORMAL_TU		= 8.0f;
const float TU_SNAP_SHOT	= 2.0f;
const float TU_AUTO_SHOT	= 4.0f;
const float TU_AIMED_SHOT	= 3.0f;
const float TU_TURN			= 0.4f;
const float SECONDARY_SHOT_SPEED_MULT = 1.4f;

const int SNAP_SHOT = 0;
const int AUTO_SHOT = 1;
const int AIMED_SHOT = 2;

const int TRAIT_SOLDIER_LOW = 20;
const int TRAIT_SOLDIER_HIGH = 80;
const int TRAIT_LEVEL_BONUS = 5;

const float STANDARD_TARGET_AREA = 0.6f;	// roughly meters
const float ACC_GOOD_SHOT = 0.10f;
const float ACC_BAD_SHOT  = 0.30f;

const float ACC_AIMED_SHOT_MULTIPLIER = 1.0f;	// LOWER accuracy is better
const float ACC_AUTO_SHOT_MULTIPLIER  = 1.4f;
const float ACC_SNAP_SHOT_MULTIPLIER  = 1.8f;

#endif // GAMELIMITS_INCLUDED