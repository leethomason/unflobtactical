#ifndef GRINLIZ_FIXED_INCLUDED
#define GRINLIZ_FIXED_INCLUDED

#include "gldebug.h"
#include "gltypes.h"
#include "glutil.h"

namespace grinliz 
{
class Fixed      
{
public:
	enum {
		FIXED_1   =    0x10000,
		FIXED_MAX = 0x7FFF0000,
		EPSILON = (int)(0.000001f * 65536.0f)
	};

	typedef S32 FIXED;

	static inline FIXED FloatToFixed( float f )  		{ return grinliz::LRintf( f * 65536.0f ); }
	static inline FIXED FloatToFixed( double f )  		{ return grinliz::LRint( f * 65536.0 ); }
	static inline float FixedToFloat( S32 v )  			{ return (float)v / 65536.0f; }
	static inline int32_t FixedToInt( FIXED f )  		{ return f>>16; }
	static inline FIXED IntToFixed( int i )  			{ return i<<16; }
	static inline int32_t FixedToIntCeil( FIXED f )		{ return ( f + FIXED_1 - 1 ) >> 16; }

	static inline FIXED FixedMul( FIXED a, FIXED b ) {
		int64_t c = (int64_t)a * (int64_t)b;
		return (FIXED)(c>>16);
	}

	static inline FIXED FixedDiv( FIXED a, FIXED b )  {
		int64_t c = ((int64_t)(a)<<32) / ((int64_t)(b)<<16);
		return (FIXED) c;
	}

public:
   S32 x;

   Fixed() : x(0)												{}
   Fixed(const Fixed &f) : x(f.x)								{}
   explicit Fixed(const int f) : x(IntToFixed(f))				{}
   explicit Fixed(const unsigned int f) : x(IntToFixed(f))		{}
   explicit Fixed(const float f) : x(FloatToFixed(f))			{}

   operator int() const                      { return FixedToInt( x ); }
   operator unsigned int() const             { return FixedToInt( x ); }
   operator float() const                    { return FixedToFloat( x ); }

   int Ceil() const							 { return FixedToIntCeil( x ); }

   Fixed& operator = (const Fixed &v)          { x = v.x;				return *this; }
   Fixed& operator = (const int v)             { x = IntToFixed(v);     return *this; }
   Fixed& operator = (const unsigned int v)    { x = IntToFixed(v);     return *this; }
   Fixed& operator = (const float v)           { x = FloatToFixed(v);   return *this; }

   Fixed& operator +=  (const Fixed v)         { x += v.x;				return *this; }
   Fixed& operator +=  (const int v)           { x += IntToFixed(v);    return *this; }
   Fixed& operator +=  (const float v)         { x += FloatToFixed(v);  return *this; }

   Fixed& operator -=  (const Fixed v)		   { x -= v.x;				return *this; }
   Fixed& operator -=  (const int v)           { x -= IntToFixed( v );  return *this; }
   Fixed& operator -=  (const float v)         { x -= FloatToFixed( v); return *this; }

   Fixed& operator *=  (const Fixed v)         { x = FixedMul( x, v.x );				return *this; }
   Fixed& operator *=  (const int v)           { x *= v;								return *this; }
   Fixed& operator *=  (const float v)         { x = FixedMul( x, FloatToFixed( v ) );	return *this; }

   Fixed& operator /=  (const Fixed v)         { x = FixedDiv( x, v.x );				return *this; }
   Fixed& operator /=  (const int v)           { x /= v;								return *this; }
   Fixed& operator /=  (const float v)         { x = FixedMul( x, FloatToFixed( v ) );  return *this; }

   Fixed& operator <<= (const int v)           { x <<= v;           return *this; }
   Fixed& operator >>= (const int v)           { x >>= v;           return *this; }

   Fixed& operator ++ ()                       { x += FIXED_1;		return *this; }
   Fixed& operator -- ()                       { x -= FIXED_1;		return *this; }

   Fixed operator ++ (int)                     { Fixed t;  t.x = x;   x += FIXED_1;  return t; }
   Fixed operator -- (int)                     { Fixed t;  t.x = x;   x -= FIXED_1;  return t; }

   Fixed operator - () const                   { Fixed t;  t.x = -x;  return t; }

   inline friend Fixed operator +  (const Fixed a, const Fixed b)	{ Fixed t;  t.x = a.x + b.x;	return t; }
   inline friend Fixed operator +  (const Fixed a, const int b)		{ Fixed t;  t.x = a.x + IntToFixed(b);	return t; }
   inline friend Fixed operator +  (const int a, const Fixed b)		{ Fixed t;  t.x = IntToFixed( a ) + b.x;	return t; }
   inline friend Fixed operator +  (const Fixed a, const float b)	{ Fixed t;  t.x = a.x + FloatToFixed( b );	return t; }
   inline friend Fixed operator +  (const float a, const Fixed b)	{ Fixed t;  t.x = FloatToFixed( a ) + b.x;	return t; }

   inline friend Fixed operator -  (const Fixed a, const Fixed b)	{ Fixed t;	t.x = a.x - b.x;	return t; }
   inline friend Fixed operator -  (const Fixed a, const int b)		{ Fixed t;	t.x = a.x - IntToFixed( b );	return t; }
   inline friend Fixed operator -  (const int a, const Fixed b)		{ Fixed t;	t.x = IntToFixed( a ) - b.x;	return t; }
   inline friend Fixed operator -  (const Fixed a, const float b)	{ Fixed t;	t.x = a.x - FloatToFixed( b );	return t; }
   inline friend Fixed operator -  (const float a, const Fixed b)	{ Fixed t;	t.x = FloatToFixed( a ) - b.x;	return t; }

