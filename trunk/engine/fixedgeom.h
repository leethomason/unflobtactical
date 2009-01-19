#ifndef FIXEDGEOM_INCLUDED
#define FIXEDGEOM_INCLUDED

#include "../grinliz/glgeometry.h"

typedef S32 FIXED;
typedef grinliz::Vector3< FIXED > Vector3X;
typedef grinliz::Vector2< FIXED > Vector2X;
typedef grinliz::Rectangle2< FIXED > Rectangle2X;
typedef grinliz::Rectangle3< FIXED > Rectangle3X;

struct SphereX
{
	Vector3X origin;
	FIXED radius;
};

struct PlaneX
{
	void Convert( const grinliz::Plane& plane );

	Vector3X	n;	///< normal
	FIXED		d;	///< offset
};



const FIXED FIXED_1   =    0x10000;
const FIXED FIXED_MAX = 0x7FFF0000;
const FIXED FIXED_EPSILON = (int)(0.000001f * 65536.0f);

inline FIXED FloatToFixed( float f ) {
	return grinliz::LRintf( f * 65536.0f );
}

inline FIXED FloatToFixed( double f ) {
	return grinliz::LRint( f * 65536.0 );
}

inline float FixedToFloat( S32 v ) {
	return (float)v / 65536.0f;
}

inline int32_t FixedToInt( FIXED f ) {
	return f>>16;
}

inline FIXED IntToFixed( int i ) {
	return i<<16;
}

inline int32_t FixedToIntCeil( FIXED f ) {
	return ( f + FIXED_1 - 1 ) >> 16;
}

inline FIXED FixedMul( FIXED a, FIXED b ) {
	int64_t c = (int64_t)a * (int64_t)b;
	return (FIXED)(c>>16);
}

inline FIXED FixedDiv( FIXED a, FIXED b ) {
	int64_t c = ((int64_t)(a)<<32) / ((int64_t)(b)<<16);
	return (FIXED) c;
}

inline FIXED FixedMean( FIXED a, FIXED b ) {
	return (a + b)>>1;
}

inline FIXED FixedSqrt( FIXED a ) {
	return FloatToFixed( sqrtf( FixedToFloat( a ) ) );
}


inline void ConvertVector3( const grinliz::Vector3F& f, Vector3X* v ) 
{
	v->x = FloatToFixed( f.x );
	v->y = FloatToFixed( f.y );
	v->z = FloatToFixed( f.z );
}


inline FIXED DotProductX( const Vector3X& v0, const Vector3X& v1 )
{
	return FixedMul( v0.x, v1.x ) + FixedMul( v0.y, v1.y ) + FixedMul( v0.z, v1.z );
}


FIXED PlanePointDistanceSquared( const PlaneX& plane, const Vector3X& point );
int ComparePlaneAABBX( const PlaneX& plane, const Rectangle3X& aabb );

/*	Intersect a ray with a sphere.
	@return REJECT, INSIDE, INTERSECT
*/
int IntersectRaySphereX(	const SphereX& sphere,
							const Vector3X& p,			// ray origin
							const Vector3X& dir,		// does not need to be unit.
							FIXED* t );

						 
/** Intersect a ray with an Axis-Aligned Bounding Box. If the origin of the ray
	is inside the box, the intersection point will be the origin and t=0.
	@return REJECT, INTERSECT, or INSIDE.
*/
int IntersectRayAABBX(	const Vector3X& origin, const Vector3X& dir,
						const Rectangle3X& aabb,
						Vector3X* intersect,
						FIXED* t );

/** Compare a sphere to a Axis-Aligned Bounding Box.
	@return INTERSECT, POSITIVE, NEGATIVE
*/
int ComparePlaneSphereX( const PlaneX& plane, const SphereX& sphere );

#endif // FIXEDGEOM_INCLUDED