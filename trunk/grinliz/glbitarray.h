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

namespace grinliz {

/**	A 2 dimensional bit map. Maps a bit to a 2 dimensional
	coordinate. Very useful for efficiently storing 2 dimensional
	boolean information. Constructed with 'size' parameters
	templatized.
*/
template< int WIDTH, int HEIGHT >
class BitArray
{
  public:
	enum {
		ROW_WIDTH  = ( ( WIDTH+31 ) / 32 ),	
		WORDSIZE = ROW_WIDTH * HEIGHT,
	};

	BitArray()					{ memset( array, 0, WORDSIZE*4 ); cache = 0; }	

	/// Check if (x,y) is set. Returns non-0 if set, 0 if not.
	U32 IsSet( int x, int y ) const	{ 
		GLASSERT( x >= 0 && x < WIDTH );
		GLASSERT( y >= 0 && y < HEIGHT );
		return array[ y*ROW_WIDTH + (x>>5) ] & ( 0x1 << (x & 31)); 
	}
	/// Set (x,y) true.
	void Set( int x, int y )	{ 
		GLASSERT( x >= 0 && x < WIDTH );
		GLASSERT( y >= 0 && y < HEIGHT );
		array[ y*ROW_WIDTH + (x>>5) ] |=   ( 0x1 << ( x & 31 ) ); 
	}
	/// Clear the bit at (x,y)
	void Clear( int x, int y )	{ 
		GLASSERT( x >= 0 && x < WIDTH );
		GLASSERT( y >= 0 && y < HEIGHT );	
		array[ y*ROW_WIDTH + (x>>5) ] &= (~( 0x1 << ( x & 31 ) ) ); 
	}
	/// Clear a rectangle of bits.
	void ClearRect( const Rectangle2I& rect )	{
													// FIXME: use a mask to make this more efficient.
													for( int j=rect.min.y; j<=rect.max.y; ++j )
														for( int i=rect.min.x; i<=rect.max.x; ++i )
															Clear( i, j );
												}
	/// Clear a rectangle of bits.
	void SetRect( const Rectangle2I& rect )	{
													// FIXME: use a mask to make this more efficient.
													for( int j=rect.min.y; j<=rect.max.y; ++j )
														for( int i=rect.min.x; i<=rect.max.x; ++i )
															Set( i, j );
												}
	/// Check if a rectangle is empty.
	bool IsRectEmpty( const Rectangle2I& rect ) const {
													// FIXME: use a mask to make this more efficient.
													for( int j=rect.min.y; j<=rect.max.y; ++j )
														for( int i=rect.min.x; i<=rect.max.x; ++i )
															if ( IsSet( i, j ) )
																return false;
													return true;
												}

	/// Check if a rectangle is completely set.
	bool IsRectSet( const Rectangle2I& rect ) const {
													// FIXME: use a mask to make this more efficient.
													for( int j=rect.min.y; j<=rect.max.y; ++j )
														for( int i=rect.min.x; i<=rect.max.x; ++i )
															if ( !IsSet( i, j ) )
																return false;
													return true;
												}
	/// Clear all the bits.
	void ClearAll()				{ memset( array, 0, WORDSIZE*4 ); }
	/// Set all the bits.
	void SetAll()				{ memset( array, 0xff, WORDSIZE*4 ); }

	/// Fast access - cache the y then query the x with IsSetCache
	void CacheY( int y )		{ cache = &array[ROW_WIDTH*y]; }
	/// Query the x at the cached y value.
	U32 IsSetCache( int x )		{ return cache[ x>>5 ] & ( 0x1 << (x & 31)); }

	// Private. (But friend iterators are no fun.)
	U32 array[ WORDSIZE ];
	U32* cache;
};

/**
	A class to quickly walk a BitArary.
*/
template< int WIDTH, int HEIGHT >
class BitArrayRowIterator
{
  public:
	BitArrayRowIterator( const BitArray<WIDTH, HEIGHT>& _array ) : bitArray( _array ), mask( 0 ), loc( 0 )	{}

	/// Initialize the walk.
	void Begin( int x, int y)	{	loc = &bitArray.array[ y*bitArray.ROW_WIDTH + (x/32) ];
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
	const BitArray<WIDTH, HEIGHT>& bitArray;
	U32 mask;
	const U32 *loc;
};
};
#endif

