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


#ifndef GRINLIZ_VECTOR_INCLUDED
#define GRINLIZ_VECTOR_INCLUDED

#include <math.h>
#include "gldebug.h"
#include "glmath.h"
#include "glutil.h"


namespace grinliz {

template< class T >
struct Vector2
{
	enum { COMPONENTS = 2 };

	T x;
	T y;

	T& X( int i )    		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }
	T  X( int i ) const		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }

	void Add( const Vector2<T>& vec ) {
		x += vec.x;
		y += vec.y;
	}

	void Subtract( const Vector2<T>& vec ) {
		x -= vec.x;
		y -= vec.y;
	}

	void Multiply( T scalar ) {
		x *= scalar;
		y *= scalar;
	}

	friend Vector2<T> operator-( const Vector2<T>& head, const Vector2<T>& tail ) {
		Vector2<T> vec = {
			vec.x = head.x - tail.x,
			vec.y = head.y - tail.y,
		};
		return vec;
	}

	friend Vector2<T> operator+( const Vector2<T>& head, const Vector2<T>& tail ) {
		Vector2<T> vec = {
			vec.x = head.x + tail.x,
			vec.y = head.y + tail.y,
		};
		return vec;
	}
		
	friend Vector2<T> operator*( Vector2<T> head, T scalar ) { head.Multiply( scalar ); return head; }
	friend Vector2<T> operator*( T scalar, Vector2<T> head ) { head.Multiply( scalar ); return head; }

	void operator+=( const Vector2<T>& vec )		{ Add( vec ); }
	void operator-=( const Vector2<T>& vec )		{ Subtract( vec ); }
	bool operator==( const Vector2<T>& rhs ) const	{ return (x == rhs.x) && (y == rhs.y); }
	bool operator!=( const Vector2<T>& rhs ) const	{ return (x != rhs.x) || (y != rhs.y); }

	friend bool Equal( const Vector2<T>& a, const Vector2<T>& b, T epsilon ) {
		return ( Equal( a.x, b.x, epsilon ) && Equal( a.y, b.y, epsilon ) );
	}

	int Compare( const Vector2<T>& r, T epsilon ) const
	{
		if ( Equal( this->x, r.x, epsilon ) ) {
			if ( Equal( this->y, r.y, epsilon ) ) {
				return 0;
			}
			else if ( this->y < r.y ) return -1;
			else					  return 1;
		}
		else if ( this->x < r.x ) return -1;
		else					  return 1;
	}	

	void Set( T _x, T _y ) {
		this->x = _x;
		this->y = _y;
	}
	void Zero() {
		x = (T)0;
		y = (T)0;
	}

	T Length() const { return grinliz::Length( x, y ); };

	friend T Length( const Vector2<T>& a, const Vector2<T>& b ) {
		return grinliz::Length( a.x-b.x, a.y-b.y );
	}

	void Normalize()	
	{ 
		T lenInv = static_cast<T>(1) / grinliz::Length( x, y );
		x *= lenInv; y *= lenInv;
		#ifdef DEBUG
		float len = x*x + y*y;
		GLASSERT( len > .9999f && len < 1.0001f );
		#endif
	}

	void RotatePos90()
	{
		T a = x;
		T b = y;
		x = -b;
		y = a;
	}
	void RotateNeg90()
	{
		T a = x;
		T b = y;
		x = b;
		y = -a;
	}
};

typedef Vector2< int > Vector2I;
typedef Vector2< float > Vector2F;


inline void DegreeToVector( float degree, Vector2<float>* vec )
{
	float rad = ToRadian( degree );
	vec->x = cosf( rad );
	vec->y = sinf( rad );
}


template< class T >
struct Line2
{
	Vector2<T>	n;	// normal
	T			d;		// offset

	T EvaluateX( T y )		{ return ( -d - y*n.y ) / n.x; }
	T EvaluateY( T x )		{ return ( -d - x*n.x ) / n.y; }
};

