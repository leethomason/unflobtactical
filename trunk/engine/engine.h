#ifndef UFOATTACK_ENGINE_INCLUDED
#define UFOATTACK_ENGINE_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glbitarray.h"

#include "map.h"
#include "camera.h"
#include "enginelimits.h"
#include "model.h"

class RenderQueue;

/*
	Standard state:
	Z-Write		enabled
	Z-Test		enabled
	Blend		disabled
*/

struct EngineData
{
	EngineData() : 
		fov( 20.f ),
		farPlane( 240.f ),
		nearPlane( 2.f ),
		cameraTilt( -50.f ),
		cameraMin( 8.0f ),
		cameraMax( 140.0f ),
		cameraHeight( 15.f )
		{}

	float fov;
	float farPlane;
	float nearPlane;
	float cameraTilt;		// degrees
	float cameraMin;
	float cameraMax;
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
	void Draw( int* trianglesDrawn );

	void MoveCameraHome();
	void MoveCameraXZ( float x, float z );

	Model* GetModel( ModelResource* );
	void ReleaseModel( Model* );
	
	Map* GetMap() { return map; }

	grinliz::BitArray<Map::SIZE, Map::SIZE>* GetFogOfWar()	{ return &fogOfWar; }
	void UpdateFogOfWar();

	static bool UnProject(	const grinliz::Vector3F& window,
							const grinliz::Rectangle2I& screen,
							const grinliz::Matrix4& modelViewProjectionInverse,
							grinliz::Vector3F* world );

	void CalcModelViewProjectionInverse( grinliz::Matrix4* modelViewProjectionInverse );
	void RayFromScreenToYPlane( int x, int y, 
								const grinliz::Matrix4& modelViewProjectionInverse, 
								grinliz::Ray* ray,
								grinliz::Vector3F* out );

	enum {
		NEAR,
		FAR,
		LEFT,
		RIGHT,
		BOTTOM,
		TOP
	};
	void CalcFrustumPlanes( grinliz::Plane* planes );

	void Drag( int action, int x, int y );
	void Zoom( int action, int distance );
	void CancelInput()						{ isDragging = false; }

	bool IsDragging() { return isDragging; }
	int InitZoomDistance()  { return initZoomDistance; }
	int LastZoomDistance()	{ return lastZoomDistance; }

	float GetZoom()				{ return zoom; }
	// 0.0 far, 1.0 close
	void SetZoom( float z );

	int Width()		{ return width; }
	int Height()	{ return height; }
	
	void ToggleShadowMode()		{ shadowMode++; if ( shadowMode == SHADOW_COUNT ) shadowMode = 0; }
	int  ShadowMode()			{ return shadowMode; }

private:
	void DrawCamera();
	void RestrictCamera();
	void CalcCameraRotation( grinliz::Matrix4* );
	void EnableLights( bool enable, bool inShadow=false );
	void LightSimple( bool inShadow );
	Model* IntersectModel( const grinliz::Ray& ray, bool onlyDraggable );
	void PushShadowMatrix();

	void PushShadowTextureMatrix();
	void PopShadowTextureMatrix();

	int		width;
	int		height;
 	float	frustumLeft, frustumRight, 
			frustumTop, frustumBottom, 
			frustumNear, frustumFar;
	grinliz::Ray cameraRay;		// origin isn't normally valid
	float	zoom, defaultZoom, initZoom;
	int		initZoomDistance, lastZoomDistance;
	int		depthFunc;
	
	grinliz::Vector3F lightDirection;
	
	grinliz::Vector3F dragStart;
	grinliz::Vector3F dragStartCameraWC;
	grinliz::Matrix4  dragMVPI;
	Model* draggingModel;
	Vector3X draggingModelOrigin;

	enum {
		SHADOW_ONE_PASS,
		SHADOW_Z,
		SHADOW_COUNT
	};
	int shadowMode;
	bool isDragging;
	const EngineData& engineData;

	Map* map;
	SpaceTree* spaceTree;
	RenderQueue* renderQueue;

	grinliz::BitArray<Map::SIZE, Map::SIZE> fogOfWar;
};

#endif // UFOATTACK_ENGINE_INCLUDED