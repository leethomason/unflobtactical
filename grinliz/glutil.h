/*
Copyright (c) 2000-2006 Lee Thomason (www.grinninglizard.com)
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



#ifndef GRINLIZ_UTIL_INCLUDED
#define GRINLIZ_UTIL_INCLUDED

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <math.h>
#include "gldebug.h"
#include "gltypes.h"

namespace grinliz {

/// Minimum
template <class T> inline T		Min( T a, T b )		{ return ( a < b ) ? a : b; }
/// Maximum
template <class T> inline T		Max( T a, T b )		{ return ( a > b ) ? a : b; }
/// Minimum
template <class T> inline T		Min( T a, T b, T c )	{ return Min( a, Min( b, c ) ); }
/// Maximum
template <class T> inline T		Max( T a, T b, T c )	{ return Max( a, Max( b, c ) ); }
/// Minimum
template <class T> inline T		Min( T a, T b, T c, T d )	{ return Min( d, Min( a, b, c ) ); }
/// Maximum
template <class T> inline T		Max( T a, T b, T c, T d )	{ return Max( d, Max( a, b, c ) ); }

/// Swap
template <class T> inline void	Swap( T* a, T* b )	{ T temp; temp = *a; *a = *b; *b = temp; }
/// Returns true if value in the range [lower, upper]
template <class T> inline bool	InRange( T a, T lower, T upper )	{ return a >= lower && a <= upper; }
/// Returned the value clamped to the range [lower, upper]
template <class T> inline T		Clamp( const T& a, T lower, T upper )	
								{ 
									if ( a < lower )
										return lower;
									else if ( a > upper )
										return upper;
									return a;
								}

/// Average (mean) value
template <class T> inline T		Mean( T t0, T t1 )	{ return (t0 + t1)/static_cast<T>( 2 ); }
/// Average (mean) value
template <class T> inline T		Mean( T t0, T t1, T t2 )	{ return (t0+t1+t2)/static_cast<T>( 3 ); }

/// Round down to the next power of 2
inline U32 FloorPowerOf2( U32 v )
{
	v = v | (v>>1);
	v = v | (v>>2);
	v = v | (v>>4);
	v = v | (v>>8);
	v = v | (v>>16);
	return v - (v>>1);
}

/// Round up to the next power of 2
inline U32 CeilPowerOf2( U32 v )
{
	v = v - 1;
	v = v | (v>>1);
	v = v | (v>>2);
	v = v | (v>>4);
	v = v | (v>>8);
	v = v | (v>>16);
	return v + 1;
}

/// Linear interpolation.
template <class A, class B> inline B Interpolate( A x0, B q0, A x1, B q1, A x )
{
	GLASSERT( static_cast<B>( x1 - x0 ) != 0.0f );
	return q0 + static_cast<B>( x - x0 ) * ( q1 - q0 ) / static_cast<B>( x1 - x0 );
}

/// Smooth (hermite) interpolation.
template< class T > inline T HermiteInterpolate( T x0, T x1, T x )
{
	const T k0 = static_cast< T >( 0 );
	const T k1 = static_cast< T >( 1 );
	const T k2 = static_cast< T >( 2 );
	const T k3 = static_cast< T >( 3 );

	T t = Clamp(( x - x0) / (x1 - x0), k0, k1 );
	return t * t * (k3 - k2 * t);
}

/// Template for linear interpolation of points at unit distance.
template <class T> inline T InterpolateUnitX( T q0, T q1, T x )
{
	GLASSERT( x >= (T)0.0 );
	GLASSERT( x <= (T)1.0 );

	return q0*( static_cast<T>(1.0) - x ) + q1*x;
}

/** Template function for linear interpolation between 4 points, at x=0, x=1, y=0, and y=1.
*/
template <class T> inline T BilinearInterpolate( T q00, T q10, T q01, T q11, T x, T y )
{
	GLASSERT( x >= (T)0.0 );
	GLASSERT( x <= (T)1.0 );
	GLASSERT( y >= (T)0.0 );
	GLASSERT( y <= (T)1.0 );

	const T one = static_cast<T>(1);
	return q00*(one-x)*(one-y) + q10*(x)*(one-y) + q01*(one-x)*(y) + q11*(x)*(y);
}