typedef Line2< float > Line2F;



template< class T >
struct Vector3
{
	enum { COMPONENTS = 3 };

	T x;
	T y;
	T z;

	// I avoid non-const references - but this one is just so handy! And, in
	// use in the code, it is clear it is an assignment.
	T& X( int i )    		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }
	T  X( int i ) const		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }

	void Add( const Vector3<T>& vec ) {
		x += vec.x;
		y += vec.y;
		z += vec.z;
	}

	friend void Add( const Vector3<T>& a, const Vector3<T>& b, Vector3<T>* c )
	{
		c->x = a.x + b.x;
		c->y = a.y + b.y;
		c->z = a.z + b.z;
	}

	void Subtract( const Vector3<T>& vec ) {
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
	}

	void Multiply( T scalar ) {
		x *= scalar;
		y *= scalar;
		z *= scalar;
	}

	friend Vector3<T> operator-( const Vector3<T>& head, const Vector3<T>& tail ) {
		Vector3<T> vec = {
			vec.x = head.x - tail.x,
			vec.y = head.y - tail.y,
			vec.z = head.z - tail.z
		};
		return vec;
	}

	friend Vector3<T> operator+( const Vector3<T>& head, const Vector3<T>& tail ) {
		Vector3<T> vec = {
			vec.x = head.x + tail.x,
			vec.y = head.y + tail.y,
			vec.z = head.z + tail.z,
		};
		return vec;
	}
	
	friend Vector3<T> operator*( Vector3<T> head, T scalar ) { head.Multiply( scalar ); return head; }
	friend Vector3<T> operator*( T scalar, Vector3<T> head ) { head.Multiply( scalar ); return head; }

	Vector3<T> operator-() const {
		Vector3<T> vec = { -x, -y, -z };
		return vec;
	}

	void operator+=( const Vector3<T>& vec )		{ Add( vec ); }
	void operator-=( const Vector3<T>& vec )		{ Subtract( vec ); }
	bool operator==( const Vector3<T>& rhs ) const	{ return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator!=( const Vector3<T>& rhs ) const	{ return x != rhs.x || y != rhs.y || z != rhs.z; }

	friend bool Equal( const Vector3<T>& a, const Vector3<T>& b, T epsilon ) {
		return ( Equal( a.x, b.x, epsilon ) && Equal( a.y, b.y, epsilon ) && Equal( a.z, b.z, epsilon ) );
	}

	int Compare( const Vector3<T>& r, T epsilon ) const
	{
		if ( Equal( this->x, r.x, epsilon ) ) {
			if ( Equal( this->y, r.y, epsilon ) ) {
				if ( Equal( this->z, r.z, epsilon ) ) {
					return 0;
				}
				else if ( this->z < r.z ) return -1;
				else					  return 1;
			}
			else if ( this->y < r.y ) return -1;
			else					  return 1;
		}
		else if ( this->x < r.x ) return -1;
		else					  return 1;
	}

	void Set( T _x, T _y, T _z ) {
		this->x = _x;
		this->y = _y;
		this->z = _z;
	}

	void Zero() {
		x = (T)0;
		y = (T)0;
		z = (T)0;
	}

	void Normalize()	
	{ 

		GLASSERT( grinliz::Length( x, y, z ) > 0.00001f );
		T lenInv = static_cast<T>(1) / grinliz::Length( x, y, z );
		x *= lenInv; 
		y *= lenInv;
		z *= lenInv;
		#ifdef DEBUG
		T len = x*x + y*y + z*z;
		GLASSERT( len > (T)(.9999) && len < (T)(1.0001) );
		#endif
	}

	T Length() const { return grinliz::Length( x, y, z ); };

	friend T Length( const Vector3<T>& a, const Vector3<T>& b ) {
		return grinliz::Length( a.x-b.x, a.y-b.y, a.z-b.z );
	}

	#ifdef DEBUG
	void Dump( const char* name ) const
	{
		GLOUTPUT(( "Vec%4s: %6.2f %6.2f %6.2f\n", name, (float)x, (float)y, (float)z ));
	}
	#endif
};


