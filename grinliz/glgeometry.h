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



#ifndef GRINLIZ_GEOMETRY_INCLUDED
#define GRINLIZ_GEOMETRY_INCLUDED

#include "glvector.h"
#include "glrectangle.h"
#include "glmatrix.h"

#include <vector>

namespace grinliz {

/**
	Convert in and out of spherical coordinates. The coordinates are oddly defined.
	'theta' is the rotation about y, while
	'rho' is the rotation up.
	
*/
struct SphericalXZ
{
	float theta;
	float rho;
	float r;

	void FromCartesion( const Vector3F& );
	void ToCartesian( Vector3F *out );
	void ToCartesian( Matrix4* out );
};


/// Cross Product
template< class T>
inline void CrossProduct( const Vector2<T>& u, const Vector2<T>& v, Vector3<T>* w )
{
	w->x = 0.0f;
	w->y = 0.0f;
	w->z = u.x * v.y - u.y * v.x;
}

/// Cross Product
template< class T>
inline T CrossProductZ( const Vector2<T>& u, const Vector2<T>& v )
{
	return u.x * v.y - u.y * v.x;
}


/// Dot Product
template< class T>
inline T DotProduct( const Vector2<T>& v0, const Vector2<T>& v1 )
{
	return v0.x*v1.x + v0.y*v1.y;
}


/// Cross Product
template< class T>
inline void CrossProduct( const Vector3<T>& u, const Vector3<T>& v, Vector3<T>* w )
{
	GLASSERT( &u != w && &v != w );

	w->x = u.y * v.z - u.z * v.y;
	w->y = u.z * v.x - u.x * v.z;
	w->z = u.x * v.y - u.y * v.x;
}


/// Cross Product
template< class T>
inline Vector3<T> CrossProduct( const Vector3<T>& u, const Vector3<T>& v )
{
	Vector3<T> w;

	w.x = u.y * v.z - u.z * v.y;
	w.y = u.z * v.x - u.x * v.z;
	w.z = u.x * v.y - u.y * v.x;
	return w;
}


/// Dot Product
template< class T>
inline T DotProduct( const Vector3<T>& v0, const Vector3<T>& v1 )
{
	return v0.x*v1.x + v0.y*v1.y + v0.z*v1.z;
}


struct BoundingSphere
{
	Vector3F	point;
	float		rad;

};


/** Used to define a ray with an origin, direction, and length.
*/
struct Ray
{
	Vector3F	origin;
	Vector3F	direction;	// should always have a length of 1.0f
	float		length;
};

/** Plane
	n * p + d = 0
	ax + by + cz + d = 0
*/
struct Plane
{
	/// Create a plane from 3 points.
	static void CreatePlane( const Vector3F* vertices, Plane* plane );

	Vector3F	n;	///< normal
	float		d;	///< offset

	float Z( float x, float y ) const { return -( d + (n.x * x) + (n.y * y) ) / ( n.z ); }

	void Normalize()
	{
		float div = 1.0f / sqrtf( ( n.x * n.x ) + ( n.y * n.y ) + ( n.z * n.z ) );
		n.x *= div;
		n.y *= div;
		n.z *= div;
		d   *= div;
	}

