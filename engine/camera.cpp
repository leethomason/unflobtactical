#include "camera.h"
#include "../grinliz/glgeometry.h"

using namespace grinliz;

Camera::Camera() : valid( false ), tilt( 45.f ), yRotation( 0.0f ), viewRotation( 0 )
{
	posWC.Set( 0.f, 0.f, 0.f );
}


void Camera::MakeValid()
{
	if ( !valid ) {
		Matrix4		rot, rotY, rotZ;

		rotY.SetYRotation( yRotation );
		rotZ.SetZRotation( tilt );

		rot = rotZ * rotY;

		Vector3F x, y, z;

		// Make a rotation matrix
		// The camera looks down the (-z) axis. 
		// This algorithm (based on the gluLookAt code) uses
		// z = eye - center
		z.Set( -rot.m11, -rot.m12, -rot.m13 );	// Inverse of the axial direction
		
		// The up direction.
		y.Set( 0.0f, 1.0f, 0.0f );
		
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

		Matrix4 vm;
		vm.SetZRotation( 90.0f * (float)viewRotation );

		m = vm * m;
		valid = true;
	}
}