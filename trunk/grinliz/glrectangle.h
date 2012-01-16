/*
Copyright (c) 2000-2012 Lee Thomason (www.grinninglizard.com)
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


/** A rectangle structure. Uses a min/max approach, which is inclusive/inclusive.
    Awkward sometimes, handy union behavior. Be aware the the size is type dependent.
*/
template< class T >
struct Rectangle2
{
	Vector2< T > min;
	Vector2< T > max;

	void Set( T xmin, T ymin, T xmax, T ymax ) {
		GLASSERT( xmax >= xmin );
		GLASSERT( ymax >= ymin );
		min.Set( xmin, ymin );
		max.Set( xmax, ymax );
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
			min.Set( x, y );
			max.Set( x, y );
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

	bool operator==( const Rectangle2<T>& that ) const { return min == that.min && max == that.max; }
	bool operator!=( const Rectangle2<T>& that ) const { return min != that.min || max != that.max; }
};


struct Rectangle2I : public Rectangle2< int >
{
	Rectangle2I() { min.Zero(); max.Zero(); }
	Rectangle2I( const Rectangle2I& rhs ) { *this = rhs; }
	Rectangle2I( int _xmin, int _ymin, int _xmax, int _ymax ) {
		min.Set( _xmin, _ymin );
		max.Set( _xmax, _ymax );
	} 

	enum { INVALID = INT_MIN };

	int Width()	 const 	{ return max.x - min.x + 1; }		///< width of the rectangle
	int Height() const	{ return max.y - min.y + 1; }		///< height of the rectangle
	int Area()   const	{ return Width() * Height();	}   ///< Area of the rectangle

	/// Initialize to an invalid rectangle.
	void SetInvalid()	{ min.x = INVALID + 1; max.x = INVALID; min.y = INVALID + 1; max.y = INVALID; }
};

struct Rectangle2F : public Rectangle2< float >
{
	Rectangle2F() { min.Zero(); max.Zero(); }
	Rectangle2F( const Rectangle2F& rhs ) { *this = rhs; }
	Rectangle2F( float _xmin, float _ymin, float _xmax, float _ymax ) { 
		min.Set( _xmin, _ymin );
		max.Set( _xmax, _ymax );
	} 

	float Width()	 const 	{ return max.x - min.x; }		///< width of the rectangle
	float Height() const	{ return max.y - min.y; }		///< height of the rectangle
	float Area()   const	{ return Width() * Height();	}   ///< Area of the rectangle
};


template< class T > 
struct Rectangle3
{
	Vector3< T > min;
	Vector3< T > max;

	void Set( T xmin, T ymin, T zmin, T xmax, T ymax, T zmax ) {
		GLASSERT( xmax >= xmin );
		GLASSERT( ymax >= ymin );
		GLASSERT( zmax >= zmin );
		min.Set( xmin, ymin, zmin );
		max.Set( xmax, ymax, zmax );
	}

	/// If i==0 return min, else return max
	const Vector3< T >& Vec( int i ) { return (i==0) ? min : max; }

	void FromPair( const Vector3<T>& a, const Vector3<T>& b ) {
		min.x = Min( a.x, b.x );
		min.y = Min( a.y, b.y );
		min.z = Min( a.z, b.z );
		max.x = Max( a.x, b.x );
		max.y = Max( a.y, b.y );
		max.z = Max( a.z, b.z );
	}

	/// Set all the members to zero.
	void Zero() {
		min.x = min.y = max.x = max.y = min.z = max.z = (T) 0;
	}

	/// Return true if the rectangles intersect.
	bool Intersect( const Rectangle3<T>& rect ) const
	{
		if (	rect.max.x < min.x
			 || rect.min.x > max.x
			 || rect.max.y < min.y
			 || rect.min.y > max.y
			 || rect.max.z < min.z
			 || rect.min.z > max.z )
		{
			return false;
		}
		return true;
	}	

	bool Intersect( const Vector3<T>& point ) const
	{
		if (	point.x < min.x
			 || point.x > max.x
			 || point.y < min.y
			 || point.y > max.y
			 || point.z < min.z
			 || point.z > max.z )
		{
			return false;
		}
		return true;
	}

	bool Intersect( T x, T y, T z ) const
	{
		if (	x < min.x
			 || x > max.x
			 || y < min.y
			 || y > max.y
			 || z < min.z
			 || z > max.z )
		{
			return false;
		}
		return true;
	}


	Vector3< T > Center() const {
		Vector3< T > v = { (min.x + max.x) / (T)2, (min.y + max.y) / (T)2, (min.z + max.z) / (T)2 };
		return v;
	}

	/// Return true if 'rect' is inside this.
	bool Contains( const Rectangle3<T>& rect ) const
	{
		if (	rect.min.x >= min.x
			 && rect.max.x <= max.x
			 && rect.min.y >= min.y
			 && rect.max.y <= max.y
			 && rect.min.z >= min.z
			 && rect.max.z <= max.z )
		{
			return true;
		}
		return false;
	}