	/// Projects the given vector onto the plain and creates a normal vector in the plane.
	void ProjectToPlane( const Vector3F& vector, Vector3F* projected ) const;
};

float CalcSphereTexture( float x, float y, bool roundHigh );

enum TessellateSphereMode
{
	TESS_SPHERE_NORMAL,
	TESS_SPHERE_TOPHALF,
	TESS_SPHERE_MIRROR,
};

/** Create a tessellation of a sphere by octrahedron subdivision. Creates a very smooth 
	sphere that is not biased to any axis. The returned triangulation is indexed triangles.

	@param iterations	Number of subdivisions. Typically in the range of 1-3.
	@param radius		Radius of the sphere.
	@param innerNormals	If true, the normals will face inside, for rendering the sphere 
						around the camera. Useful for creating a skydome.
	@param vertex		Pointer to a vector for vertices (required)
	@param index		Pointer to a vector for indices (required)
	@param normal		Pointer to a vector for normals (optional)
	@param texture		Generate texture coordinates (optional)
	@param mode			Controls texture coordinate generation.
*/
void TessellateSphere(	int iterations, float radius, bool innerNormals, 
						std::vector< Vector3F >* vertex, 
						std::vector< U32 >* index, 
						std::vector< Vector3F >* normal = 0,
						std::vector< Vector2F >* texture = 0,
						TessellateSphereMode mode = TESS_SPHERE_NORMAL );

/**
	Create a tessellated cube.
*/
void TessellateCube(	const Vector3F& center,
						const Vector3F& size,
						std::vector< Vector3F >* _vertex, 
						std::vector< U32 >* _index,
						std::vector< Vector3F >* _normal,
						std::vector< Vector2F >* _texture );

/** A point in a LineLoop. See LineLoop for a full description.
*/
struct LineNode
{
	grinliz::Vector2F	point;		// the point on the line
	LineNode*	prev;
	LineNode*	next;
	float		value;				// optional interpolation info

	LineNode( const grinliz::Vector2F& p ) : point( p ), prev( 0 ), next( 0 ), value( 0.0f ) {}
	LineNode( float x, float y ) : prev( 0 ), next( 0 ), value( 0.0f ) { point.Set( x, y ); }
	LineNode( float x, float y, float val ) : prev( 0 ), next( 0 ), value( val ) { point.Set( x, y ); }
	~LineNode() {}
};


/** A representation of a closed 2D polygon as a series of points.
	The "loop" is a true circular
	list - there are no nulls or sentinels. The next pointer is positive
	(counter-clock) and the prev pointer is negative (clock).
*/
class LineLoop
{
public:
	/// Construct an empty loop.
	LineLoop()	: first( 0 )	{}
	~LineLoop()	{ Clear(); }

	/// Clear the loop and free the memory for the lines.
	void Clear();
	/// Delete a point.
	void Delete( LineNode* del );
	/** Add a point at the end of the loop. Normally a loop is created by:

		@verbatim
		frustum2D.Clear();
		frustum2D.AddAtEnd( new LineNode( 0.0f, 0.0f ) );
		frustum2D.AddAtEnd( new LineNode( (float)(VERTEXSIZE-1), 0.0f ) );
		frustum2D.AddAtEnd( new LineNode( (float)(VERTEXSIZE-1), (float)(VERTEXSIZE-1) ) );
		frustum2D.AddAtEnd( new LineNode( 0.0f, (float)(VERTEXSIZE-1) ) );
		@endverbatim
	*/
	void AddAtEnd( LineNode* node );
	/// Add a pointer after the point specified.
	void AddAfter( LineNode* me, LineNode* add );
	/// Return the first point - null if empty.
	LineNode* First()	{ return first; }
	/// Return the first point - null if empty.
	const LineNode* First() const	{ return first; }
	/** Sort so the positive direction will the left edge,
		and the negitive direction the right. First() will point
		to the first left edge.
	*/
	void SortToTop();
	/// Compute the bounds of the loop.
	void Bounds( grinliz::Rectangle2F* bounds );

	/** Draw the lineloop to a floating point surface, using
		'value' in the line node. Note that only fully included
		points will be drawn.
	*/
	void Render( float* surface, int width, int height, bool fill=false );

	#ifdef DEBUG
	void Dump();
	#endif
 
private:
	LineNode* first;
};


/**
	Create a mini vertex array for a quad. 
	- The X0 term is the pattern: 0, 1, 1, 0
	- The X1 term is the pattern: 0, 0, 1, 1
*/
template< int X0, int X1, int XC >
struct Quad3F
{
	enum { NUM_VERTEX = 4 };
	Vector3F vertex[NUM_VERTEX];

