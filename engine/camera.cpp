#include "platformgl.h"
#include "camera.h"
#include "../grinliz/glgeometry.h"

using namespace grinliz;

Camera::Camera() : 
	tilt( 45.f ), 
	yRotation( 0.0f ), 
	viewRotation( 0 )
{
	posWC.Set( 0.f, 0.f, 0.f );
}


void Camera::CalcWorldXForm( Matrix4* xform )
{
	Matrix4 translation, rotationY, rotationTilt;

	/* Test values:
	translation.SetTranslation( -5.0f, 5.0f, -5.0f );
	rotationY.SetYRotation( 225.0f );
	rotationTilt.SetXRotation( -5.0f );
	*/

	translation.SetTranslation( posWC );
	rotationY.SetYRotation( yRotation );
	rotationTilt.SetXRotation( tilt );

	// Done in world: we tilt it down, turn it around y, then move it.
	Matrix4 world = translation * rotationY * rotationTilt;
	*xform = world;
}


void Camera::DrawCamera()
{
	Matrix4 world;
	CalcWorldXForm( &world );

	Matrix4 m, view;
	world.Invert( &m );

	view.SetZRotation( (float)(viewRotation)*90.0f );
	m = view * m;

	glMultMatrixf( m.x );
}


void Camera::CalcEyeRay( Ray* ray )
{
	ray->origin = posWC;
	ray->length = 1.0f;
	
	Matrix4 m;
	CalcWorldXForm( &m );
	Vector4F v = { 0.0f, 0.0f, -1.0f, 0.0f };
	Vector4F vOut = m * v;

	ray->direction.x = vOut.x;
	ray->direction.y = vOut.y;
	ray->direction.z = vOut.z;
}

