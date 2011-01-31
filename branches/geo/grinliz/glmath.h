/*
Copyright (c) 2000-2007 Lee Thomason (www.grinninglizard.com)
Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/


#ifndef GRINLIZ_MATH_INCLUDED
#define GRINLIZ_MATH_INCLUDED

#include "glutil.h"

namespace grinliz
{
const float  PI = 3.1415926535897932384626433832795f;
const float  TWO_PI = 2.0f*3.1415926535897932384626433832795f;
const double PI_D = 3.1415926535897932384626433832795;
const double TWO_PI_D = 2.0*3.1415926535897932384626433832795;

const float RAD_TO_DEG = (float)( 360.0 / ( 2.0 * PI ) );
const float DEG_TO_RAD = (float)( ( 2.0 * PI ) / 360.0f );
const double RAD_TO_DEG_D = ( 360.0 / ( 2.0 * PI_D ) );
const double DEG_TO_RAD_D = ( ( 2.0 * PI_D ) / 360.0 );
const float SQRT2 = 1.4142135623730950488016887242097f;
const float SQRT2OVER2 = float( 1.4142135623730950488016887242097 / 2.0 );

inline float ToDegree( float radian ) { return radian * RAD_TO_DEG; }
inline double ToDegree( double radian ) { return radian * RAD_TO_DEG_D; }
inline float ToRadian( float degree ) { return degree * DEG_TO_RAD; }
inline double ToRadian( double degree ) { return degree * DEG_TO_RAD_D; }

void SinCosDegree( float degreeTheta, float* sinTheta, float* cosTheta );

const float EPSILON = 0.000001f;

inline float NormalizeAngleDegrees( float alpha ) {
	while ( alpha < 0.0f )		alpha += 360.0f;
	while ( alpha >= 360.0f )	alpha -= 360.0f;
	return alpha;
}

inline int NormalizeAngleDegrees( int alpha ) {
	while ( alpha < 0 )		alpha += 360;
	while ( alpha >= 360 )	alpha -= 360;
	return alpha;
}


/** A loose equality check. */
inline bool Equal( float x, float y, float epsilon )
{
	return fabsf( x - y ) <= epsilon;
}

inline bool Equal( double x, double y, double epsilon )
{
	return fabs( x - y ) <= epsilon;
}

inline bool EqualInt( float a, float epsilon = EPSILON )
{
	return Equal( (float)LRintf(a), a, epsilon );
}

/** The shortest path between 2 angles.
	@param angle0	The first angle in degrees.
	@param angle1	The second angle in degrees.
	@param distance	The distance in degrees between the 2 angles - always positive.
	@param bias		The direction of travel - either +1.0 or -1.0
*/
void MinDeltaDegrees( float angle0, float angle1, float* distance, float* bias );

/** A length that is reasonably accurate. (24 bits or better.) */
inline float Length( float x, float y )
{
	// It's worth some time here:
	//		http://www.azillionmonkeys.com/qed/sqroot.html
	// to make this (perhaps) better. 

	// Use the much faster "f" version.
	return sqrtf( x*x + y*y );
}

inline double Length( double x, double y ) 
{ 
	// The more accurate double version.
	return sqrt( x*x + y*y ); 
}

/** A length that is reasonably accurate. (24 bits or better.) */
inline float Length( float x, float y, float z )
{
	return sqrtf( x*x + y*y + z*z );
}

inline double Length( double x, double y, double z ) 
{ 
	return sqrt( x*x + y*y + z*z ); 
}

/** A length that is reasonably accurate. (24 bits or better.) */
inline float Length( float x, float y, float z, float w )
{
	return sqrtf( x*x + y*y + z*z + w*w );
}


/// Approximate length, but fast. NOTE: Length is faster for float. 
template <class T> inline T FastLength( T x, T y, T z )
{
	float a = (float) fabs( x );
	float b = (float) fabs( y );
	float c = (float) fabs( z );

	if ( b > a ) Swap( &b, &a );
	if ( c > a ) Swap( &c, &a );

	// a, b, and c are now meaningless, but
	// a is the max.

	float ret = a + ( b + c ) / 4;
	return ret;
}


/// Approximate length, but fast. NOTE: Length is faster for float.
template <class T> inline T FastLength( T x, T y )
{
	float a = (float) fabs( x );
	float b = (float) fabs( y );

	if ( b > a ) grinliz::Swap( &b, &a );

	// a and b are now meaningless, but
	// a is the max.

	float ret = a + b / 4;
	return ret;
}

/// Template to return the average of 2 numbers.
template <class T> inline T Average( T y0, T y1 )
{
	return ( y0 + y1 ) / T( 2 );
}



};	// namespace grinliz

#endif
