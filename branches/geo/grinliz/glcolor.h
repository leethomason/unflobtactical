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


#ifndef GRINLIZ_COLOR_INCLUDED
#define GRINLIZ_COLOR_INCLUDED

#include "gldebug.h"
#include "glutil.h"
#include "glvector.h"

namespace grinliz {

/** RGB color represented as floats.*/
struct Color3F
{
	float r, g, b;

	void Set( float _r, float _g, float _b )	{ this->r = _r; this->g = _g; this->b = _b; }
	void Set255( int _r, int _g, int _b )		
	{ 
		const float INV = 1.0f/255.0f;	
		Set( (float)_r*INV, (float)_g*INV, (float)_b*INV );
	}
	void Scale( float v )	{ r*=v; g*=v; b*=v; }
	float Average()			{ return (r+g+b)/3.0f; }

	friend Color3F operator*( const Color3F& rh, float lh ) {
		Color3F result = { rh.r * lh, rh.g * lh, rh.b * lh };
		return result;
	}
};

/** RGB color represented as bytes.*/
struct Color3U8
{
	void Set( U8 _r, U8 _g, U8 _b )	{ this->r = _r; this->g = _g; this->b = _b; }
	U8 r, g, b;
};

/** RGBA color represented as floats.*/
struct Color4F
{
	float Average()			{ return (r+g+b)/3.0f; }
	void Set( float _r, float _g, float _b, float _a )	{ this->r = _r; this->g = _g; this->b = _b; this->a = _a; }
	float r, g, b, a;
};

/** RGBA color represented as bytes.*/
struct Color4U8
{
	void Set( U8 _r, U8 _g, U8 _b, U8 _a )	{ this->r = _r; this->g = _g; this->b = _b; this->a=_a; }
	U8 r, g, b, a;
};

/** RGBA color represented as bytes.*/
struct Color4U32
{
	void Set( U32 _r, U32 _g, U32 _b, U32 _a )	{ this->r = _r; this->g = _g; this->b = _b; this->a=_a; }
	U32 r, g, b, a;
};


/// Color type conversion.
inline void Convert( const Color4F& c0, Color3U8* c1 ) {
	c1->r = (U8)LRintf( c0.r * 255.0f );
	c1->g = (U8)LRintf( c0.g * 255.0f );
	c1->b = (U8)LRintf( c0.b * 255.0f );
}

/// Interpolate between 2 colors. 'val' can range from 0 to 1.
inline void InterpolateColor( const Color3U8& c0, const Color3U8& c1, float val, Color3U8* out ) {
	GLASSERT( val >= 0.0f && val <= 1.0f );
	int ival = LRintf( val * 255.0f );
	GLASSERT( ival >= 0 && ival < 256 );
	out->r = (U8) Interpolate( 0, (int)c0.r, 255, (int)c1.r, ival );
	out->g = (U8) Interpolate( 0, (int)c0.g, 255, (int)c1.g, ival );
	out->b = (U8) Interpolate( 0, (int)c0.b, 255, (int)c1.b, ival );
}


};

#endif
