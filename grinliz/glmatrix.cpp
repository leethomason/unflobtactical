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

#include "glmatrix.h"
#include "glgeometry.h"
#include "glrectangle.h"

using namespace grinliz;

void grinliz::SinCosDegree( float degreeTheta, float* sinTheta, float* cosTheta )
{
	degreeTheta = NormalizeAngleDegrees( degreeTheta );

	if ( degreeTheta == 0.0f ) {
		*sinTheta = 0.0f;
		*cosTheta = 1.0f;
	}
	else if ( degreeTheta == 90.0f ) {
		*sinTheta = 1.0f;
		*cosTheta = 0.0f;
	}
	else if ( degreeTheta == 180.0f ) {
		*sinTheta = 0.0f;
		*cosTheta = -1.0f;
	}
	else if ( degreeTheta == 270.0f ) {
		*sinTheta = -1.0f;
		*cosTheta = 0.0f;
	}
	else {
		float radian = ToRadian( degreeTheta );
		*sinTheta = sinf( radian );
		*cosTheta = cosf( radian );	
	}
	
}


void Matrix4::SetXRotation( float theta )
{
	float cosTheta, sinTheta;
	SinCosDegree( theta, &sinTheta, &cosTheta );

	// COLUMN 1
	x[0] = 1.0f;
	x[1] = 0.0f;
	x[2] = 0.0f;
	
	// COLUMN 2
	x[4] = 0.0f;
	x[5] = cosTheta;
	x[6] = sinTheta;

	// COLUMN 3
	x[8] = 0.0f;
	x[9] = -sinTheta;
	x[10] = cosTheta;

	// COLUMN 4
	x[12] = 0.0f;
	x[13] = 0.0f;
	x[14] = 0.0f;
}


void Matrix4::SetYRotation( float theta )
{
	float cosTheta, sinTheta;
	SinCosDegree( theta, &sinTheta, &cosTheta );

	// COLUMN 1
	x[0] = cosTheta;
	x[1] = 0.0f;
	x[2] = -sinTheta;
	
	// COLUMN 2
	x[4] = 0.0f;
	x[5] = 1.0f;
	x[6] = 0.0f;

	// COLUMN 3
	x[8] = sinTheta;
	x[9] = 0;
	x[10] = cosTheta;

	// COLUMN 4
	x[12] = 0.0f;
	x[13] = 0.0f;
	x[14] = 0.0f;
}

void Matrix4::SetZRotation( float theta )
{
	float cosTheta, sinTheta;
	SinCosDegree( theta, &sinTheta, &cosTheta );

	// COLUMN 1
	x[0] = cosTheta;
	x[1] = sinTheta;
	x[2] = 0.0f;
	
	// COLUMN 2
	x[4] = -sinTheta;
	x[5] = cosTheta;
	x[6] = 0.0f;

	// COLUMN 3
	x[8] = 0.0f;
	x[9] = 0.0f;
	x[10] = 1.0f;

	// COLUMN 4
	x[12] = 0.0f;
	x[13] = 0.0f;
	x[14] = 0.0f;
}


float Matrix4::CalcRotationAroundAxis( int axis ) const
{
	float theta = 0.0f;

	switch ( axis ) {
		case 0:		theta = atan2f( x[6], x[10] );		break;
		case 1:		theta = atan2f( x[8], x[0] );		break;
		case 2:		theta = atan2f( x[1], x[0] );		break;

		default:
			GLASSERT( 0 );
	}
	theta *= RAD_TO_DEG;
	if ( theta < 0.0f ) theta += 360.0f;

	return theta ;
}


void Matrix4::SetAxisAngle( const Vector3F& axis, float angle )
{
	angle *= DEG_TO_RAD;
	float c = (float) cos( angle );
	float s = (float) sin( angle );
	float t = 1.0f - c;

	x[0]  = t*axis.x*axis.x + c;
	x[1]  = t*axis.x*axis.y - s*axis.z;
	x[2]  = t*axis.x*axis.z + s*axis.y;
	x[3]  = 0.0f;

	x[4]  = t*axis.x*axis.y + s*axis.z;
	x[5]  = t*axis.y*axis.y + c;
	x[6]  = t*axis.y*axis.z - s*axis.x;
	x[7]  = 0.0f;

	x[8]  = t*axis.x*axis.z - s*axis.y;
	x[9]  = t*axis.y*axis.z + s*axis.x;
	x[10] = t*axis.z*axis.z + c;
	x[11] = 0.0f;

	x[12] = x[13] = x[14] = 0.0f;
	x[15] = 1.0f;
}


