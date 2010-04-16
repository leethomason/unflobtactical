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

#include "../game/cgame.h"

#include "platformgl.h"
#include "engine.h"
#include "loosequadtree.h"
#include "renderQueue.h"
#include "surface.h"
#include "particle.h"

using namespace grinliz;

int trianglesRendered = 0;	// FIXME: should go away once all draw calls are moved to the enigine
int drawCalls = 0;			// ditto

//#define SHOW_FOW

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

/// VBOs.
	A complete waste. Turning them off improved performance on the iPod. From 20 to 27 fps.

*/


Engine::Engine( Screenport* port, const EngineData& _engineData ) 
	:	AMBIENT( 0.3f ),
		DIFFUSE( 0.7f ),
		DIFFUSE_SHADOW( 0.3f ),
		screenport( port ),
		engineData( _engineData ),
		initZoomDistance( 0 ),
		enableMap( true )
{
	TextureManager::Create();
	ImageManager::Create();
	ModelResourceManager::Create();
	ParticleSystem::Create();

	spaceTree = new SpaceTree( -0.1f, 3.0f );
	map = new Map( spaceTree );
	renderQueue = new RenderQueue();

	camera.SetViewRotation( screenport->Rotation() );
	camera.SetPosWC( -5.0f, engineData.cameraHeight, (float)Map::SIZE + 5.0f );
	camera.SetYRotation( -45.f );
	camera.SetTilt( engineData.cameraTilt );

	lightDirection.Set( 0.7f, 3.0f, 1.4f );
	lightDirection.Normalize();
	depthFunc = 0;
	enableMeta = true;
}


Engine::~Engine()
{
	delete renderQueue;
	delete map;
	delete spaceTree;

	ParticleSystem::Destroy();
	ModelResourceManager::Destroy();
	ImageManager::Destroy();
	TextureManager::Destroy();
}


void Engine::MoveCameraHome()
{
	camera.SetPosWC( -5.0f, engineData.cameraHeight, (float)map->Height() + 5.0f );
	camera.SetYRotation( -45.f );
	camera.SetTilt( engineData.cameraTilt );
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
		RestrictCamera();
	}
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


void Engine::PushShadowMatrix()
{
	Matrix4 m;
	m.m12 = -lightDirection.x/lightDirection.y;
	m.m22 = 0.0f;

	// The shadow needs a depth to use the Z-buffer. More depth is good,
	// more resolution, but intoduces an error in eye space. Try to correct
	// for the error in eye space using the camera ray. (The correct answer
	// is per-vertex, but we won't pay for that). Combine that with a smallish
	// depth value to try to minimize shadow errors.
	const Vector3F* eyeDir = camera.EyeDir3();

	const float DEPTH = 0.2f;
	m.m14 = -eyeDir[0].x/eyeDir[0].y * DEPTH;	// x hide the shift 
	m.m24 = -DEPTH;								// y term down
	m.m34 = -eyeDir[0].z/eyeDir[0].y * DEPTH;	// z hide the shift
	
	m.m32 = -lightDirection.z/lightDirection.y;

	glPushMatrix();
	glMultMatrixf( m.x );
}


