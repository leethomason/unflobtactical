#include "platformgl.h"
#include "camera.h"
#include "../grinliz/glgeometry.h"

using namespace grinliz;

Camera::Camera() : 
	tilt( 45.f ), 
	yRotation( 0.0f ), 
	viewRotation( 0 ),
	valid( false )
{
	posWC.Set( 0.f, 0.f, 0.f );
}


void Camera::CalcWorldXForm( Matrix4* xform )
{
	if ( !valid ) {
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
		worldXForm = translation * rotationY * rotationTilt;
		valid = true;
	}
	*xform = worldXForm;
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


void Camera::CalcEyeRay( Ray* ray, Ray* up, Ray* right )
{
	
	Matrix4 m;
	CalcWorldXForm( &m );

	Ray* rays[3] = { ray, up, right };
	Vector4F dir[3] = {
		{ 0.0f, 0.0f, -1.0f, 0.0f },	// ray
		{ 0.0f, 1.0f, 0.0f, 0.0f },		// up (y axis)
		{ 1.0f, 0.0f, 0.0f, 0.0f },		// right (x axis)
	};

	for( int i=0; i<3; ++i ) {
		Ray* r = rays[i];
		if ( r ) {
			r->origin = posWC;
			r->length = 1.0f;

			Vector4F vOut = m * dir[i];

			r->direction.x = vOut.x;
			r->direction.y = vOut.y;
			r->direction.z = vOut.z;
		}
	}
}