   inline friend Fixed operator *  (const Fixed a, const Fixed b)	{ Fixed t;	t.x = FixedMul( a.x, b.x );	return t; }
   inline friend Fixed operator *  (const Fixed a, const int b)		{ Fixed t;	t.x = a.x * b; return t; }
   inline friend Fixed operator *  (const int a, const Fixed b)		{ Fixed t;	t.x = a * b.x; return t; }
   inline friend Fixed operator *  (const Fixed a, const float b)	{ Fixed t;	t.x = FixedMul( a.x, FloatToFixed( b ) ); return t; }
   inline friend Fixed operator *  (const float a, const Fixed b)	{ Fixed t;	t.x = FixedMul( FloatToFixed( a ), b.x ); return t; }

   inline friend Fixed operator /  (const Fixed a, const Fixed b)	{ Fixed t;	t.x = FixedDiv( a.x, b.x ); return t; }
   inline friend Fixed operator /  (const Fixed a, const int b)		{ Fixed t;	t.x = a.x / b; return t; }
   inline friend Fixed operator /  (const int a, const Fixed b)		{ Fixed t;	t.x = FixedDiv( IntToFixed( a ), b ); return t; }
   inline friend Fixed operator /  (const Fixed a, const float b)	{ Fixed t;	t.x = FixedDiv( a, FloatToFixed( b ) ); return t; }
   inline friend Fixed operator /  (const float a, const Fixed b)	{ Fixed t;	t.x = FixedDiv( FloatToFixed( a ), b ); return t; }

   inline friend Fixed operator << (const Fixed a, const int b)		{ Fixed t; t.x = a.x << b; return t; }
   inline friend Fixed operator >> (const Fixed a, const int b)		{ Fixed t; t.x = a.x >> b; return t; }

   inline friend bool operator == (const Fixed a, const Fixed b)	{ return a.x == b.x; }
   inline friend bool operator == (const Fixed a, const int b)		{ return a.x == IntToFixed( b ); }
   inline friend bool operator == (const int a, const Fixed b)		{ return IntToFixed( a ) == b.x; }
   inline friend bool operator == (const Fixed a, const float b)	{ return a.x == FloatToFixed( b ); }
   inline friend bool operator == (const float a, const Fixed b)	{ return FloatToFixed( a ) == b.x; }

   inline friend bool operator != (const Fixed a, const Fixed b)	{ return a.x != b.x; }
   inline friend bool operator != (const Fixed a, const int b)		{ return a.x != IntToFixed( b ); }
   inline friend bool operator != (const int a, const Fixed b)		{ return IntToFixed( a ) != b.x; }
   inline friend bool operator != (const Fixed a, const float b)	{ return a.x != FloatToFixed( b ); }
   inline friend bool operator != (const float a, const Fixed b)	{ return FloatToFixed( a ) != b.x; }

   inline friend bool operator <  (const Fixed a, const Fixed b)	{ return a.x < b.x; }
   inline friend bool operator <  (const Fixed a, const int b)		{ return a.x < IntToFixed( b ); }
   inline friend bool operator <  (const int a, const Fixed b)		{ return IntToFixed( a ) < b.x; }
   inline friend bool operator <  (const Fixed a, const float b)	{ return a.x < FloatToFixed( b ); }
   inline friend bool operator <  (const float a, const Fixed b)	{ return FloatToFixed( a ) < b.x; }

   inline friend bool operator >  (const Fixed a, const Fixed b)	{ return a.x > b.x; }
   inline friend bool operator >  (const Fixed a, const int b)		{ return a.x > IntToFixed( b ); }
   inline friend bool operator >  (const int a, const Fixed b)		{ return IntToFixed( a ) > b.x; }
   inline friend bool operator >  (const Fixed a, const float b)	{ return a.x > FloatToFixed( b ); }
   inline friend bool operator >  (const float a, const Fixed b)	{ return FloatToFixed( a ) > b.x; }

   inline friend bool operator <=  (const Fixed a, const Fixed b)	{ return a.x <= b.x; }
   inline friend bool operator <=  (const Fixed a, const int b)		{ return a.x <= IntToFixed( b ); }
   inline friend bool operator <=  (const int a, const Fixed b)		{ return IntToFixed( a ) <= b.x; }
   inline friend bool operator <=  (const Fixed a, const float b)	{ return a.x <= FloatToFixed( b ); }
   inline friend bool operator <=  (const float a, const Fixed b)	{ return FloatToFixed( a ) <= b.x; }

   inline friend bool operator >=  (const Fixed a, const Fixed b)	{ return a.x >= b.x; }
   inline friend bool operator >=  (const Fixed a, const int b)		{ return a.x >= IntToFixed( b ); }
   inline friend bool operator >=  (const int a, const Fixed b)		{ return IntToFixed( a ) >= b.x; }
   inline friend bool operator >=  (const Fixed a, const float b)	{ return a.x >= FloatToFixed( b ); }
   inline friend bool operator >=  (const float a, const Fixed b)	{ return FloatToFixed( a ) >= b.x; }

   inline Fixed Sqrt()	{ Fixed t; t.x = FloatToFixed( sqrtf( FixedToFloat( x ) ) ); return t; }
};

};	// grinliz
#endif // GRINLIZ_FIXED_INCLUDED
