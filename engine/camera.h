#ifndef UFOATTACK_CAMERA_INCLUDED
#define UFOATTACK_CAMERA_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glgeometry.h"

/*	By default, the camera is looking down the -z axis with y up.
	View rotation is handled by this class to account for device
	turning.
*/
class Camera
{
public:
	Camera();
	~Camera()	{}

	float GetTilt()									{ return tilt; }
	void SetTilt( float value )						{ tilt = value; }
	void DeltaTilt( float delta )					{ tilt += delta; }

	float GetYRotation()							{ return yRotation; }
	void SetYRotation( float value )				{ yRotation = value; }

	// 0-3, in view space (corresponds to device rotation). Normally one.
	void SetViewRotation( int v )					{ viewRotation = v; }

	const grinliz::Vector3F& PosWC()				{ return posWC; }
	void SetPosWC( const grinliz::Vector3F& value )	{ posWC = value; }
	void SetPosWC( float x, float y, float z )		{ posWC.Set( x, y, z ); }
	void DeltaPosWC( float x, float y, float z )	{ posWC.x += x; posWC.y += y; posWC.z += z; }

	// Draws the camera and submits the glMultMatrix to OpenGL
	void DrawCamera();

	void CalcEyeRay( grinliz::Ray* ray );

private:
	// Position of the camera in the world - no view rotation, no inverse.
	void CalcWorldXForm( grinliz::Matrix4* m );

	float tilt;
	float yRotation;
	int viewRotation;
	grinliz::Vector3F posWC;
};


#endif // UFOATTACK_CAMERA_INCLUDED

