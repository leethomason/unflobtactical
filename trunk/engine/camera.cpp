#include "camera.h"
#include "../grinliz/glgeometry.h"

using namespace grinliz;

Camera::Camera() : valid( false ), tilt( 45.f )
{
	posWC.Set( 0.f, 0.f, 0.f );
}


void Camera::MakeValid()
{
	if ( !valid ) {
		Matrix4		rot, rot1, rot2;

		rot1.SetYRotation( 45.0f );
		rot2.SetZRotation( tilt );
		MultMatrix4( rot2, rot1, &rot );

		Vector3F x, y, z;

		// Make a rotation matrix
		// The camera looks down the (-z) axis. 
		// This algorithm (based on the gluLookAt code) uses
		// z = eye - center
		z.Set( -rot.m11, -rot.m12, -rot.m13 );	// Inverse of the axial direction
		
		y.Set( 0.0f, 1.0f, 0.0f );	// The up vector is positive z.

		// X = Y cross Z
		CrossProduct( y, z, &x );
		// Y = Z cross X
		CrossProduct( z, x, &y );

		x.Normalize();
		y.Normalize();
		z.Normalize();

		m.m11 = x.x;
		m.m12 = x.y;
		m.m13 = x.z;
		m.m14 = 0.0f;

		m.m21 = y.x;
		m.m22 = y.y;
		m.m23 = y.z;
		m.m24 = 0.0f;

		m.m31 = z.x;
		m.m32 = z.y;
		m.m33 = z.z;
		m.m34 = 0.0f;

		valid = true;
	}
}