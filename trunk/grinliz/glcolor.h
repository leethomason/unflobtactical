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

/** RGB color.*/
template <class T>
struct Color3
{
	T r, g, b;

	void Set( T _r, T _g, T _b )	{ this->r = _r; this->g = _g; this->b = _b; }
	T X( int i ) const				{ GLASSERT( i>=0 && i<3 ): return *(&r+i); }
	T& X(int i)						{ GLASSERT( i>=0 && i<3 ): return *(&r+i); }
};

typedef Color3<float> Color3F;
typedef Color3<U8> Color3U8;


/** RGBA color represented as floats.*/
template <class T>
struct Color4
{
	T r, g, b, a;

	void Set( T _r, T _g, T _b, T _a )	{ this->r = _r; this->g = _g; this->b = _b; this->a = _a; }
	
	T X(int i) const					{ GLASSERT( i>=0 && i<4 ); return *(&r+i); }
	T& X(int i)							{ GLASSERT( i>=0 && i<4 ); return *(&r+i); }

	bool operator==( const Color4<T>& rh ) const {
		return r==rh.r && g==rh.g && b==rh.b && a==rh.a;
	}
	bool operator!=( const Color4<T>& rh ) const {
		return r!=rh.r || g!=rh.g || b!=rh.b || a!=rh.a;
	}
};

typedef Color4<float> Color4F;
typedef Color4<U8> Color4U8;

/// Color type conversion.
inline void Convert( const Color4F& c0, Color3U8* c1 ) {
	c1->r = (U8)LRintf( c0.r * 255.0f );
	c1->g = (U8)LRintf( c0.g * 255.0f );
	c1->b = (U8)LRintf( c0.b * 255.0f );
}

inline Color4U8 Convert_4F_4U8( const Color4F& c0 ) {
	Color4U8 c1;
	c1.r = (U8)LRintf( c0.r * 255.0f );
	c1.g = (U8)LRintf( c0.g * 255.0f );
	c1.b = (U8)LRintf( c0.b * 255.0f );
	c1.a = (U8)LRintf( c0.a * 255.0f );
	return c1;
}

inline Color4F Convert_4U8_4F( const Color4U8& c0 ) {
	Color4F c1;
	static const float INV=1.0f/255.f;
	c1.r = (float)c0.r * INV;
	c1.g = (float)c0.g * INV;
	c1.b = (float)c0.b * INV;
	c1.a = (float)c0.a * INV;
	return c1;
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