void Engine::Draw()
{
	// -------- Camera & Frustum -------- //
	screenport->SetView( camera.ViewMatrix() );	// Draw the camera

	float bbRotation = camera.GetBillboardYRotation();
	float shadowRotation = ToDegree( atan2f( lightDirection.x, lightDirection.z ) );
//	glDisable( GL_LIGHTING );

	// Compute the frustum planes
	Plane planes[6];
	CalcFrustumPlanes( planes );

	Model* modelRoot = spaceTree->Query( planes, 6, 0, 0, false );

#	ifdef USING_GL
#		ifdef DEBUG
			GLASSERT( glIsEnabled( GL_DEPTH_TEST ) );
			int depthMask = 0;
			glGetIntegerv( GL_DEPTH_WRITEMASK, &depthMask );
			GLASSERT( depthMask );
#		endif
#	endif

	if ( depthFunc == 0 ) {
		// Not clear what the default is (from the driver): LEQUAL or LESS. So query and cache.
		glGetIntegerv( GL_DEPTH_FUNC, &depthFunc );	
	}

	if ( enableMap ) {
		// -------- Ground plane lighted -------- //
		Color4F color;
		LightGroundPlane( OPEN_LIGHT, 1.0f, &color );

		// The depth mask and the depth test should be completely
		// independent...but it's not. Very subtle point of how
		// OpenGL works.
		glColor4f( color.x, color.y, color.z, 1.0f );
		map->Draw();

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
			// The shadow matrix pushes in a depth. Its the depth<0 that allows the GL_LESS
			// test for the shador write, below.
			PushShadowMatrix();

			int textureState = 0;
			glDisable( GL_TEXTURE_2D );
			glDepthFunc( GL_ALWAYS );

			textureState = Model::NO_TEXTURE;

			renderQueue->SetColor( 0, 0, 0 );
			GLASSERT( renderQueue->Empty() );

			for( Model* model=modelRoot; model; model=model->next ) {
				// Take advantage of this walk to adjust the billboard rotations. Note that the rotation
				// will never change it's position in the space tree, which is why we can set it here.
				if ( model->IsBillboard() ) {
					if ( model->GetRotation() != bbRotation )
						model->SetRotation( bbRotation );
					if ( model->IsShadowRotated() )
						model->SetRotation( shadowRotation );
				}
				if ( model->IsFlagSet( Model::MODEL_NO_SHADOW ) )
					continue;
				if ( model->IsFlagSet( Model::MODEL_METADATA ) && !enableMeta )
					continue;

				// Draw model shadows.
				model->Queue( renderQueue, textureState );
			}
			renderQueue->Flush();

			CHECK_GL_ERROR;
			glEnable( GL_TEXTURE_2D );
			glPopMatrix();
		}
		// -------- Ground plane shadow ---------- //
		// Redraw the map, checking z, in shadow.
		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		CHECK_GL_ERROR;

		LightGroundPlane( IN_SHADOW, shadowAmount, &color );
		glDepthFunc( GL_LESS );
		glColor4f( color.x, color.y, color.z, 1.0f );
		map->Draw();

		// Draw the "where can I walk" overlay.
		glDepthFunc( GL_ALWAYS );

		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		map->DrawOverlay();
		map->DrawFOW();

		glDepthFunc( depthFunc );

	}

	// -------- Models in light ---------- //
	CHECK_GL_ERROR;
	glEnable( GL_TEXTURE_2D );
	glDepthFunc( depthFunc );
	CHECK_GL_ERROR;

	renderQueue->SetColor( 1, 1, 1 );
	EnableLights( true, map->DayTime() ? DAY_TIME : NIGHT_TIME );
	Model* fogRoot = 0;
	const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>& fogOfWar = map->GetFogOfWar();

	for( Model* model=modelRoot; model; model=model->next ) 
	{
		if ( !enableMap && model->IsFlagSet( Model::MODEL_OWNED_BY_MAP ) )
			continue;
		if ( model->IsFlagSet( Model::MODEL_METADATA ) && !enableMeta )
			continue;

		// Remove the shadow rotation for this pass.
		if ( model->IsBillboard() && model->IsShadowRotated() ) {
			model->SetRotation( bbRotation );
		}

		const Vector3F& pos = model->Pos();
		int x = LRintf( pos.x - 0.5f );
		int y = LRintf( pos.z - 0.5f );

		bool queue = false;		// draw in color
		bool queue0 = false;	// draw in black

		if ( model->IsFlagSet(  Model::MODEL_OWNED_BY_MAP ) ) {
			if ( model->mapBoundsCache.min.x < 0 ) {
				map->MapBoundsOfModel( model, &model->mapBoundsCache );
			}

			Rectangle2I fogRect = model->mapBoundsCache;
			if ( fogRect.min.x > 0 )	fogRect.min.x -= 1;
			if ( fogRect.min.y > 0 )	fogRect.min.y -= 1;
			if ( fogRect.max.x < Map::SIZE-1 ) fogRect.max.x += 1;
			if ( fogRect.max.y < Map::SIZE-1 ) fogRect.max.y += 1;

			// Map is always rendered, possibly in black.
			if ( !fogOfWar.IsRectEmpty( fogRect ) ) {
				queue = true;
			}
			else {
				queue0 = true;
			}
		}
		else if ( model->IsFlagSet( Model::MODEL_ALWAYS_DRAW )) {
			queue = true;
		}
		else if ( fogOfWar.IsSet( x, y ) ) {
			queue = true;
		}
#ifdef SHOW_FOW
		else if ( !fogOfWar.IsSet( x, y ) ) {
			queue0 = true;
		}
#endif

		GLASSERT( !(queue && queue0) );
		if ( queue ) {
			model->Queue( renderQueue, Model::MODEL_TEXTURE );
		}
		else if ( queue0 ) {
			model->next0 = fogRoot;
			fogRoot = model;
		}
	}
	renderQueue->Flush();
	EnableLights( false, map->DayTime() ? DAY_TIME : NIGHT_TIME );
	glBindTexture( GL_TEXTURE_2D, 0 );

