/*
Copyright (c) 2000-2011 Lee Thomason (www.grinninglizard.com)
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

/** Rectangle class based on vectors. (Actually an AABB.) 
    inclusive-exclusive edges.
*/
template< class T > 
struct Rectangle3
{
	Vector3<T> pos, size;

	Rectangle3<T>() { pos.Zero(); size.Zero(); }
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
	void DoIntersection( const Rectangle3<T>& rect )
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

	bool operator==( const Rectangle3<T>& that ) const { return	pos == that.pos && size == that.size; }
	bool operator!=( const Rectangle3<T>& that ) const { return	pos != that.pos || size != that.size; }
};

typedef Rectangle3< int > Rectangle3I;
typedef Rectangle3< float > Rectangle3F;


template< class T >
struct Rectangle2
{
	Vector2< T > pos;
	Vector2< T > size;

	Rectangle2<T>() { pos.Zero(); size.Zero(); }
	Rectangle2<T>( T _x, T _y, T _cx, T _cy )  { pos.x = _x; pos.y = _y; size.x = _cx; size.y = _cy; }
	Rectangle2<T>( const Vector2<T>& _pos, const Vector2<T>& _size ) : pos( _pos ), size( _size ) {}

	T X0( int i ) const { return pos.X(i); }
	T X1( int i ) const { return pos.X(i)+size.X(i); }

	T X0() const { return pos.x; }
	T Y0() const { return pos.y; }
	T X1() const { return pos.x+size.x; }
	T Y1() const { return pos.y+size.y; }

	void Set( const Vector2<T>& a ) {
		pos = a;
		size.Zero();
	}

	void Set( const Vector2<T>& a, const Vector2<T>& b ) {
		pos = a;
		size.Zero();
		DoUnion( b );
	}

	/// Set all the members to zero.
	void Zero() {
		pos.Zero();
		size.Zero();
	}

	bool Empty() const { return size.x == 0 || size.y == 0; }

	/// Return true if the rectangles intersect.
	bool Intersect( const Rectangle2<T>& rect ) const
	{
		if (	(rect.pos.x+rect.size.x) <= pos.x
			 || rect.pos.x >= (pos.x+size.x)
			 || (rect.pos.y+rect.size.y) <= pos.y
			 || rect.pos.y >= (pos.y+size.y) )
		{
			return false;
		}
		return true;
	}	

	bool Intersect( const Vector2<T>& point ) const
	{
		if (	point.x < pos.x
			 || point.x >= (pos.x+size.x)
			 || point.y < pos.y
			 || point.y >= (pos.y+size.y) )
		{
			return false;
		}
		return true;
	}

	bool Intersect( T x, T y ) const
	{
		return Intersect( Vector2<T>( x, y ) );
	}

	Vector2< T > Center() const {
		Vector2< T > v = {	pos.x + size.x/2, 
							pos.y + size.y/2 };
		return v;
	}

	bool Contains( const Rectangle2<T>& rect ) const
	{
		for( int i=0; i<2; ++i ) {
			if ( rect.X0(i) < X0(i) || rect.X1(i) > X1(i) ) {
				return false;
			}
		}
		return true;
	}

	bool Contains( const Vector2<T>& point ) const
	{
		for( int i=0; i<2; ++i ) {
			if ( point.X(i) < X0(i) || point.X(i) >= X1(i) ) {
				return false;
			}
		}
		return true;
	}

	bool Contains( T x, T y ) const {
		Vector2<T> p = { x, y };
		return Contains( p );
	}

	/// Merge the Vector into this.
	void DoUnion( const Vector2<T>& vec )
	{
		for( int i=0; i<2; ++i ) {
			T x0 = grinliz::Min( X0(i), vec.X(i) );
			T x1 = grinliz::Max( X1(i), vec.X(i) );
			pos.X(i) = x0;
			size.X(i) = x1 - x0;
		}
	}

	void DoUnion( T x, T y ) { 
		Vector2<T> v = { x, y };
		DoUnion( v );
	}

	/// Merge the rect into this.
	void DoUnion( const Rectangle2<T>& rect )
	{
		DoUnion( rect.pos );
		DoUnion( rect.pos + rect.size );
	}
 
 	/// Turn this into the intersection.
	void DoIntersection( const Rectangle2<T>& rect )
	{
		for( int i=0; i<2; ++i ) {
			T x0 = grinliz::Max( X0(i), rect.X0(i) );
			T x1 = grinliz::Min( X1(i), rect.X1(i) );
			pos.X(i) = x0;
			T sz = x1 - x0;
			size.X(i) = sz > 0 ? sz : 0;
		}
	}

	/// Scale all coordinates by the given ratios:
	void Scale( T x, T y )
	{
		pos.x  *= x;
		size.x *= x;
		pos.y  *= y;
		size.y *= y;
	}

	/// Changes the boundaries
	void EdgeAdd( T i )
	{
		pos.x  -= i;
		size.x += i+i;
		pos.y  -= i;
		size.y += i+i;
	}

	void Outset( T i ) { EdgeAdd( i ); }

	/// Query the corners of the rectangle.
	Vector2<T> Corner( int i ) const
	{	
		Vector2<T> c = { 0, 0 };
		switch ( i ) {
			case 0:		c.Set( pos.x, pos.y );					break;
			case 1:		c.Set( pos.x+size.x, pos.y );			break;
			case 2:		c.Set( pos.x+size.x, pos.y+size.y );	break;
			case 3:		c.Set( pos.x, pos.y+size.y );			break;
			default:	GLASSERT( 0 );
		}
		return c;
	}
	/// Query the edge of the rectangle. The edges are ordered: bottom, right, top, left.
	void Edge( int i, Vector2< T >* head, Vector2< T >* tail ) const
	{	
		*tail = Corner(i);
		*head = Corner((i+1)&3);
	}

	bool operator==( const Rectangle2<T>& that ) const { return pos == that.pos && size == that.size; }
	bool operator!=( const Rectangle2<T>& that ) const { return pos != that.pos || size != that.size; }

};

typedef Rectangle2<int>   Rectangle2I;
typedef Rectangle2<float> Rectangle2F;

inline Rectangle2F Rectangle2I_To_2F( const Rectangle2I& in ) 
{
	Rectangle2F r( (float)in.pos.x, (float)in.pos.y, (float)in.size.x, (float)in.size.y );
	return r;
}

};	// namespace grinliz



#endif

