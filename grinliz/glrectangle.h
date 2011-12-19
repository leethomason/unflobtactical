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


#ifndef GRINLIZ_RECTANGLE_INCLUDED
#define GRINLIZ_RECTANGLE_INCLUDED

#include "glvector.h"

namespace grinliz {


/** A rectangle structure. Common, but deprecated. The min/max approach is abyssmal.
    Use Rect instead.
*/
template< class T >
struct Rectangle2
{
	Vector2< T > min;
	Vector2< T > max;

	/// Initialize. Convenience function.
	void Set( T _xmin, T _ymin, T _xmax, T _ymax )	{ 
		min.x = _xmin; min.y = _ymin; max.x = _xmax; max.y = _ymax;
	}
	/// Set all the members to zero.
	void Zero() {
		min.x = min.y = max.x = max.y = (T) 0;
	}

	/// Return true if this is potentially a valid rectangle.
	bool IsValid() const {
		return ( min.x <= max.x ) && ( min.y <= max.y );
	}

	/** Creates the rectangle from 2 points, which can be 
		in any relationship to each other.
	*/
	void FromPair( T x0, T y0, T x1, T y1 )
	{
		min.x = grinliz::Min( x0, x1 );
		max.x = grinliz::Max( x0, x1 );
		min.y = grinliz::Min( y0, y1 );
		max.y = grinliz::Max( y0, y1 );
	}

	/// Return true if the rectangles intersect.
	bool Intersect( const Rectangle2<T>& rect ) const
	{
		if (	rect.max.x < min.x
			 || rect.min.x > max.x
			 || rect.max.y < min.y
			 || rect.min.y > max.y )
		{
			return false;
		}
		return true;
	}	

	bool Intersect( const Vector2<T>& point ) const
	{
		if (	point.x < min.x
			 || point.x > max.x
			 || point.y < min.y
			 || point.y > max.y )
		{
			return false;
		}
		return true;
	}

	bool Intersect( T x, T y ) const
	{
		if (	x < min.x
			 || x > max.x
			 || y < min.y
			 || y > max.y )
		{
			return false;
		}
		return true;
	}


	/// Return true if 'rect' is inside this.
	bool Contains( const Rectangle2<T>& rect ) const
	{
		if (	rect.min.x >= min.x
			 && rect.max.x <= max.x
			 && rect.min.y >= min.y
			 && rect.max.y <= max.y )
		{
			return true;
		}
		return false;
	}

	bool Contains( const Vector2<T>& point ) const
	{
		if (	point.x >= min.x
			 && point.x <= max.x
			 && point.y >= min.y
			 && point.y <= max.y )
		{
			return true;
		}
		return false;
	}

	bool Contains( T x, T y ) const {
		Vector2<T> p = { x, y };
		return Contains( p );
	}

	/// Merge the rect into this.
	void DoUnion( const Rectangle2<T>& rect )
	{
		if ( IsValid() ) {
			min.x = grinliz::Min( min.x, rect.min.x );
			max.x = grinliz::Max( max.x, rect.max.x );
			min.y = grinliz::Min( min.y, rect.min.y );
			max.y = grinliz::Max( max.y, rect.max.y );
		}
		else {
			*this = rect;
		}
	}

	/// Merge the rect into this.
	void DoUnion( T x, T y )
	{
		if ( IsValid() ) {
			min.x = grinliz::Min( min.x, x );
			max.x = grinliz::Max( max.x, x );
			min.y = grinliz::Min( min.y, y );
			max.y = grinliz::Max( max.y, y );
		}
		else {
			Set( x, y, x, y );
		}
	}


	void DoUnion( const Vector2<T>& v ) { DoUnion( v.x, v.y ); }
 
 	/// Turn this into the intersection.
	void DoIntersection( const Rectangle2<T>& rect )
	{
		GLASSERT( IsValid() );
		min.x = grinliz::Max( min.x, rect.min.x );
		max.x = grinliz::Min( max.x, rect.max.x );
		min.y = grinliz::Max( min.y, rect.min.y );
		max.y = grinliz::Min( max.y, rect.max.y );
	}

	/// Clip this to the passed in rectangle. Will become invalid if they don't intersect.
	void DoClip( const Rectangle2<T>& rect )
	{
		min.x = rect.min.x > min.x ? rect.min.x : min.x;
		max.x = rect.max.x < max.x ? rect.max.x : max.x;
		min.y = rect.min.y > min.y ? rect.min.y : min.y;
		max.y = rect.max.y < max.y ? rect.max.y : max.y;
	}