	bool Contains( const Vector3<T>& point ) const
	{
		if (	point.x >= min.x
			 && point.x <= max.x
			 && point.y >= min.y
			 && point.y <= max.y
			 && point.z >= min.z
			 && point.z <= max.z )
		{
			return true;
		}
		return false;
	}

	/// Merge the rect into this.
	void DoUnion( const Rectangle3<T>& rect )
	{
		min.x = grinliz::Min( min.x, rect.min.x );
		max.x = grinliz::Max( max.x, rect.max.x );
		min.y = grinliz::Min( min.y, rect.min.y );
		max.y = grinliz::Max( max.y, rect.max.y );
		min.z = grinliz::Min( min.z, rect.min.z );
		max.z = grinliz::Max( max.z, rect.max.z );
	}

	/// Merge the Vector into this.
	void DoUnion( const Vector3<T>& vec )
	{
		min.x = grinliz::Min( min.x, vec.x );
		max.x = grinliz::Max( max.x, vec.x );
		min.y = grinliz::Min( min.y, vec.y );
		max.y = grinliz::Max( max.y, vec.y );
		min.z = grinliz::Min( min.z, vec.z );
		max.z = grinliz::Max( max.z, vec.z );
	}
	
	/// Turn this into the intersection.
	void DoIntersection( const Rectangle2<T>& rect )
	{
		min.x = grinliz::Max( min.x, rect.min.x );
		max.x = grinliz::Min( max.x, rect.max.x );
		min.y = grinliz::Max( min.y, rect.min.y );
		max.y = grinliz::Min( max.y, rect.max.y );
		min.z = grinliz::Max( min.z, rect.min.z );
		max.z = grinliz::Min( max.z, rect.max.z );
	}

	/// Scale all coordinates by the given ratios:
	void Scale( T x, T y, T z )
	{
		min.x = ( x * min.x );
		max.x = ( x * max.x );
		min.y = ( y * min.y );
		max.y = ( y * max.y );
		min.z = ( z * min.z );
		max.z = ( z * max.z );
	}

	/// Changes the boundaries
	void EdgeAdd( T i )
	{
		min.x -= i;
		max.x += i;
		min.y -= i;
		max.y += i;
		min.z -= i;
		max.z += i;
	}

	bool operator==( const Rectangle2<T>& that ) const { return	min == that.min && max == that.max; }
	bool operator!=( const Rectangle2<T>& that ) const { return	min != that.min || max != that.max; }
};



struct Rectangle3I : public Rectangle3< int >
{
	Rectangle3I() { min.Zero(); max.Zero(); }
	Rectangle3I( const Rectangle3I& rhs ) { *this = rhs; }
	Rectangle3I( int _xmin, int _ymin, int _zmin, int _xmax, int _ymax, int _zmax )	{ 
		min.Set( _xmin, _ymin, _zmin );
		max.Set( _xmax, _ymax, _zmax );
	} 
	enum { INVALID = INT_MIN };

	int SizeX()	 const 	{ return max.x - min.x + 1; }		///< width of the rectangle
	int SizeY() const	{ return max.y - min.y + 1; }		///< height of the rectangle
	int SizeZ()  const  { return max.z - min.z + 1; }
	int Volume() const	{ return SizeX() * SizeY() * SizeZ();	}   ///< Volume of the 3D rectangle
	int Size( int i ) const { return max.X(i) - min.X(i) + 1; }
	#ifdef DEBUG
	void Dump() { GLOUTPUT(( "(%d,%d,%d)-(%d,%d,%d)", min.x, min.y, min.z, max.x, max.y, max.z )); }
	#endif
};


struct Rectangle3F : public Rectangle3< float >
{
	Rectangle3F() { min.Zero(); max.Zero(); }
	Rectangle3F( const Rectangle3F& rhs ) { *this = rhs; }
	Rectangle3F( float _xmin, float _ymin, float _zmin, float _xmax, float _ymax, float _zmax )	{ 
		min.Set( _xmin, _ymin, _zmin );
		max.Set( _xmax, _ymax, _zmax );
	} 
	float SizeX() const 	{ return max.x - min.x; }		///< width of the rectangle
	float SizeY() const		{ return max.y - min.y; }		///< height of the rectangle
	float SizeZ() const		{ return max.z - min.z; }		///< depth of the rectangle
	float Volume()const		{ return SizeX() * SizeY() * SizeZ();	}   ///< Volume of the 3D rectangle
	float Size( int i ) const { return max.X(i) - min.X(i); }

	#ifdef DEBUG
	void Dump() { 
		GLOUTPUT(( "(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f)", min.x, min.y, min.z, max.x, max.y, max.z )); 
	}
	#endif
};




template< class T >
struct Rect2
{
	T x, y, w, h;

	Rect2() : x(0), y(0), w(0), h(0) {}
	Rect2( const Rectangle2<T>& rhs ) { *this = rhs; }
	Rect2( T _x, T _y, T _w, T _h ) : x( _x ), y( _y ), w( _w ), h( _h ) {}

	void Set( T _x, T _y, T _w, T _h ) {
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

