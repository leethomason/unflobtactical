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

#include "gpustatemanager.h"
#include "camera.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glmatrix.h"
#include "serialize.h"

using namespace grinliz;

Camera::Camera() : 
	tilt( 45.f ), 
	yRotation( 0.0f ), 
	valid( false )
{
	posWC.Set( 0.f, 0.f, 0.f );
}


void Camera::CalcWorldXForm()
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
		CalcEyeDir();

		// Calc the View Matrix
		Matrix4 inv, zRot;
		worldXForm.Invert( &inv );
		//zRot.SetZRotation( -(float)(viewRotation)*90.0f );
		viewMatrix = zRot*inv;

		valid = true;
	}
}


void Camera::CalcEyeDir()
{
	Vector4F dir[3] = {
		{ 0.0f, 0.0f, -1.0f, 0.0f },	// ray
		{ 0.0f, 1.0f, 0.0f, 0.0f },		// up (y axis)
		{ 1.0f, 0.0f, 0.0f, 0.0f },		// right (x axis)
	};

	for( int i=0; i<3; ++i ) {
		eyeDir[i] = worldXForm * dir[i];
		eyeDir3[i].x = eyeDir[i].x;
		eyeDir3[i].y = eyeDir[i].y;
		eyeDir3[i].z = eyeDir[i].z;
	}
}


void Camera::Orbit( float rotation )
{
	// Find the rotation point (the pole)
	EyeDir();			// bring cache current
	Vector3F pole;
	if ( IntersectRayPlane( posWC, eyeDir3[0], 1, 0, &pole ) == INTERSECT ) {

		Matrix4 r;
		r.SetYRotation( yRotation + rotation );

		float length = sqrtf( (pole.x-posWC.x)*(pole.x-posWC.x) + (pole.z-posWC.z)*(pole.z-posWC.z) );

		Vector3F vec = { 0.0f, 0.0f, length };
		Vector3F vecPrime = r * vec;
		vecPrime.y = posWC.y;

		posWC = pole + vecPrime;

		yRotation += rotation;
	}
	valid = false;
}
