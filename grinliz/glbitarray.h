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

	/// Check if (x,y) is set. Returns non-0 if set, 0 if not.
	U32 IsSet( int x, int y, int z=0 ) const	{ 
		GLASSERT( x >= 0 && x < WIDTH );
		GLASSERT( y >= 0 && y < HEIGHT );
		GLASSERT( z >= 0 && z < DEPTH );
		return array[ z*PLANE32 + y*WIDTH32 + (x>>5) ] & ( 0x1 << (x & 31)); 
	}
	/// Set (x,y) true.
	void Set( int x, int y, int z=0 )	{ 
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
	void Clear( int x, int y, int z=0 )	{ 
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


	/*void SetRect( const Rectangle2I& rect, int z=0 )	
	{
		PLAYERASSERT( 0 );	// NEEDS DEBUGGING
		for( int j=rect.min.y; j<=rect.max.y; ++j ) {
			int x0 = (rect.min.x+31)&(~31);
			int x1 = (rect.max.x+32)&(~31);

			for( i=rect.min.x; i<x0; ++i ) {
				Set( i, j, z );
			}

			U32* p = &array[ z*PLANE32 + j*WIDTH32 + (i>>5) ];
			for( ; i <= x1; i+=32, ++p ) {
				*p = 0xffffffff;
			}

			for ( ; i<=rect.max.x; ++i ) {
				Set( i, j, z );
			}
		}
	}*/


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

	/// Clear all the bits.
	void ClearAll()				{ memset( array, 0, TOTAL_MEM ); }
	/// Set all the bits.
	void SetAll()				{ memset( array, 0xff, TOTAL_MEM ); }

	const U32* Access32( int x, int y, int z ) { return &array[ z*PLANE32 + y*WIDTH32 + (x>>5) ]; }

private:
	U32 array[ TOTAL_MEM32 ];
};

/**
	A class to quickly walk a BitArary.
*/

template< int WIDTH, int HEIGHT, int DEPTH >
class BitArrayRowIterator
{
  public:
	BitArrayRowIterator( const BitArray<WIDTH, HEIGHT, DEPTH>& _array ) : bitArray( _array ), mask( 0 ), loc( 0 )	{}

	/// Initialize the walk.
	void Begin( int x, int y, int z=0 )	
								{	loc = bitArray.Access32( x, y, z );
									U32 bit = x & 31;
									mask = 0x01 << bit;
								}
	/// Get the next bit.
	void Next()					{	mask <<= 1;
									if ( !mask ) {
										mask = 0x01;
										++loc;
									}
								}
	/// Return non-0 if the current location is set.
	U32 IsSet()					{	return ( *loc ) & ( mask ); }
	/// Return true if the current 32 bits are not set.
	bool WordEmpty()			{	return !(*loc); }

  private:
	const BitArray<WIDTH, HEIGHT, DEPTH>& bitArray;
	U32 mask;
	const U32 *loc;
};

};
#endif