inline void DegreeToVector( float degree, Vector3<float>* vec )
{
	Vector2<float> v;
	DegreeToVector( degree, &v );
	vec->Set( v.x, v.y, 0.0f );
}

template< class T >
inline void Convert( const Vector3<T>& v3, Vector2<T>* v2 )
{
	v2->x = v3.x;
	v2->y = v3.y;
}


template< class T >
struct Vector4
{
	enum { COMPONENTS = 4 };

	T x;
	T y;
	T z;
	T w;

	// I avoid non-const references - but this one is just so handy! And, in
	// use in the code, it is clear it is an assignment.
	T& X( int i )    		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }
	T  X( int i ) const		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }

	void Add( const Vector4<T>& vec ) {
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
	}

	friend void Add( const Vector4<T>& a, const Vector4<T>& b, Vector4<T>* c )
	{
		c->x = a.x + b.x;
		c->y = a.y + b.y;
		c->z = a.z + b.z;
		c->w = a.w + b.w;
	}

	void Subtract( const Vector4<T>& vec ) {
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		w -= vec.w;
	}

	void Multiply( T scalar ) {
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;
	}

	friend Vector4<T> operator-( const Vector4<T>& head, const Vector4<T>& tail ) {
		Vector4<T> vec = {
			vec.x = head.x - tail.x,
			vec.y = head.y - tail.y,
			vec.z = head.z - tail.z,
			vec.w = head.w - tail.w
		};
		return vec;
	}

	friend Vector4<T> operator+( const Vector4<T>& head, const Vector4<T>& tail ) {
		Vector4<T> vec = {
			vec.x = head.x + tail.x,
			vec.y = head.y + tail.y,
			vec.z = head.z + tail.z,
			vec.w = head.w + tail.w,
		};
		return vec;
	}
	
	friend Vector4<T> operator*( Vector4<T> head, T scalar ) { head.Multiply( scalar ); return head; }
	friend Vector4<T> operator*( T scalar, Vector4<T> head ) { head.Multiply( scalar ); return head; }

	Vector4<T> operator-() const {
		Vector4<T> vec = { -x, -y, -z, -w };
		return vec;
	}

	void operator+=( const Vector4<T>& vec )		{ Add( vec ); }
	void operator-=( const Vector4<T>& vec )		{ Subtract( vec ); }
	bool operator==( const Vector4<T>& rhs ) const	{ return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
	bool operator!=( const Vector4<T>& rhs ) const	{ return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }

	void Set( T _x, T _y, T _z, T _w ) {
		this->x = _x;
		this->y = _y;
		this->z = _z;
		this->w = _w;
	}

	void Set( const Vector3<T>& v, T _w ) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = _w;
	}

	void Zero() {
		x = (T)0;
		y = (T)0;
		z = (T)0;
		w = (T)0;
	}

	void Normalize()	
	{ 

		GLASSERT( grinliz::Length( x, y, z, w ) > 0.00001f );
		T lenInv = static_cast<T>(1) / grinliz::Length( x, y, z, w );
		x *= lenInv; 
		y *= lenInv;
		z *= lenInv;
		w *= lenInv;
		#ifdef DEBUG
		T len = x*x + y*y + z*z + w*w;
		GLASSERT( len > (T)(.9999) && len < (T)(1.0001) );
		#endif
	}

	T Length() const { return grinliz::Length( x, y, z, w ); };

	friend T Length( const Vector4<T>& a, const Vector4<T>& b ) {
		return grinliz::Length( a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w );
	}
};


typedef Vector3< int > Vector3I;
typedef Vector3< float > Vector3F;

typedef Vector4< int > Vector4I;
typedef Vector4< float > Vector4F;


};	// namespace grinliz

#endif
