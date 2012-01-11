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

#ifndef GRINLIZ_BITARRAY_INCLUDED
#define GRINLIZ_BITARRAY_INCLUDED


#include "gltypes.h"
#include "glrectangle.h"

namespace grinliz {

/**	A 3 dimensional bit map. Maps a bit to a 3 dimensional
	coordinate. Very useful for efficiently storing 3 dimensional
	boolean information. Constructed with 'size' parameters
	templatized.
*/
template< int WIDTH, int HEIGHT, int DEPTH >
class BitArray
{
  public:
	enum {
		WIDTH32		= ( ( WIDTH+31 ) / 32 ),
		PLANE32		= WIDTH32 * HEIGHT,
		TOTAL_MEM32	= WIDTH32 * HEIGHT * DEPTH,
		TOTAL_MEM   = TOTAL_MEM32*4
	};

	BitArray()					{ memset( array, 0, TOTAL_MEM ); }	

	bool operator==( const BitArray< WIDTH, HEIGHT, DEPTH >& rhs ) const {
		return memcmp( array, rhs.array, TOTAL_MEM ) == 0;
	}
	bool operator!=( const BitArray< WIDTH, HEIGHT, DEPTH >& rhs ) const {
		return memcmp( array, rhs.array, TOTAL_MEM ) != 0;
	}
	void operator=( const BitArray< WIDTH, HEIGHT, DEPTH >& rhs ) {
		memcpy( array, rhs.array, TOTAL_MEM );
	}

	/// Check if (x,y) is set. Returns non-0 if set, 0 if not.
	U32 IsSet( int x, int y=0, int z=0 ) const	{ 
		GLASSERT( x >= 0 && x < WIDTH );
		GLASSERT( y >= 0 && y < HEIGHT );
		GLASSERT( z >= 0 && z < DEPTH );
		return array[ z*PLANE32 + y*WIDTH32 + (x>>5) ] & ( 0x1 << (x & 31)); 
	}
	/// Set (x,y) true.
	void Set( int x, int y=0, int z=0 )	{ 
		GLASSERT( x >= 0 && x < WIDTH );
		GLASSERT( y >= 0 && y < HEIGHT );
		GLASSERT( z >= 0 && z < DEPTH );
		array[ z*PLANE32 + y*WIDTH32 + (x>>5) ] |= ( 0x1 << (x & 31));
	}
	/// Set (x,y) to 'on'
	void Set( int x, int y, int z, bool on )	{ 
		if ( on )
			Set( x, y, z );
		else
			Clear( x, y, z );
	}
	/// Clear the bit at (x,y)
	void Clear( int x, int y=0, int z=0 )	{ 
		GLASSERT( x >= 0 && x < WIDTH );
		GLASSERT( y >= 0 && y < HEIGHT );	
		GLASSERT( z >= 0 && z < DEPTH );
		array[ z*PLANE32 + y*WIDTH32 + (x>>5) ] &= (~( 0x1 << ( x & 31 ) ) ); 
	}
	/// Clear a rectangle of bits
	void ClearRect( const Rectangle2I& rect, int z=0 )	{
													// FIXME: use a mask to make this more efficient.
													for( int j=rect.min.y; j<=rect.max.y; ++j )
														for( int i=rect.min.x; i<=rect.max.x; ++i )
															Clear( i, j, z );
													}
	void ClearPlane( int z )						
	{
		GLASSERT( z >= 0 && z < DEPTH );
		memset( &array[ z*PLANE32 ], 0, PLANE32*4 );
	}

	/// Clear a rectangle of bits in z=0.
	void SetRect( const Rectangle2I& rect, int z=0 )	
	{
		for( int j=rect.min.y; j<=rect.max.y; ++j )
			for( int i=rect.min.x; i<=rect.max.x; ++i )	
				Set( i, j, z );
	}


	/// Check if a rectangle is empty in z=0.
	bool IsRectEmpty( const Rectangle2I& rect, int z=0 ) const 
	{
		for( int j=rect.min.y; j<=rect.max.y; ++j )
			for( int i=rect.min.x; i<=rect.max.x; ++i )
				if ( IsSet( i, j, z ) )
					return false;
		return true;
	}

	/// Check if a rectangle is empty in z=0.
	bool IsRectEmpty( const Rectangle3I& rect ) const 
	{
		for( int k=rect.min.z; k<=rect.max.z; ++k )
			for( int j=rect.min.y; j<=rect.max.y; ++j )
				for( int i=rect.min.x; i<=rect.max.x; ++i )
					if ( IsSet( i, j, k ) )
						return false;
		return true;
	}
	/// Check if a rectangle is completely set in z=0.
	bool IsRectSet( const Rectangle2I& rect, int z=0 ) const 
	{
		// FIXME: use a mask to make this more efficient.
		for( int j=rect.min.y; j<=rect.max.y; ++j )
			for( int i=rect.min.x; i<=rect.max.x; ++i )
				if ( !IsSet( i, j, z ) )
					return false;
		return true;
	}

	/// Check if a rectangle is completely set in z=0.
	bool IsRectSet( const Rectangle3I& rect ) const 
	{
		for( int k=rect.min.z; k<=rect.max.z; ++k )
			for( int j=rect.min.y; j<=rect.max.y; ++j )
				for( int i=rect.min.x; i<=rect.max.x; ++i )
					if ( !IsSet( i, j, k ) )
						return false;
	}

	int NumSet( const Rectangle3I& rect ) const 
	{
		int count = 0;
		for( int k=rect.min.z; k<=rect.max.z; ++k )
			for( int j=rect.min.y; j<=rect.max.y; ++j )
				for( int i=rect.min.x; i<=rect.max.x; ++i )
					if ( IsSet( i, j, k ) )
						++count;
		return count;
	}

	void DoUnion( const BitArray< WIDTH, HEIGHT, DEPTH >& rhs ) {
		for( int i=0; i<TOTAL_MEM32; ++i ) {
			array[i] |= rhs.array[i];
		}
	}

	/// Clear all the bits.
	void ClearAll()				{ memset( array, 0, TOTAL_MEM ); }
	/// Set all the bits.
	void SetAll()				{ memset( array, 0xff, TOTAL_MEM ); }

	U32 Access32( int x, int y, int z ) { return array[ z*PLANE32 + y*WIDTH32 + (x>>5) ]; }

	// 0xffffffff
	enum { STRING_SIZE = TOTAL_MEM32*8 + 1 };

	void ToString( char* str ) const {
		for( int i=0; i<TOTAL_MEM32; ++i ) {
			for( int k=7; k>=0; --k ) {
				U32 nybble = ( array[i] >> (k*4) ) & 0x0f;
				*str++ = ( nybble < 10 ) ? ('0'+nybble) : ('a'+(nybble-10));
			}
		}
		*str = 0;
	}

	void FromString( const char* str ) {
		ClearAll();
		for( int i=0; i<TOTAL_MEM32; ++i ) {
			for( int nybble = 7; nybble >= 0; --nybble, ++str ) {
				if ( *str >= '0' && *str <= '9') {
					array[i] |= (*str-'0')<<(nybble*4);
				}
				else if (*str >= 'a' && *str <= 'f' ) {
					array[i] |= (*str-'a'+10)<<(nybble*4);
				}
				else {
					GLASSERT( 0 );
					break;
				}
			}
		}
	}

private:
	U32 array[ TOTAL_MEM32 ];
};

};
#endif

