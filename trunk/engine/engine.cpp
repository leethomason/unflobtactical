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

#include "../grinliz/glutil.h"
#include "../grinliz/glmath.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glperformance.h"

#include "gpustatemanager.h"
#include "engine.h"
#include "loosequadtree.h"
#include "renderQueue.h"
#include "surface.h"
#include "texture.h"
#include "particle.h"


using namespace grinliz;

#define ENGINE_RENDER_MODELS
#define ENGINE_RENDER_SHADOWS
#define ENGINE_RENDER_MAP

/* Optmization #2: Android Nexus One
	Baseline: 27

	No:
		ENGINE_RENDER_MODELS	36
		ENGINE_RENDER_SHADOWS	32
		ENGINE_RENDER_MAP		32

		Interesting. Why would models be more costly than shadows? State changes?

	Turning on VBOs
	FPS: 31				Not a bad jump.
		
*/

/*
	Optimization notes:
	This all starts with tri-counts way high and frame rates way low.
	
	Baseline: Standard test: 11.3 fps, 25K/frame, 290K/sec
 
	Ouch, that's a lot of triangles. Roughly 1600 in the map, and another 10,000 in models. Thats all rendered twice,
	once for the shadow pass and once normally, and it gets to 25K.
 
	1. Fix the renderQueue size. Incremental improvement.
	2. Was using FLOAT instead of FIXED. No difference. Considering switching everything back to float.
	3. Using BLEND instead of ALPHA_TEST does help a little. Up to about 12fps. Since character models get rendered
	   before the alpha pass, this is a good one to keep.
	4. Switched to FLOAT. Makes no difference - handled in VBOs anyway?
 
	Various tests:
	Baseline				11
	1/2 Triangles/model:	20
	1/2 Models:				20
	No background draw:		14
	No shadows:				18
	Background one quad:	13
 
	5. Vertex buffering. In theory sounds smart. The docs say do it. In practice, re-transforming every vertex an extra time.
	   A silly and impractical optimization.
	
	Reducing art to 13K tri/second - 20fps!!
	Its all about the # of triangles, it seems.
 
	From 20 to 30!
	Less triangles in the farmland doesn't help...nor does taking out the character models. Magic framerate #? Removing all
	model drawing gets to 40, so the timer isn't the issue.
 
	New baseline:			20.0 fps
 
	Ideas:	1. Better bounding tests. Bounding boxes are still quite loose. [done]
			2. Replace characters with billboards.		[pass]
			3. Triangle reduction of the background.	[done - switched to texture]
			4. Background as a single texture.			[done]
			5. Is it possible to do planar shadows in one pass? Map from vertex to texture coordinates? [HECK YEAH - complex, but done]
 
			ALL THAT! Gets it back to 30.0fps, 7.6K tris/frame. Yay!

	Ideas:	1. Remove all down facing triangles.
*/

/*	Notes

///	Shadows.
 
	The engine supports planar shadows. The obvious way to implement this is to:
		1. render the background in light
		2. render the shadows to stencil / depth / destination-alpha
		3. render the background in shadow, testing against stencil / depth /destination-alpha
 
	The iphone doesn't have stencil. Testing dest-alpha (blending) vs. depth indicated that depth is *much* 
	faster. So depth is supported as SHADOW_Z.
 
	Once I switched the background plane to a single texture, it opened up a new possibility. Transform the 
	shadow coordinates to ground texture coordinates. Tricky as hell on the fixed pipeline.
 
	1. Feed vertex coordinates to texture coordinates.
	2. Transform the texture/vertex coordinates to the ground plane (shadow)
	3. Transform from ground plane coordinates to texture (uv) coordinates.

	Simple in theory, simple on a shader, tricky in fixed pipeline. But does work.
 
	When modulation is added in (for shadows) 2 texture units have to be set. That makes the z-buffer path 
	faster again. *sigh* All that work and I'm back to the first approach. The single texture approach 
	would be much simpler with shaders. Oh well. Still at 30fps.

*/