#ifdef SHOW_FOW
	renderQueue->SetColor( 0.5f, 0.5f, 0.0f );
#else
	renderQueue->SetColor( 0, 0, 0 );
#endif

	for( Model* model=fogRoot; model; model=model->next0 ) {
		model->Queue( renderQueue, Model::NO_TEXTURE );
	}
	renderQueue->Flush();

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

//	*triCount = renderQueue->GetTriCount();
//	renderQueue->ClearTriCount();

/*
#ifdef DEBUG
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

		glDisable( GL_TEXTURE_2D );
		glDisableClientState( GL_NORMAL_ARRAY );
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );

		spaceTree->Draw();

		glEnableClientState( GL_NORMAL_ARRAY );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glEnable( GL_TEXTURE_2D );
		glDisable( GL_BLEND );
	}
#endif
*/
//	if ( scissorUI.IsValid() ) {
//		screenport.SetViewport( 0 );
//	}
}


void Engine::EnableLights( bool enable, DayNight dayNight )
{
	CHECK_GL_ERROR;
	if ( !enable ) {
		glDisable( GL_LIGHTING );
	}
	else {
		glEnable( GL_LIGHTING );
		CHECK_GL_ERROR;

		const float white[4]	= { 1.0f, 1.0f, 1.0f, 1.0f };
		const float black[4]	= { 0.0f, 0.0f, 0.0f, 1.0f };

		float ambient[4] = { AMBIENT, AMBIENT, AMBIENT, 1.0f };
		float diffuse[4] = { DIFFUSE, DIFFUSE, DIFFUSE, 1.0f };
		if ( dayNight == NIGHT_TIME ) {
			diffuse[0] *= EL_NIGHT_RED;
			diffuse[1] *= EL_NIGHT_GREEN;
			diffuse[2] *= EL_NIGHT_BLUE;
		}

		Vector3F lightDir = lightDirection;

		float lightVector4[4] = { lightDir.x, lightDir.y, lightDir.z, 0.0 };	// parallel

		CHECK_GL_ERROR;
		//glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

		// Light 0. The Sun or Moon.
		glEnable(GL_LIGHT0);
		glLightfv(GL_LIGHT0, GL_POSITION, lightVector4 );
		glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient );
		glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse );
		glLightfv(GL_LIGHT0, GL_SPECULAR, black );
		CHECK_GL_ERROR;

		// The material.
		glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, black );
		glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, black );
		glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  white );
		glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  white );
		CHECK_GL_ERROR;
	}
}


void Engine::LightGroundPlane( ShadowState shadows, float shadowAmount, Color4F* outColor )
{
	// The color (unshadowed) is already committed to the ground plane shadow map.
	// This just turns shadows on and off.
	float light = 1.0f;
	if ( shadows == IN_SHADOW ) {
		light = 1.0f - 0.3f*shadowAmount;
	}
	outColor->Set( light, light, light, 1.0f );
}


bool Engine::RayFromScreenToYPlane( int x, int y, const Matrix4& mvpi, Ray* ray, Vector3F* out )
{	
	Vector2I v;
	screenport->ScreenToView( x, y, &v );

	Vector3F win0 ={ (float)v.x, (float)v.y, 0.0f };
	Vector3F win1 ={ (float)v.x, (float)v.y, 1.0f };

	Vector3F p0, p1;
	
	screenport->ViewToWorld( win0, mvpi, &p0 );
	screenport->ViewToWorld( win1, mvpi, &p1 );

	Plane plane;
	plane.n.Set( 0.0f, 1.0f, 0.0f );
	plane.d = 0.0;
	
	Vector3F dir = p1 - p0;

	if ( ray ) {
		ray->origin = p0;
		ray->direction = dir;
	}
	int result = REJECT;
	if ( out ) {
		float t;
		result = IntersectLinePlane( p0, p1, plane, out, &t );
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
	z = Clamp( z, 0.0f, 1.0f );

	const Vector3F* eyeDir = camera.EyeDir3();
	Vector3F origin;
	int result = IntersectRayPlane( camera.PosWC(), eyeDir[0], 1, 0.0f, &origin );
	if ( result != grinliz::INTERSECT ) {
		MoveCameraHome();
	}
	else {
		float len = ( engineData.cameraMin + z*(engineData.cameraMax-engineData.cameraMin) ) / eyeDir[0].y;
		Vector3F pos = origin + len*eyeDir[0];
		camera.SetPosWC( pos );
	}
}


float Engine::GetZoom() 
{
	float z = ( camera.PosWC().y - engineData.cameraMin ) / ( engineData.cameraMax - engineData.cameraMin );
	return z;
}