/** Template function for linear interpolation between 4 points, at x=0, x=1, y=0, and y=1.
	Adds a weight factor for each point. A weight can be 0, but not all weights should be
	0 (if they are, 0 will be returned).
	The order of points in the input array is q00, q10, q01, q11.
*/
template <class T> inline T BilinearInterpolate( const T* q, T x, T y, const T* weight )
{
	GLASSERT( x >= (T)0.0 );
	GLASSERT( x <= (T)1.0 );
	GLASSERT( y >= (T)0.0 );
	GLASSERT( y <= (T)1.0 );

	const T one = static_cast<T>(1);
	const T zero = static_cast<T>(0);
	const T area[4] = { (one-x)*(one-y), (one-x)*(y), (x)*(one-y), x*y };

	T totalWeight = zero;
	T sum = zero;

	for ( unsigned i=0; i<4; ++i ) {
		totalWeight += area[i] * weight[i];
		sum += q[i] * area[i] * weight[i];
	}
	if ( totalWeight == zero )
		return zero;
	return sum / totalWeight;
}


/// fast double to long conversion, with rounding.
inline long LRint( double val)
{
	#if defined (__GNUC__)
		return lrint( val );
	#elif defined (_MSC_VER)
		long retval;
		__asm fld val
		__asm fistp retval
		return retval;
	#else
		// This could be better. 
		return (long)( val + 0.5 );
	#endif
}


/// fast float to long conversion, with rounding.
inline long LRintf( float val)
{
	#if defined (__GNUC__)
		return lrintf( val );
	#elif defined (_MSC_VER)
		long retval;
		__asm fld val
		__asm fistp retval
		return retval;
	#else
		// This could be better. 
		return (long)( val + 0.5f );
	#endif
}


extern float gU8ToFloat[256];

/// Normalized U8 to float; maps [0,255] to [0.0f,1.0f]
inline float U8ToFloat( U8 v )
{
	return gU8ToFloat[v];
}

#if defined(__hppa__) || \
    defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
    (defined(__MIPS__) && defined(__MISPEB__)) || \
    defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
    defined(__sparc__)
	#define GRINLIZ_BIG_ENDIAN
#else
	#define GRINLIZ_LITTLE_ENDIAN
#endif

inline U16 Swap16( U16 x ) { return ((x<<8)|(x>>8)); }
inline U32 Swap32( U32 x ) { return ((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24)); }

#ifdef GRINLIZ_LITTLE_ENDIAN
	inline U16 SwapLE16( U16 x )	{ return x; }
	inline U32 SwapLE32( U32 x )	{ return x; }
	inline U16 SwapBE16( U16 x )	{ return Swap16(x); }
	inline U32 SwapBE32( U32 x )	{ return Swap32(x); }
	void SwapBufferBE16( U16* buffer, int size );
	void SwapBufferBE32( U32* buffer, int size );
#else
	inline U16 SwapLE16( U16 x )	{ return Swap16(x); }
	inline U32 SwapLE32( U32 x )	{ return Swap32(x); }
	inline U16 SwapBE16( U16 x )	{ return x; }
	inline U32 SwapBE32( U32 x )	{ return x; }
	void SwapBufferBE16( U16* buffer, int size )	{}
	void SwapBufferBE32( U32* buffer, int size )	{}
#endif

/*	A class to store a set of bit flags, and set and get them in a standard way.
	Class T must be some kind of integer type.
*/
template < class T >
class Flag
{
  public:
	Flag()								{ store = 0; }
	