	Quad3F( float x0, float y0, float x1, float y1, float c ) 
	{
		vertex[0].X(X0) = x0;	vertex[0].X(X1) = y0;	vertex[0].X(XC) = c;
		vertex[1].X(X0) = x1;	vertex[1].X(X1) = y0;	vertex[1].X(XC) = c;
		vertex[2].X(X0) = x1;	vertex[2].X(X1) = y1;	vertex[2].X(XC) = c;
		vertex[3].X(X0) = x0;	vertex[3].X(X1) = y1;	vertex[3].X(XC) = c;
	}
};

typedef Quad3F< 0, 1, 2 > Quad3Fz;


/**
	Create a mini vertex array for a quad. 
	- The X0 term is the pattern: 0, 1, 1, 0
	- The X1 term is the pattern: 0, 0, 1, 1
*/
template< int X0, int X1 >
struct Quad2F
{
	enum { NUM_VERTEX = 4 };
	Vector2F vertex[NUM_VERTEX];

	Quad2F( float x0, float y0, float x1, float y1 ) 
	{
		vertex[0].X(X0) = x0;	vertex[0].X(X1) = y0;
		vertex[1].X(X0) = x1;	vertex[1].X(X1) = y0;
		vertex[2].X(X0) = x1;	vertex[2].X(X1) = y1;
		vertex[3].X(X0) = x0;	vertex[3].X(X1) = y1;
	}
};

typedef Quad2F< 0, 1 > Quad2Fz;



/** A quaternion class. Whether these are useful or not is still up to
	debate, but they are certainly getting more common. Lilith uses
	them for the camera.
*/
class Quaternion
{
public:
	Quaternion() : x( 0.0f ), y( 0.0f ), z( 0.0f ), w( 1.0f )	{}

	void Normalize();

	const void ToAxisAngle( Vector3F* axis, float* angleOut ) const;
	const void ToMatrix( Matrix4* matrix ) const;
	void FromRotationMatrix( const Matrix4& matrix );
	void FromAxisAngle( const Vector3F& axis, float angle );

	static void SLERP( const Quaternion& start, const Quaternion& end, float t, Quaternion* result );
	static void Multiply( const Quaternion& a, const Quaternion& b, Quaternion* result );

