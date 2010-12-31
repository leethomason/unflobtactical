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

struct EngineData
{
	EngineData() : 
		cameraTilt( -50.f ),
		cameraMin( 8.0f ),
		cameraMax( 140.0f ),
		cameraHeight( 15.f )
		{}

	float cameraTilt;		// degrees
	float cameraMin;
	float cameraMax;
	float cameraHeight;
};


class Engine
{
public:
	Engine( Screenport* screenport, const EngineData& engineData, const gamedb::Reader* database );
	~Engine();

	const float AMBIENT;
	const float DIFFUSE;
	const float DIFFUSE_SHADOW;

	Camera camera;

	// Render everything. Return triangles drawn. (Currently excludes UI and particles in tri-count.)
	void Draw();

	SpaceTree* GetSpaceTree()	{ return spaceTree; }

	void MoveCameraHome();
	void CameraIso( bool iso );
	// Move the camera so that it points to x,z. If 'calc' is non-null,
	// the camera will *not* be moved, but the destination for the camera
	// is returned.
	void MoveCameraXZ( float x, float z, grinliz::Vector3F* calc=0 );
	void CameraLookingAt( grinliz::Vector3F* at );

	Model* AllocModel( const ModelResource* );
	void FreeModel( Model* );
	
	void EnableMap( bool enable )		{ enableMap = enable; }
	Map* GetMap()						{ return map; }

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

	Screenport* screenport;
	float	initZoom;
	int		initZoomDistance;
	bool	enableMeta;
	
	const EngineData& engineData;

	Map* map;
	SpaceTree* spaceTree;
	RenderQueue* renderQueue;
	bool enableMap;

	grinliz::Vector3F lightDirection;
	grinliz::Matrix4  shadowMatrix;
};

#endif // UFOATTACK_ENGINE_INCLUDED