bool Matrix4::IsRotation() const
{
	// Check the rows and columns:

	for( int i=0; i<4; ++i )
	{
		float row = 0.0f;
		float col = 0.0f;
		for( int j=0; j<4; ++j )
		{
			row += x[i+j*4]*x[i+j*4];
			col += x[i*4+j]*x[i*4+j];
		}
		if ( !Equal( row, 1.0f, 0.0001f ) )
			return false;
		if ( !Equal( row, 1.0f, 0.0001f ) )
			return false;
	}
	// Should also really check orthogonality.
	return true;
}


void Matrix4::Transpose( Matrix4* transpose ) const
{
  for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
			transpose->x[INDEX(c,r)] = x[INDEX(r,c)];
        }
    }
}

float Matrix4::Determinant() const
{
	//	11 12 13 14
	//	21 22 23 24
	//	31 32 33 34
	//	41 42 43 44

	// Old school. Derived from Graphics Gems 1
	return m11 * Determinant3x3(	m22, m23, m24, 
									m32, m33, m34, 
									m42, m43, m44 ) -
		   m12 * Determinant3x3(	m21, m23, m24, 
									m31, m33, m34, 
									m41, m43, m44 ) +
		   m13 * Determinant3x3(	m21, m22, m24,
									m31, m32, m34, 
									m41, m42, m44 ) -
		   m14 * Determinant3x3(	m21, m22, m23, 
									m31, m32, m33, 
									m41, m42, m43 );

	/*
    // The determinant is the dot product of:
	// the first row and the first row of cofactors
	// which is the first col of the adjoint matrix
	Matrix4 cofactor;
	Cofactor( &cofactor );
	Vector3F rowOfCofactor, rowOfThis;
	Row( 0, &rowOfThis );

	float det = 0.0f;
	for( int c=0; c<4; ++c ) {
		cofactor.Row( c, &rowOfCofactor );
		det += DotProduct( rowOfCofactor, rowOfThis );
	}
	return det;
	*/
}


void Matrix4::Adjoint( Matrix4* adjoint ) const
{
	Matrix4 cofactor;
	Cofactor( &cofactor );
	cofactor.Transpose( adjoint );
}


void Matrix4::Invert( Matrix4* inverse ) const
{
	// The inverse is the adjoint / determinant of adjoint
	Adjoint( inverse );
	float d = Determinant();
	inverse->ApplyScalar( 1.0f / d );

	#ifdef DEBUG
	Matrix4 result;
	Matrix4 identity;
	MultMatrix4( *this, *inverse, &result );
	GLASSERT( Equal( identity, result, 0.001f ) );
	#endif
}


void Matrix4::InvertRotationMat( Matrix4* inverse ) const
{
	// http://graphics.stanford.edu/courses/cs248-98-fall/Final/q4.html
	*inverse = *this;

	// Transpose the rotation terms.
	Swap( &inverse->m12, &inverse->m21 );
	Swap( &inverse->m13, &inverse->m31 );
	Swap( &inverse->m23, &inverse->m32 );

	const Vector3F* u = (const Vector3F*)(&x[0]);
	const Vector3F* v = (const Vector3F*)(&x[4]);
	const Vector3F* w = (const Vector3F*)(&x[8]);
	const Vector3F* t = (const Vector3F*)(&x[12]);

	inverse->m14 = -DotProduct( *u, *t );
	inverse->m24 = -DotProduct( *v, *t );
	inverse->m34 = -DotProduct( *w, *t );

	#ifdef DEBUG
	Matrix4 result;
	Matrix4 identity;
	MultMatrix4( *this, *inverse, &result );
	GLASSERT( Equal( identity, result, 0.001f ) );
	#endif
}


void Matrix4::Cofactor( Matrix4* cofactor ) const
{
    int i = 1;

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            float det = SubDeterminant(r, c);
            cofactor->x[INDEX(r,c)] = i * det;	// i flip flops between 1 and -1
            i = -i;
        }
        i = -i;
    }
}