Engine::Engine( Screenport* port, const gamedb::Reader* database ) 
	:	AMBIENT( 0.3f ),
		DIFFUSE( 0.8f ),
		DIFFUSE_SHADOW( 0.2f ),
		screenport( port ),
		initZoomDistance( 0 ),
		map( 0 ),
		iMap( 0 )
{
	spaceTree = new SpaceTree( -0.1f, 3.0f );
	renderQueue = new RenderQueue();

	lightDirection.Set( EL_LIGHT_X, EL_LIGHT_Y, EL_LIGHT_Z );
	lightDirection.Normalize();
	enableMeta = mapMakerMode;
}


Engine::~Engine()
{
	delete renderQueue;
	delete spaceTree;
}


bool Engine::mapMakerMode = false;


void Engine::CameraIso( bool normal, bool sizeToWidth, float width, float height )
{
	if ( normal ) {
		camera.SetYRotation( -45.f );
		camera.SetTilt( -50.0f );
		MoveCameraHome();
		//camera.SetPosWC( 0, 10, 0 0 );
	}
	else {
		float h = 0;
		float theta = ToRadian( EL_FOV/2.0f );
		float ratio = screenport->UIAspectRatio();

		if ( sizeToWidth ) {
			h = (width) / tanf(theta);		
		}
		else {
			h = (height/2) / (tanf(theta)*ratio);
		}
		camera.SetYRotation( 0 );
		camera.SetTilt( -90.0f );
		camera.SetPosWC( width/2.0f, h, height/2.0f );
	}
}


void Engine::MoveCameraHome()
{
	camera.SetPosWC( EL_MAP_SIZE/2, 25.0f, EL_MAP_SIZE/2 );
	camera.SetYRotation( -45.f );
	camera.SetTilt( -50.0f );
}


void Engine::CameraLookAt( float x, float z, float heightOfCamera, float yRotation, float tilt )
{
	camera.SetPosWC( x, heightOfCamera, z );
	camera.SetYRotation( yRotation );
	camera.SetTilt( tilt );

	Vector3F at;
	CameraLookingAt( &at );
	
	camera.DeltaPosWC( x-at.x, 0, z-at.z );
}


void Engine::CameraLookingAt( grinliz::Vector3F* at )
{
	const Vector3F* eyeDir = camera.EyeDir3();
	float h = camera.PosWC().y;

	*at = camera.PosWC() - eyeDir[0]*(h/eyeDir[0].y);	
}


void Engine::MoveCameraXZ( float x, float z, Vector3F* calc )
{
	// Move the camera, but don't change tilt or rotation.
	// The distance is based on triangle ratios.
	//
	const Vector3F* eyeDir = camera.EyeDir3();

	Vector3F start = { x, 0.0f, z };
	
	float h = camera.PosWC().y;
	Vector3F pos = start + eyeDir[0]*(h/eyeDir[0].y);

	if ( calc ) {
		*calc = pos;
	}
	else {
		camera.SetPosWC( pos.x, pos.y, pos.z );
	}
}


void Engine::SetLightDirection( const grinliz::Vector3F* dir ) 
{
	lightDirection.Set( EL_LIGHT_X, EL_LIGHT_Y, EL_LIGHT_Z );
	if ( dir ) {
		lightDirection = *dir;
	}
	lightDirection.Normalize();
}

Model* Engine::AllocModel( const ModelResource* resource )
{
	GLASSERT( resource );
	return spaceTree->AllocModel( resource );
}


void Engine::FreeModel( Model* model )
{
	spaceTree->FreeModel( model );
}


