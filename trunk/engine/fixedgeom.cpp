#include "fixedgeom.h"


void PlaneX::Convert( const grinliz::Plane& plane )
{
	n.x = FloatToFixed( plane.n.x );
	n.y = FloatToFixed( plane.n.y );
	n.z = FloatToFixed( plane.n.z );
	d   = FloatToFixed( plane.d );
}



FIXED PlanePointDistanceSquared( const PlaneX& plane, const Vector3X& point )
{
	// http://mathworld.wolfram.com/Point-PlaneDistance.html
	FIXED num =   FixedMul( plane.n.x, point.x ) 
				+ FixedMul( plane.n.y, point.y )
				+ FixedMul( plane.n.z, point.z )
				+ plane.d;
	FIXED denom =   FixedMul( plane.n.x, plane.n.x )
				  + FixedMul( plane.n.y, plane.n.y )
				  + FixedMul( plane.n.z, plane.n.z );

	FIXED d2 = FixedDiv( FixedMul( num, num ), denom );
	return d2;
}


int ComparePlaneSphereX( const PlaneX& plane, const SphereX& sphere )
{
	FIXED d2 = PlanePointDistanceSquared( plane, sphere.origin );
	FIXED r2 = FixedMul( sphere.radius, sphere.radius );

	if ( d2 <= 0 ) {
		if ( d2 > r2 ) {
			return grinliz::POSITIVE;
		}
	}
	else if ( d2 < -r2 ) {
		return grinliz::NEGATIVE;
	}
	return grinliz::INTERSECT;
}


int IntersectRaySphereX(	const SphereX& sphere,
							const Vector3X& p,
							const Vector3X& dir,
							FIXED* t )
{
	Vector3X raySphere = sphere.origin - p;
	FIXED raySphereLen2 = DotProductX( raySphere, raySphere );
	FIXED sphereR2 = FixedMul( sphere.radius, sphere.radius );

	if (raySphereLen2 < sphereR2) 
	{	
		// Origin is inside the sphere.
		return grinliz::INSIDE;
	} 
	else 
	{
		// Clever idea: what is the rays closest approach to the sphere?
		// see: http://www.devmaster.net/wiki/Ray-sphere_intersection

		FIXED closest = DotProductX(raySphere, dir);
		if (closest < 0) {
			// Then we are pointing away from the sphere (and we know we aren't inside.)
			return grinliz::REJECT;
		}
		FIXED halfCordLen = FixedDiv( (sphereR2 - raySphereLen2), DotProductX(dir, dir)) + (closest*closest);
		if ( halfCordLen > 0 ) {
			*t = closest - FixedSqrt( halfCordLen );
			return grinliz::INTERSECT;
		}
	}
	return grinliz::REJECT;
}


int ComparePlaneAABBX( const PlaneX& plane, const Rectangle3X& aabb )
{
	// Only need to check the most positive and most negative point,
	// or minimum and maximum depending on point of view. The octant
	// of the plane normal implies a vertex to check.

	Vector3X posPoint, negPoint;

	if ( plane.n.x > 0.0f )	{
		posPoint.x = aabb.max.x;
		negPoint.x = aabb.min.x;
	} else {
		posPoint.x = aabb.min.x;
		negPoint.x = aabb.max.x;
	}

	if ( plane.n.y > 0.0f )	{
		posPoint.y = aabb.max.y;
		negPoint.y = aabb.min.y;
	} else {
		posPoint.y = aabb.min.y;
		negPoint.y = aabb.max.y;
	}

	if ( plane.n.z > 0.0f ) {
		posPoint.z = aabb.max.z;
		negPoint.z = aabb.min.z;
	} else {
		posPoint.z = aabb.min.z;
		negPoint.z = aabb.max.z;
	}		

	// RTR 735
	// f(P) = N*P + d

	// If the most negative point is still on the positive
	// side, it is a positive, and vice versa.
	FIXED fp = DotProductX( plane.n, negPoint ) + plane.d;
	if ( fp > 0 )
	{
		return grinliz::POSITIVE;
	}

	fp = DotProductX( plane.n, posPoint ) + plane.d;
	if ( fp < 0 )
	{
		return grinliz::NEGATIVE;
	}
	return grinliz::INTERSECT;
}


int IntersectRayAABBX( const Vector3X& origin, 
						const Vector3X& dir,
						const Rectangle3X& aabb,
						Vector3X* intersect,
						FIXED* t )
{
	enum
	{
		RIGHT,
		LEFT,
		MIDDLE
	};

	bool inside = true;
	int quadrant[3];
	int i;
	int whichPlane;
	FIXED maxT[3];
	FIXED candidatePlane[3];

	const FIXED *pOrigin = &origin.x;
	const FIXED *pBoxMin = &aabb.min.x;
	const FIXED *pBoxMax = &aabb.max.x;
	const FIXED *pDir    = &dir.x;
	FIXED *pIntersect = &intersect->x;

	// Find candidate planes
	for (i=0; i<3; ++i)
	{
		if( pOrigin[i] < pBoxMin[i] ) 
		{
			quadrant[i] = LEFT;
			candidatePlane[i] = pBoxMin[i];
			inside = false;
		}
		else if ( pOrigin[i] > pBoxMax[i] ) 
		{
			quadrant[i] = RIGHT;
			candidatePlane[i] = pBoxMax[i];
			inside = false;
		}
		else	
		{
			quadrant[i] = MIDDLE;
		}
	}

	// Ray origin inside bounding box
	if( inside )	
	{
		*intersect = origin;
		*t = 0;
		return grinliz::INSIDE;
	}


	// Calculate T distances to candidate planes
	for (i = 0; i < 3; i++)
	{
		if (   quadrant[i] != MIDDLE 
		    && ( pDir[i] > FIXED_EPSILON || pDir[i] < -FIXED_EPSILON ) )
		{
			maxT[i] = FixedDiv( ( candidatePlane[i]-pOrigin[i] ), pDir[i] );
		}
		else
		{
			maxT[i] = -FIXED_1;
		}
	}

	// Get largest of the maxT's for final choice of intersection
	whichPlane = 0;
	for (i = 1; i < 3; i++)
		if (maxT[whichPlane] < maxT[i])
			whichPlane = i;

	// Check final candidate actually inside box
	if ( maxT[whichPlane] < 0 )
		return grinliz::REJECT;

	for (i = 0; i < 3; i++)
	{
		if (whichPlane != i ) 
		{
			pIntersect[i] = pOrigin[i] + FixedMul( maxT[whichPlane], pDir[i] );
			if (pIntersect[i] < pBoxMin[i] || pIntersect[i] > pBoxMax[i])
				return grinliz::REJECT;
		} 
		else 
		{
			pIntersect[i] = candidatePlane[i];
		}
	}
	*t = maxT[whichPlane];
	return grinliz::INTERSECT;
}	