float Matrix4::SubDeterminant(int excludeRow, int excludeCol) const 
{
    // Compute non-excluded row and column indices
    int row[3];
    int col[3];

    for (int i = 0; i < 3; ++i) {
        row[i] = i;
        col[i] = i;

        if (i >= excludeRow) {
            ++row[i];
        }
        if (i >= excludeCol) {
            ++col[i];
        }
    }

    // Compute the first row of cofactors 
    float cofactor00 = 
      x[INDEX(row[1],col[1])] * x[INDEX(row[2],col[2])] -
      x[INDEX(row[1],col[2])] * x[INDEX(row[2],col[1])];

    float cofactor10 = 
      x[INDEX(row[1],col[2])] * x[INDEX(row[2],col[0])] -
      x[INDEX(row[1],col[0])] * x[INDEX(row[2],col[2])];

    float cofactor20 = 
      x[INDEX(row[1],col[0])] * x[INDEX(row[2],col[1])] -
      x[INDEX(row[1],col[1])] * x[INDEX(row[2],col[0])];

    // Product of the first row and the cofactors along the first row
    return      x[INDEX(row[0],col[0])] * cofactor00 +
				x[INDEX(row[0],col[1])] * cofactor10 +
				x[INDEX(row[0],col[2])] * cofactor20;
}

/*
bool Matrix4::SetPerspective( float fieldOfView, float _aspectRatio, float _zNear, float _zFar)
{
	double aspectRatio = _aspectRatio;
	double zNear = _zNear;
	double zFar = _zFar;

    double radians = (double)(ToRadian( fieldOfView )) / 2.0;

    double deltaZ = zFar - zNear;
    double sine = sin(radians);
    if ( (deltaZ == 0.0) || (sine == 0.0) || (aspectRatio == 0.0) || zNear <= 0.0 || zFar <= 0.0 ) {
		return false;
    }
    double cotangent = cos(radians) / sine;

    SetIdentity();
    x[ INDEX( 0, 0 ) ] = float( cotangent / aspectRatio );
    x[ INDEX( 1, 1 ) ] = float( cotangent );
    x[ INDEX( 2, 2 ) ] = float( -(zFar + zNear) / deltaZ );
    x[ INDEX( 2, 3 ) ] = float( -1.0 );
    x[ INDEX( 3, 2 ) ] = float( -2.0 * zNear * zFar / deltaZ );
    x[ INDEX( 3, 3 ) ] = 0.0f;

	return true;
}
*/

bool Matrix4::SetLookAt( const Vector3F& eye, const Vector3F& center, const Vector3F& up )
{
	Vector3F forward = center - eye;
	forward.Normalize();

    /* Side = forward x up */
	Vector3F side;
	CrossProduct(forward, up, &side);
    side.Normalize();

    /* Recompute up as: up = side x forward */
	Vector3F up2;
    CrossProduct(side, forward, &up2);

    SetIdentity();
    x[ INDEX(0,0) ] = side.x;
    x[ INDEX(0,1) ] = side.y;
    x[ INDEX(0,2) ] = side.z;

    x[ INDEX(1,0)] = up2.x;
    x[ INDEX(1,1)] = up2.y;
    x[ INDEX(1,2)] = up2.z;

    x[ INDEX(2,0)] = -forward.x;
    x[ INDEX(2,1)] = -forward.y;
    x[ INDEX(2,2)] = -forward.z;

	Matrix4 m, n;
	m.SetTranslation( -eye.x, -eye.y, -eye.z );
	MultMatrix4( *this, m, &n );
	*this = n;
	return true;
}



/*
not yet working
bool grinliz::ScreenToWorld(	const Vector3F& screen,
								const Matrix4& modelView,
								const Matrix4& projection,
								const Rectangle2F& viewport,
								Vector3F* world )
{
    Matrix4 inverse;

	Matrix4 matrix = modelView * projection;
	matrix.Invert( &inverse );
	
	Vector3F in = { (screen.x - viewport.min.x) / viewport.Width(), 
					(screen.y - viewport.min.y) / viewport.Height(),
					screen.z };
					//1.0f };
	
    // normalize -1 to 1
    in.x = in.x*2.0f - 1.0f;
    in.y = in.y*2.0f - 1.0f;
    in.z = in.z*2.0f - 1.0f;

	Vector3F out;
	MultMatrix4( inverse, in, &out );

	//if ( out.w == 0.0f )
	//	return false;

	//world->x = out.x / out.w;
	//world->y = out.y / out.w;
	//world->y = out.z / out.w;
  
	*world = out;
	return true;
}
*/