	/// Scale all coordinates by the given ratios:
	void Scale( T x, T y )
	{
		min.x = ( x * min.x );
		min.y = ( y * min.y );
		max.x = ( x * max.x );
		max.y = ( y * max.y );
	}

	/// Changes the boundaries
	void EdgeAdd( T i )
	{
		min.x -= i;
		max.x += i;
		min.y -= i;
		max.y += i;
	}

	/// Query the edge of the rectangle. The edges are ordered: bottom, right, top, left.
	void Edge( int i, Vector2< T >* head, Vector2< T >* tail ) const
	{	
		switch ( i ) {
			case 0:		tail->Set( min.x, min.y );	head->Set( max.x, min.y );	break;
			case 1:		tail->Set( max.x, min.y );	head->Set( max.x, max.y );	break;
			case 2:		tail->Set( max.x, max.y );	head->Set( min.x, max.y );	break;
			case 3:		tail->Set( min.x, max.y );	head->Set( min.x, min.y );	break;
			default:	GLASSERT( 0 );
		}
	}

	/// Query the corners of the rectangle.
	void Corner( int i, Vector2< T >* c ) const
	{	
		switch ( i ) {
			case 0:		c->Set( min.x, min.y );	break;
			case 1:		c->Set( max.x, min.y );	break;
			case 2:		c->Set( max.x, max.y );	break;
			case 3:		c->Set( min.x, max.y );	break;
			default:	GLASSERT( 0 );
		}
	}


	void Outset( T dist ) {
		min.x -= dist;
		min.y -= dist;
		max.x += dist;
		max.y += dist;
	}

	Vector2< T > Center() const {
		Vector2< T > v = { (min.x + max.x) / (T)2, (min.y + max.y) / (T)2 };
		return v;
	}

	bool operator==( const Rectangle2<T>& that ) const { return     ( min.x == that.min.x )
													&& ( max.x == that.max.x )
													&& ( min.y == that.min.y )
													&& ( max.y == that.max.y ); }
	bool operator!=( const Rectangle2<T>& that ) const { return     ( min.x != that.min.x )
													|| ( max.x != that.max.x )
													|| ( min.y != that.min.y )
													|| ( max.y != that.max.y ); }

};


struct Rectangle2I : public Rectangle2< int >
{
	enum { INVALID = INT_MIN };

	int Width()	 const 	{ return max.x - min.x + 1; }		///< width of the rectangle
	int Height() const	{ return max.y - min.y + 1; }		///< height of the rectangle
	int Area()   const	{ return Width() * Height();	}   ///< Area of the rectangle

	/// Initialize to an invalid rectangle.
	void SetInvalid()	{ min.x = INVALID + 1; max.x = INVALID; min.y = INVALID + 1; max.y = INVALID; }

	/// Just like DoUnion, except takes validity into account.
	void DoUnionV( int x, int y )
	{
		if ( !IsValid() ) {
			min.x = max.x = x;
			min.y = max.y = y;
		}
		else {
			min.x = Min( min.x, x );
			max.x = Max( max.x, x );
			min.y = Min( min.y, y );
			max.y = Max( max.y, y );
		}
	}
};

struct Rectangle2F : public Rectangle2< float >
{
	float Width()	 const 	{ return max.x - min.x; }		///< width of the rectangle
	float Height() const	{ return max.y - min.y; }		///< height of the rectangle
	float Area()   const	{ return Width() * Height();	}   ///< Area of the rectangle
};

/** Rectangle class based on vectors. (Actually an AABB.) 
    inclusive-exclusive edges.
*/
template< class T > 
struct Rectangle3
{
	Vector3<T> pos, size;

	Rectangle3<T>() {}
	Rectangle3<T>( T _x, T _y, T _z, T _cx, T _cy, T _cz ) : pos( _x, _y, _z ), size( _cx, _cy, _cz ) {}
	Rectangle3<T>( const Vector3<T>& _pos, const Vector3<T>& _size ) : pos( _pos ), size( _size ) {}

	T X0( int i ) const { return pos.X(i); }
	T X1( int i ) const { return pos.X(i) + size.X(i); }

	T X0() const { return pos.x; }
	T Y0() const { return pos.y; }
	T Z0() const { return pos.z; }
	T X1() const { return pos.x+size.x; }
	T Y1() const { return pos.y+size.y; }
	T Z1() const { return pos.z+size.z; }

	void Set( const Vector3<T>& a ) {
		pos = a;
		size.Zero();
	}

	void Set( const Vector3<T>& a, const Vector3<T>& b ) {
		pos = a;
		size.Zero();
		DoUnion( b );
	}

