#ifndef FIXEDGEOM_INCLUDED
#define FIXEDGEOM_INCLUDED

#include "../grinliz/glfixed.h"
#include "../grinliz/glgeometry.h"

typedef grinliz::Vector3< grinliz::Fixed > Vector3X;
typedef grinliz::Vector2< grinliz::Fixed > Vector2X;

typedef grinliz::Rectangle2< grinliz::Fixed > Rectangle2X;
typedef grinliz::Rectangle3< grinliz::Fixed > Rectangle3X;

void DumpRectangle( const Rectangle3X& r );
void DumpRectangle( const Rectangle2X& r );

struct SphereX
{
	Vector3X origin;
	grinliz::Fixed radius;
};

struct CircleX
{
	Vector2X origin;
	grinliz::Fixed radius;
};

struct PlaneX
{
	void Convert( const grinliz::Plane& plane );

	Vector3X	n;	///< normal
	grinliz::Fixed		d;	///< offset
};


inline void ConvertVector3( const grinliz::Vector3F& f, Vector3X* v ) 
{
	v->x = f.x;
	v->y = f.y;
	v->z = f.z;
}

inline void ConvertVector3( const Vector3X& f, grinliz::Vector3F* v ) 
{
	v->x = f.x;
	v->y = f.y;
	v->z = f.z;
}


grinliz::Fixed PlanePointDistanceSquared( const PlaneX& plane, const Vector3X& point );
int ComparePlaneAABBX( const PlaneX& plane, const Rectangle3X& aabb );

/*	Intersect a ray with a sphere.
	@return REJECT, INSIDE, INTERSECT
*/
int IntersectRaySphereX(	const SphereX& sphere,
							const Vector3X& p,			// ray origin
							const Vector3X& dir,		// does not need to be unit.
							grinliz::Fixed* t );

						 
/** Intersect a ray with an Axis-Aligned Bounding Box. If the origin of the ray
	is inside the box, the intersection point will be the origin and t=0.
	@return REJECT, INTERSECT, or INSIDE.
*/
int IntersectRayAABBX(	const Vector3X& origin, const Vector3X& dir,
						const Rectangle3X& aabb,
						Vector3X* intersect,
						grinliz::Fixed* t );

/** Compare a sphere to a Axis-Aligned Bounding Box.
	@return INTERSECT, POSITIVE, NEGATIVE
*/
int ComparePlaneSphereX( const PlaneX& plane, const SphereX& sphere );

#endif // FIXEDGEOM_INCLUDED