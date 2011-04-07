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
#include "screenport.h"

class RenderQueue;



/*
	Standard state:
	Z-Write		enabled
	Z-Test		enabled
	Blend		disabled
*/

	/*
struct EngineData
{
	EngineData() : 
		cameraTilt( -50.f ),
		cameraMin( 8.0f ),
		cameraMax( 140.0f ),
		cameraHeight( 7.f )
		{}

	float cameraTilt;		// degrees
	float cameraMin;
	float cameraMax;
	float cameraHeight;
};
*/

class Engine
{
public:
	Engine( Screenport* screenport, const gamedb::Reader* database );
	~Engine();

	const float AMBIENT;
	const float DIFFUSE;
	const float DIFFUSE_SHADOW;

	Camera camera;

	// Render everything. Return triangles drawn. (Currently excludes UI and particles in tri-count.)
	void Draw();

	SpaceTree* GetSpaceTree()	{ return spaceTree; }

	void MoveCameraHome();
	void CameraIso(  bool normal, bool sizeToWidth, float width, float height );
	// Move the camera so that it points to x,z. If 'calc' is non-null,
	// the camera will *not* be moved, but the destination for the camera
	// is returned.
	void MoveCameraXZ( float x, float z, grinliz::Vector3F* calc=0 );

	void CameraLookingAt( grinliz::Vector3F* at );
	void CameraLookAt( float x, float z, float heightOfCamera, float yRotation=-45.0f, float tilt=-50.0f );

	// Direction from world TO sun. (y is positive). If null, sets the default.
	void SetLightDirection( const grinliz::Vector3F* lightDir );

	Model* AllocModel( const ModelResource* );
	void FreeModel( Model* );

	void SetMap( Map* m )				{ map = m; iMap = m;}
	void SetIMap( IMap* m )				{ map = 0; iMap = m; }
	Map* GetMap()						{ return map; }

	const RenderQueue* GetRenderQueue()	{ return renderQueue; }
	//void ResetRenderCache();

	// Only matters for MapMaker. Game never renders the metadata.
	void EnableMetadata( bool enable )	{ enableMeta = enable; }
	bool IsMetadataEnabled()			{ return enableMeta; }

	bool RayFromViewToYPlane( const grinliz::Vector2F& view,
							  const grinliz::Matrix4& modelViewProjectionInverse, 
							  grinliz::Ray* ray,
							  grinliz::Vector3F* intersection );

	Model* IntersectModel(	const grinliz::Ray& ray, 
							HitTestMethod testMethod,
							int required, int exclude, const Model* ignore[],
							grinliz::Vector3F* intersection );

	enum {
		NEAR,
		FAR,
		LEFT,
		RIGHT,
		BOTTOM,
		TOP
	};
	void CalcFrustumPlanes( grinliz::Plane* planes );

	float GetZoom();
	void SetZoom( float zoom );

	const Screenport& GetScreenport() { return *screenport; }

	// FIXME - automatic??
	void RestrictCamera();

	static bool mapMakerMode;

private:
	enum ShadowState {
		IN_SHADOW,
		OPEN_LIGHT
	};

	enum DayNight {
		DAY_TIME,
		NIGHT_TIME
	};

	//void DrawCamera();
	void CalcCameraRotation( grinliz::Matrix4* );
	void CalcLights( DayNight dayNight, Color4F* ambient, grinliz::Vector4F* dir, Color4F* diffuse );
	void LightGroundPlane( DayNight dayNight, ShadowState shadows, float shadowAmount, Color4F* outColor );

	void PushShadowSwizzleMatrix( GPUShader* );
	void PushLightSwizzleMatrix( GPUShader* );

	Screenport* screenport;
	float	initZoom;
	int		initZoomDistance;
	bool	enableMeta;
	
	Map*	map;	// If map is set, iMap and map are the same object
	IMap*	iMap;	// If only the iMap is set, map may be null

	SpaceTree* spaceTree;
	RenderQueue* renderQueue;

	grinliz::Vector3F lightDirection;
	grinliz::Matrix4  shadowMatrix;
};

#endif // UFOATTACK_ENGINE_INCLUDED