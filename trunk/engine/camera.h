#ifndef UFOATTACK_CAMERA_INCLUDED
#define UFOATTACK_CAMERA_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmatrix.h"

class Camera
{
public:
	Camera();
	~Camera()	{}

	float Tilt()						{ return tilt; }
	void SetTilt( float value )			{ valid = false; tilt = value; }
	void SetYRotation( float value )	{ valid = false; yRotation = value; }
	void SetViewRotation( int v )		{ valid = false; viewRotation = v; }
	const grinliz::Vector3F& PosWC()	{ return posWC; }
	
	void SetPosWC( const grinliz::Vector3F& value )	{ valid = false; posWC = value; }
	void SetPosWC( float x, float y, float z )		{ valid = false; posWC.Set( x, y, z ); }
	void DeltaPosWC( float x, float y, float z )	{	valid = false;
														posWC.x += x; posWC.y += y; posWC.z += z; }
	void DeltaTilt( float delta )					{ valid = false; tilt += delta; }

	const grinliz::Matrix4& CameraMat()	{	MakeValid();
											return m;	}

private:
	void MakeValid();

	bool valid;
	float tilt;
	int viewRotation;
	float yRotation;
	grinliz::Vector3F posWC;
	grinliz::Matrix4 m;
};


#endif // UFOATTACK_CAMERA_INCLUDED