void Engine::PushShadowSwizzleMatrix( GPUShader* shader )
{
	// A shadow matrix for a flat y=0 plane! heck yeah!
	shadowMatrix.m12 = -lightDirection.x/lightDirection.y;
	shadowMatrix.m22 = 0.0f;
	shadowMatrix.m32 = -lightDirection.z/lightDirection.y;

	// x'    1/64  0    0    0
	// y'      0   0  -1/64  -1
	//     =   0   0    0    0
	//		
	Matrix4 swizzle;
	swizzle.m11 = 1.f/((float)EL_MAP_SIZE);
	swizzle.m22 = 0;	swizzle.m23 = -1.f/((float)EL_MAP_SIZE);	swizzle.m24 = 1.0f;
	swizzle.m33 = 0.0f;

	shader->PushTextureMatrix( 3 );
	shader->MultTextureMatrix( 3, swizzle );
	shader->MultTextureMatrix( 3, shadowMatrix );

	shader->PushMatrix( GPUShader::MODELVIEW_MATRIX );
	shader->MultMatrix( GPUShader::MODELVIEW_MATRIX, shadowMatrix );
}


void Engine::PushLightSwizzleMatrix( GPUShader* shader )
{
	GLASSERT( iMap );
	float w, h, dx, dz;
	iMap->LightFogMapParam( &w, &h, &dx, &dz );

	Matrix4 swizzle;
	swizzle.m11 = 1.f/w;
	swizzle.m22 = 0;	swizzle.m23 = -1.f/h;	swizzle.m24 = 1.0f;
	swizzle.m33 = 0.0f;

	swizzle.m14 = dx;
	swizzle.m34 = dz;

	shader->PushTextureMatrix( 2 );
	shader->MultTextureMatrix( 2, swizzle );
}


