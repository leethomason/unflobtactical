#ifndef UFOATTACK_ENGINE_INCLUDED
#define UFOATTACK_ENGINE_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glmatrix.h"

#include "map.h"
#include "camera.h"

struct EngineData
{
	EngineData() : 
		fov( 20.f ),
		farPlane( 100.f ),
		nearPlane( 2.f ),
		cameraTilt( 50.f ),
		cameraHeight( 15.f )
		{}

	float fov;
	float farPlane;
	float nearPlane;
	float cameraTilt;		// degrees
	float cameraHeight;

};


class Engine
{
public:
	Engine( int _width, int _height, const EngineData& engineData );
	~Engine();

	Camera camera;
	float fov;

	void SetPerspective();
	void Draw();

	static bool UnProject(	const grinliz::Vector3F& window,
							const grinliz::Rectangle2I& screen,
							const grinliz::Matrix4& modelViewProjectionInverse,
							grinliz::Vector3F* world );

	void CalcModelViewProjectionInverse( grinliz::Matrix4* modelViewProjectionInverse );
	void RayFromScreenToYPlane( int x, int y, const grinliz::Matrix4& modelViewProjectionInverse, grinliz::Vector3F* out );

	void DragStart( int x, int y );
	void DragMove( int x, int y );
	void DragEnd( int x, int y );
	bool IsDragging() { return isDragging; }

private:
	void DrawCamera();
	void CalcCameraRotation( grinliz::Matrix4* );

	int		width;
	int		height;
	float	frustumLeft, frustumRight, 
			frustumTop, frustumBottom, 
			frustumNear, frustumFar;
	
	grinliz::Vector3F dragStart;
	//grinliz::Vector2I dragStartScreen;
	grinliz::Vector3F dragStartCameraWC;
	grinliz::Matrix4  dragMVPI;

	bool isDragging;

	const EngineData& engineData;
	Map map;
};

#endif // UFOATTACK_ENGINE_INCLUDED