	void Set( T x0, T y0, T z0, T x1, T y1, T z1 ) {
		Vector3<T> a = { x0, y0, z0 };
		Vector3<T> b = { x1, y1, z1 };
		Set( a, b );
	}

	/// Set all the members to zero.
	void Zero() {
		pos.Zero();
		size.Zero();
	}

	/// Return true if the rectangles intersect.
	bool Intersect( const Rectangle3<T>& rect ) const
	{
		if (	(rect.pos.x+rect.size.x) <= pos.x
			 || rect.pos.x >= (pos.x+size.x)
			 || (rect.pos.y+rect.size.y) <= pos.y
			 || rect.pos.y >= (pos.y+size.y)
			 || (rect.pos.z+rect.size.z) <= pos.z
			 || rect.pos.z >= (pos.z+size.z) )
		{
			return false;
		}
		return true;
	}	

	bool Intersect( const Vector3<T>& point ) const
	{
		if (	point.x < pos.x
			 || point.x >= (pos.x+size.x)
			 || point.y < pos.y
			 || point.y >= (pos.y+size.y)
			 || point.z < pos.z
			 || point.z >= (pos.z+size.z) )
		{
			return false;
		}
		return true;
	}

	bool Intersect( T x, T y, T z ) const
	{
		return Intersect( Vector3<T>( x, y, z ) );
	}

	Vector3< T > Center() const {
		Vector3< T > v = {	pos.x + size.x/2, 
							pos.y + size.y/2,
							pos.z + size.z/2 };
		return v;
	}

	/// Return true if 'rect' is inside this.
	bool Contains( const Rectangle3<T>& rect ) const
	{
		for( int i=0; i<3; ++i ) {
			if ( rect.X0(i) < X0(i) || rect.X1(i) > X1(i) ) {
				return false;
			}
		}
		return true;
	}

	bool Contains( const Vector3<T>& point ) const
	{
		for( int i=0; i<3; ++i ) {
			if ( point.X(i) < X0(i) || point.X(i) >= X1(i) ) {
				return false;
			}
		}
		return true;
	}

	/// Merge the Vector into this.
	void DoUnion( const Vector3<T>& vec )
	{
		for( int i=0; i<3; ++i ) {
			T x0 = grinliz::Min( X0(i), vec.X(i) );
			T x1 = grinliz::Max( X1(i), vec.X(i) );
			pos.X(i) = x0;
			size.X(i) = x1 - x0;
		}
	}


	/// Merge the rect into this.
	void DoUnion( const Rectangle3<T>& rect )
	{
		DoUnion( rect.pos );
		DoUnion( rect.pos + rect.size );
	}

	
	/// Turn this into the intersection.
	void DoIntersection( const Rectangle2<T>& rect )
	{
		for( int i=0; i<3; ++i ) {
			T x0 = grinliz::Max( X0(i), rect.X0(i) );
			T x1 = grinliz::Min( X1(i), rect.X1(i) );
			pos.X(i) = x0;
			T sz = x1 - x0;
			size.X(i) = sz > 0 ? sz : 0;
		}
	}


	/// Scale all coordinates by the given ratios:
	void Scale( T x, T y, T z )
	{
		// x0=2 x1=4 -> x0=1 x1=2
		// x0=2 x1=5 -> x0=6 x1=15
		pos.x  *= x;
		size.x *= x;
		pos.y  *= y;
		size.y *= y;
		pos.z  *= z;
		size.z *= z;
	}

	/// Changes the boundaries
	void EdgeAdd( T i )
	{
		pos.x  -= i;
		size.x += i+i;
		pos.y  -= i;
		size.y += i+i;
		pos.z  -= i;
		size.z += i+i;
	}

	bool operator==( const Rectangle2<T>& that ) const { return	pos == that.pos && size == that.size; }
	bool operator!=( const Rectangle2<T>& that ) const { return	pos != that.pos || size != that.size; }
};

typedef Rectangle3< int > Rectangle3I;
typedef Rectangle3< float > Rectangle3F;

template< class T >
struct Rect2
{
	T x, y, w, h;

	/// Initialize. Convenience function.
	void Set( T _x, T _y, T _w, T _h )	{ 
		x = _x; y = _y; w = _w; h = _h;
	}
	/// Set all the members to zero.
	void Zero() {
		x = y = w = h = (T) 0;
	}

	/// Return true if this is potentially a valid rectangle.
	bool IsValid() const {
		return ( w > 0 ) && ( h > 0 );
	}
};

typedef Rect2<int> Rect2I;
typedef Rect2<float> Rect2F;

};	// namespace grinliz



#endif

