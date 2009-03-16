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

	const float AMBIENT;
	const float DIFFUSE;
	const float DIFFUSE_SHADOW;
	const float NIGHT_RED;
	const float NIGHT_GREEN;
	const float NIGHT_BLUE;

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

	void SetDayNight( bool dayTime, Surface* lightMap );
	bool GetDayTime()							{ return (dayNight == DAY_TIME); }

private:
	enum ShadowState {
		IN_SHADOW,
		OPEN_LIGHT
	};

	enum DayNight {
		DAY_TIME,
		NIGHT_TIME
	};

	void DrawCamera();
	void RestrictCamera();
	void CalcCameraRotation( grinliz::Matrix4* );
	void EnableLights( bool enable, DayNight dayNight );
	void LightSimple( DayNight dayNight, ShadowState shadows );

	Model* IntersectModel( const grinliz::Ray& ray, bool onlyDraggable );
	void PushShadowMatrix();

	//void PushShadowTextureMatrix();
	//void PopShadowTextureMatrix();

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

	int shadowMode;
	bool isDragging;
	DayNight dayNight;
	const EngineData& engineData;

	Map* map;
	SpaceTree* spaceTree;
	RenderQueue* renderQueue;

	grinliz::BitArray<Map::SIZE, Map::SIZE> fogOfWar;
};

#endif // UFOATTACK_ENGINE_INCLUDED