	float x, y, z, w;
};


enum
{
	REJECT = 0,		// No intersection.
	INTERSECT,		// Intersected
	POSITIVE,		// All comparison positive
	NEGATIVE,		// All Comparison negative
	INSIDE,			// An intersection, expected to be outside of the object, is originating inside
};

enum 
{
	// Reordering would be bad. x=0, y=1, z=2
	YZ_PLANE,
	XZ_PLANE,
	XY_PLANE
};


/** @page intersection Intersection Functions

	Intersection functions use the following conventions:

	- Plane:	an infinite 3D plane
	- Ray:		point and direction, possibly a length
	- Gravray:	point with direction in the -z only, possibly a length
	- AABB:		axis aligned bounding box 
	- Tri:		triangle

	Intersection functions return:
	- REJECT		No intersection.
	- INTERSECT		Intersected. Usually the point of intersection is written to a structure.
	- POSITIVE,		A comparison was positive. 
	- NEGATIVE,		A Comparison was negative.
*/

/** Compare a plane to a Axis-Aligned Bounding Box. Has more detailed return
	types than IntersectPlaneAABB().
	@return INTERSECT, POSITIVE, NEGATIVE
*/
int ComparePlaneAABB( const Plane&, const Rectangle3F& aabb );

/** Compare a plane to a point.
	@return POSITIVE, NEGATIVE, (very unlikely) INTERSECT
*/
int ComparePlanePoint( const Plane&, const Vector3F& p );

/** Compare a plane to a Axis-Aligned Bounding Box.
	@return REJECT or INTERSECT
*/
inline int IntersectPlaneAABB( const Plane& p, const Rectangle3F& aabb )
{
	return ( ComparePlaneAABB( p, aabb ) == INTERSECT ) ? INTERSECT : REJECT; 
}


/** The very important triangle test. Will not return a back face intersection.
	The "intersect" pointer is an optional parameter.
	@return REJECT or INTERSECT
*/
int IntersectRayTri(	const Vector3F& point, const Vector3F& dir,
						const Vector3F& vert0, const Vector3F& vert1, const Vector3F& vert2,
						Vector3F* intersect );

/**	Internect a ray and a plane. The planeType can be X, Y, or Z (0, 1, 2), and
	have a corresponding location in space.
	@return REJECT or INTERSECT
*/
int IntersectRayAAPlane(	const Vector3F& point, const Vector3F& dir,
							int planeType, float planeLoc,
							Vector3F* intersect,
							float* t );
						 
/** Intersect a ray with an Axis-Aligned Bounding Box. If the origin of the ray
	is inside the box, the intersection point will be the origin and t=0.
	@return REJECT, INTERSECT, or INSIDE.
*/
int IntersectRayAABB(	const Vector3F& origin, const Vector3F& dir,
						const Rectangle3F& aabb,
						Vector3F* intersect,
						float* t );

/** Intersect a ray with an Axis-Aligned Bounding Box. If the origin of the ray
	is inside the box, the intersection point will be the origin and t=0. Also optionally
	returns 'planeIndex' (x=0,y,z) and 'quadrant' (RIGHT=0, LEFT, MIDDLE)
	@return REJECT, INTERSECT, or INSIDE.
*/
int IntersectRayAABB(	const Vector2F& origin, const Vector2F& dir,
						const Rectangle2F& aabb,
						Vector2F* intersect,
						float* t, int* planeIndex=0, int* quadrant=0 );

/** The ray and a bounding box form an intersection that is another bounding
	box. Very useful when you have a ray and a big box and want to know
	the smaller box of interest.
	@return REJECT, INTERSECT
*/
int IntersectionRayAABB(	const Ray& ray,
							const Rectangle3F& aabb,
							Rectangle3F* result );

/** Intersect a ray with the plane (at offset x.) Plane x=0, y=1, z=2.
	@return REJECT or INTERSECT
*/
int IntersectRayPlane(	const Vector3F& origin, const Vector3F& dir,
						int plane, float x,
						Vector3F* intersect );

/** Intersection of a line and a plane (general case).
	@return REJECT or INTERSECT. Note that INTERSECT may be true, but
			the 't' parameter is <0 or >1
*/
int IntersectLinePlane( const Vector3F& a0, const Vector3F& a1,
						const Plane& plane, 
						Vector3F* out,
						float* t );

/** Intersection of 3D lines. The return value is itself a line (out0 and out1)
    that is the shortest line between a and b. The return value is not limited
	by the endpoints.
	@return REJECT or INTERSECT.
*/
int IntersectLineLine(	const Vector3F& a0, const Vector3F& a1,
						const Vector3F& b0, const Vector3F& b1,
						Vector3F* out0, 
						Vector3F* out1 );

/** Intersection of 2D lines. The return value is a point. If the
	intersection is actually between the endpoints, t0 and t1 will
	be in the range [0,1]
	@return REJECT or INTERSECT.
*/
int IntersectLineLine(	const Vector2F& a0, const Vector2F& a1,
						const Vector2F& b0, const Vector2F& b1,
						Vector2F* out,
						float* t0,
						float* t1 );


/** Returns whether the point is in the *convex* polygon specified by vertex.
	@return REJECT or INTERSECT.
*/
int PointInPolygon( const Vector2F& p, const Vector2F* vertex, int numVertex );

/**	This is assumes the point *is* on the line. Given this, returns
	the parameterized distance (t) of pi between p0 and p1 )
*/
float PointBetweenPoints( const Vector3F& p0, const Vector3F& p1, const Vector3F& pi );

/**
    Intersection of 3 planes is a point. Returs INTERSECT if there is such
	a point, or REJECT if planes are parallel.
*/
int Intersect3Planes( const Plane& p0, const Plane& p1, const Plane& p2, Vector3F* intersection );

};	// namespace grinliz
#endif

