/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#if 0
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
	void Dump() {
		GLOUTPUT(( "origin=(%.2f,%.2f,%.2f) radius=%.2f\n", 
					(float)origin.x, (float)origin.y, (float)origin.z, (float)radius ));
	}
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
#endif