void Engine::Draw()
{
	GRINLIZ_PERFTRACK;

	// -------- Camera & Frustum -------- //
	screenport->SetView( camera.ViewMatrix() );	// Draw the camera

#ifdef DEBUG
	{
		Vector3F at;
		CameraLookingAt( &at );
		//GLOUTPUT(( "View set. Camera at (%.1f,%.1f,%.1f) looking at (%.1f,%.1f,%.1f)\n",
		//	camera.PosWC().x, camera.PosWC().y, camera.PosWC().z,
		//	at.x, at.y, at.z ));
		if ( map ) {
			Rectangle2I b = map->Bounds();
			b.Outset( 2 );
			if ( !b.Contains( (int)at.x, (int)at.z ) ) {
				GLASSERT( 0 );	// looking at nothing.
			}
		}
	}
#endif
				

	// Compute the frustum planes and query the tree.
	Plane planes[6];
	CalcFrustumPlanes( planes );

	Model* modelRoot = spaceTree->Query( planes, 6, 0, Model::MODEL_INVISIBLE, false );
	
	Color4F ambient, diffuse;
	Vector4F dir;
	CalcLights( ( !map || map->DayTime() ) ? DAY_TIME : NIGHT_TIME, &ambient, &dir, &diffuse );

	LightShader lightShader( ambient, dir, diffuse, false );
	LightShader blendLightShader( ambient, dir, diffuse, true );	// Some tiles use alpha - for instance the "splat" image

	LightShader mapItemShader( ambient, dir, diffuse, false );
	LightShader mapBlendItemShader( ambient, dir, diffuse, true );

	Rectangle2I mapBounds( 0, 0, EL_MAP_SIZE-1, EL_MAP_SIZE-1 );
	if ( map ) {
		mapBounds = map->Bounds();
	}

	// ------------ Process the models into the render queue -----------
	{
		GLASSERT( renderQueue->Empty() );
		const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>* fogOfWar = (map) ? &map->GetFogOfWar() : 0;

		for( Model* model=modelRoot; model; model=model->next ) {
			if ( model->IsFlagSet( Model::MODEL_METADATA ) && !enableMeta )
				continue;

			if ( model->IsFlagSet(  Model::MODEL_OWNED_BY_MAP ) ) {
				model->Queue(	renderQueue, 
								&mapItemShader,
								&mapBlendItemShader,
								0 );
			}
			else {
				Vector3F pos = model->AABB().Center();
				int x = LRintf( pos.x - 0.5f );
				int y = LRintf( pos.z - 0.5f );

#ifdef EL_SHOW_ALL_UNITS
				{
#else
				if ( mapBounds.Contains( x, y ) && (!fogOfWar || fogOfWar->IsSet( x, y ) ) ) {
#endif
					model->Queue( renderQueue, &lightShader, &blendLightShader, 0 );
				}
			}
		}
	}


	// ----------- Render Passess ---------- //
	Color4F color;

	if ( map ) {
		// If the map is enabled, we draw the basic map plane lighted. Then draw the model shadows.
		// The shadows are the tricky part: one matrix is used to transform the vertices to the ground
		// plane, and the other matrix is used to transform the vertices to texture coordinates.
		// Shaders make this much, much, much easier.

		// -------- Ground plane lighted -------- //
#ifdef ENGINE_RENDER_MAP
		map->GenerateSeenUnseen();
		map->DrawSeen();
#endif

		// -------- Shadow casters/ground plane ---------- //
		// Set up the planar projection matrix, with a little z offset
		// to help with z resolution fighting.
		const float SHADOW_START_HEIGHT = 80.0f;
		const float SHADOW_END_HEIGHT   = SHADOW_START_HEIGHT + 5.0f;
		float shadowAmount = 1.0f;
		if ( camera.PosWC().y > SHADOW_START_HEIGHT ) {
			shadowAmount = 1.0f - ( camera.PosWC().y - SHADOW_START_HEIGHT ) / ( SHADOW_END_HEIGHT - SHADOW_START_HEIGHT );
		}
		if ( shadowAmount > 0.0f ) {
#ifdef ENGINE_RENDER_SHADOWS
			CompositingShader shadowShader;
			shadowShader.SetTexture0( map->BackgroundTexture() );
			shadowShader.SetTexture1( map->LightMapTexture() );

			// The shadow matrix pushes in a depth. Its the depth<0 that allows the GL_LESS
			// test for the shadow write, below.
			PushShadowSwizzleMatrix( &shadowShader );

			// Just computes how dark the shadow is.
			LightGroundPlane( map->DayTime() ? DAY_TIME : NIGHT_TIME, IN_SHADOW, shadowAmount, &color );
			shadowShader.SetColor( color );

			renderQueue->Submit(	&shadowShader,
									RenderQueue::MODE_PLANAR_SHADOW,
									0,
									Model::MODEL_NO_SHADOW );

			shadowShader.PopMatrix( GPUShader::MODELVIEW_MATRIX );
			shadowShader.PopTextureMatrix( 3 );
		}
#endif
		{
			LightGroundPlane( map->DayTime() ? DAY_TIME : NIGHT_TIME, OPEN_LIGHT, 0, &color );

			float ave = 0.5f*((color.r + color.g + color.b)*0.333f);
			Color4F c = { ave, ave, ave, 1.0f };
#ifdef ENGINE_RENDER_MAP
			map->DrawPastSeen( c );
#endif
		}

#ifdef ENGINE_RENDER_MAP
		map->DrawOverlay( Map::LAYER_UNDER_LOW );
		map->DrawUnseen();
		map->DrawOverlay( Map::LAYER_UNDER_HIGH );
#endif
	}

	// -------- Models ---------- //
#ifdef ENGINE_RENDER_MODELS
	{
		if ( iMap ) {
			mapItemShader.SetTexture1( iMap->LightFogMapTexture() );
			mapBlendItemShader.SetTexture1( iMap->LightFogMapTexture() );
			
			PushLightSwizzleMatrix( &mapItemShader );

			renderQueue->Submit( 0, 0, Model::MODEL_OWNED_BY_MAP, 0 );
			lightShader.PopTextureMatrix( 2 );
		}
		// Render everything NOT in the map.
		renderQueue->Submit( 0, 0, 0, Model::MODEL_OWNED_BY_MAP );
	}
#endif

	if ( map )
		map->DrawOverlay( Map::LAYER_OVER );
	renderQueue->Clear();
}


void Engine::CalcLights( DayNight dayNight, Color4F* ambient, Vector4F* dir, Color4F* diffuse )
{
	ambient->Set( AMBIENT, AMBIENT, AMBIENT, 1.0f );
	diffuse->Set( DIFFUSE, DIFFUSE, DIFFUSE, 1.0f );
	if ( dayNight == NIGHT_TIME ) {
		diffuse->r *= EL_NIGHT_RED;
		diffuse->g *= EL_NIGHT_GREEN;
		diffuse->b *= EL_NIGHT_BLUE;
	}
	dir->Set( lightDirection.x, lightDirection.y, lightDirection.z, 0 );	// '0' in last term is parallel
}


void Engine::LightGroundPlane( DayNight dayNight, ShadowState shadows, float shadowAmount, Color4F* outColor )
{
	outColor->Set( 1, 1, 1, 1 );

	if ( shadows == IN_SHADOW ) {
		float delta = 1.0f - DIFFUSE_SHADOW*shadowAmount;
		outColor->r *= delta;
		outColor->g *= delta;
		outColor->b *= delta;
	}
}


bool Engine::RayFromViewToYPlane( const Vector2F& v, const Matrix4& mvpi, Ray* ray, Vector3F* out )
{	
	screenport->ViewToWorld( v, &mvpi, ray );

	Plane plane;
	plane.n.Set( 0.0f, 1.0f, 0.0f );
	plane.d = 0.0;
	
	int result = REJECT;
	if ( out ) {
		float t;
		result = IntersectLinePlane( ray->origin, ray->origin+ray->direction, plane, out, &t );
	}
	return ( result == INTERSECT );
}


void Engine::CalcFrustumPlanes( grinliz::Plane* planes )
{
	// --------- Compute the view frustum ----------- //
	// A strange and ill-documented algorithm from Real Time Rendering, 2nd ed, pg.613
	Matrix4 m;
	screenport->ViewProjection3D( &m );

	// m is the matrix from multiplying projection and model. The
	// subscript is the row.

	// Left
	// -(m3 + m0) * (x,y,z,0) = 0
	planes[0].n.x = ( m.x[3+0]  + m.x[0+0] );
	planes[0].n.y = ( m.x[3+4]  + m.x[0+4] );
	planes[0].n.z = ( m.x[3+8]  + m.x[0+8] );
	planes[0].d   = ( m.x[3+12] + m.x[0+12] );
	planes[0].Normalize();

	// right
	planes[1].n.x = ( m.x[3+0]  - m.x[0+0] );
	planes[1].n.y = ( m.x[3+4]  - m.x[0+4] );
	planes[1].n.z = ( m.x[3+8]  - m.x[0+8] );
	planes[1].d   = ( m.x[3+12] - m.x[0+12] );
	planes[1].Normalize();

	// bottom
	planes[2].n.x = ( m.x[3+0]  + m.x[1+0] );
	planes[2].n.y = ( m.x[3+4]  + m.x[1+4] );
	planes[2].n.z = ( m.x[3+8]  + m.x[1+8] );
	planes[2].d   = ( m.x[3+12] + m.x[1+12] );
	planes[2].Normalize();

	// top
	planes[3].n.x = ( m.x[3+0]  - m.x[1+0] );
	planes[3].n.y = ( m.x[3+4]  - m.x[1+4] );
	planes[3].n.z = ( m.x[3+8]  - m.x[1+8] );
	planes[3].d   = ( m.x[3+12] - m.x[1+12] );
	planes[3].Normalize();

	// near
	planes[4].n.x = ( m.x[3+0]  + m.x[2+0] );
	planes[4].n.y = ( m.x[3+4]  + m.x[2+4] );
	planes[4].n.z = ( m.x[3+8]  + m.x[2+8] );
	planes[4].d   = ( m.x[3+12] + m.x[2+12] );
	planes[4].Normalize();

	// far
	planes[5].n.x = ( m.x[3+0]  - m.x[2+0] );
	planes[5].n.y = ( m.x[3+4]  - m.x[2+4] );
	planes[5].n.z = ( m.x[3+8]  - m.x[2+8] );
	planes[5].d   = ( m.x[3+12] - m.x[2+12] );
	planes[5].Normalize();

	GLASSERT( DotProduct( planes[LEFT].n, planes[RIGHT].n ) < 0.0f );
	GLASSERT( DotProduct( planes[TOP].n, planes[BOTTOM].n ) < 0.0f );
	GLASSERT( DotProduct( planes[NEAR].n, planes[FAR].n ) < 0.0f );
}


Model* Engine::IntersectModel( const grinliz::Ray& ray, HitTestMethod method, int required, int exclude, const Model* ignore[], Vector3F* intersection )
{
	GRINLIZ_PERFTRACK

	Model* model = spaceTree->QueryRay(	ray.origin, ray.direction, 
										required, exclude, ignore,
										method,
										intersection );
	return model;
}


void Engine::RestrictCamera()
{
	const Vector3F* eyeDir = camera.EyeDir3();

	Vector3F intersect;
	IntersectRayPlane( camera.PosWC(), eyeDir[0], XZ_PLANE, 0.0f, &intersect );
//	GLOUTPUT(( "Intersect %.1f, %.1f, %.1f\n", intersect.x, intersect.y, intersect.z ));

	const float SIZEX = (float)map->Width();
	const float SIZEZ = (float)map->Height();

	if ( intersect.x < 0.0f ) {
		camera.DeltaPosWC( -intersect.x, 0.0f, 0.0f );
	}
	if ( intersect.x > SIZEX ) {
		camera.DeltaPosWC( -(intersect.x-SIZEX), 0.0f, 0.0f );
	}
	if ( intersect.z < 0.0f ) {
		camera.DeltaPosWC( 0.0f, 0.0f, -intersect.z );
	}
	if ( intersect.z > SIZEZ ) {
		camera.DeltaPosWC( 0.0f, 0.0f, -(intersect.z-SIZEZ) );
	}
}



void Engine::SetZoom( float z )
{
//	float startY = camera.PosWC().y;

	z = Clamp( z, GAME_ZOOM_MIN, GAME_ZOOM_MAX );
	float d = Interpolate(	GAME_ZOOM_MIN, EL_CAMERA_MIN,
							GAME_ZOOM_MAX, EL_CAMERA_MAX,
							z );

	const Vector3F* eyeDir = camera.EyeDir3();
	Vector3F origin;
	int result = IntersectRayPlane( camera.PosWC(), eyeDir[0], 1, 0.0f, &origin );
	if ( result != grinliz::INTERSECT ) {
		MoveCameraHome();
	}
	else {
		// The 'd' is the actual height. Adjust for the angle of the camera.
		float len = ( d ) / eyeDir[0].y;
		Vector3F pos = origin + len*eyeDir[0];
		camera.SetPosWC( pos );
	}
	//GLOUTPUT(( "Engine set zoom. y=%f to y=%f", startY, camera.PosWC().y ));
}


float Engine::GetZoom() 
{
	float z = Interpolate(	EL_CAMERA_MIN, GAME_ZOOM_MIN,
							EL_CAMERA_MAX, GAME_ZOOM_MAX,
							camera.PosWC().y );
	return z;
}


/*
void Engine::ResetRenderCache()
{
	renderQueue->ResetRenderCache();
	Model* model = spaceTree->Query( 0, 0, 0, 0 );
	for( ; model; model = model->next ) {
		for( int i=0; i<EL_MAX_MODEL_GROUPS; ++i )
			model->cacheStart[i] = Model::CACHE_UNINITIALIZED;
	}
}
*/