	inline void Set( T flag )			{ store |= flag; }
	inline void Clear( T flag )			{ store &= ~flag; }
	inline T IsSet( T flag ) const		{ return ( store & flag ); }

	inline U32  ToU32() const			{ return store; }
	inline void FromU32( U32 s )		{ store = (T) s; }

	inline void ClearAll()				{ store = 0; }

  private:
	T store;
};	

/*	A strange class: it creates bitmaps used for collision detection. Given
	an unsigned something, this is a utility class to pack bits...starting
	with the highest bit.
*/

template < class T >
class HighBitWriter 
{
  public:
	enum
	{
		MAXBIT = ( sizeof(T)*8-1 ),
		NUMBIT = ( sizeof(T)*8 ),
		ALLSET = T( -1 )
	};

	HighBitWriter( T* _data ) : data( _data ), bitPos( MAXBIT )	{}

	void Skip()
	{
		if ( bitPos == 0 )
		{
			++data;
			bitPos = MAXBIT;
		}
		else
		{
			--bitPos;
		}
	}

	void Skip_N( unsigned n )
	{
		bitPos -= n % NUMBIT;
		if ( bitPos < 0 )
		{
			bitPos += NUMBIT;
			++data;
		}
		data += n / NUMBIT;
	}
	
	void Push_1()	
	{
		*data |= ( 1 << bitPos );
		Skip();
	}

	void Push_1N( unsigned n )
	{
		// Push bits to T boundary
		while( n && bitPos != MAXBIT )
		{
			Push_1();
			--n;
		}

		// Write Full T size
		while( n >= NUMBIT )
		{
			*data = ALLSET;
			++data;
			n -= NUMBIT;
		}

		// Write the remainder
		while ( n )
		{
			Push_1();
			--n;
		}
	}

  private:
	T*	data;
	int bitPos;
};

/*
template< class T > class WeakPtr;

struct WeakPtrHolder
{
	WeakPtrHolder() {
		next = prev = this;
	}
	~WeakPtrHolder() {
		while ( next != prev ) {
			next
		}
	}
	WeakPtr< T >* next;
	WeakPtr< T >* prev;
};
*/

/** A weak pointer class. Differs from the boost implementation in that it requires the
	class referenced to have a 'weakPtrHolder' member. On the other hand, it imposes
	no restrictions on how the referenced class is deleted. Useful for objects that can
	be deleted out of the control of the library.
*/
/*
template< class T > 
class WeakPtr
{
public:
	enum Sentinel { SENTINEL };

	WeakPtr( Sentinel ) { prev = next = this; ptr = (T)(1); }

	WeakPtr()				{ prev = next = this; ptr = 0; }
	WeakPtr( T t )			{ Init( t ); }
	void operator=( T t )	{ GLASSERT( ptr != (T)(1) );
							  Unlink();
							  Init( t );
	}

	~WeakPtr()
	{
		if ( ptr == (T)(1) ) 
		{
			// We are the holder. Unlink everything.
			while( next != prev ) {
				next->Unlink();
			}
			GLASSERT( next == prev );
			GLASSERT( next == this );
			ptr = 0;
		}
		else {
			// We are not a sentinel: unlink ourselves.
			Unlink();
		}
	}

	T Ptr() { return ( ptr == (T)(1) ) ? 0 : ptr; }

private:
	WeakPtr( const WeakPtr& );
	void operator=( const WeakPtr& );

	void Init( T t )
	{
		GLASSERT( t );
		ptr = t;

		next = t->weakPtrHolder.next;
		prev = &t->weakPtrHolder;
		t->weakPtrHolder.next->prev = this;
		t->weakPtrHolder.next = this;
	}
	void Unlink()
	{
		prev->next = next;
		next->prev = prev;
		ptr = 0;
	}

	T ptr;
	WeakPtr<T>* next;
	WeakPtr<T>* prev;
};
*/
};	// namespace grinliz

#endif
