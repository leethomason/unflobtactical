
#include "../grinliz/glutil.h"
#include "../grinliz/glmath.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"

#include "../game/cgame.h"

#include "platformgl.h"
#include "engine.h"
#include "loosequadtree.h"
#include "renderQueue.h"

using namespace grinliz;

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
 
	Ideas:	1. Better bounding tests. Bounding boxes are still quite loose.
			2. Replace characters with billboards.
			3. Triangle reduction of the background.	[done - switched to texture]
			4. Background as a single texture.			[done]
			5. Is it possible to do planar shadows in one pass? Map from vertex to texture coordinates?
*/


Engine::Engine( int _width, int _height, const EngineData& _engineData ) 
	:	width( _width ), 
		height( _height ), 
		shadowMode( SHADOW_ONE_PASS ), 
		//shadowMode( SHADOW_Z ), 
		isDragging( false ), 
		engineData( _engineData ),
		initZoomDistance( 0 ),
		lastZoomDistance( 0 )
{
	spaceTree = new SpaceTree( Fixed(-0.1f), Fixed(3) );
	map = new Map( spaceTree );
	renderQueue = new RenderQueue();

	camera.SetPosWC( -5.0f, engineData.cameraHeight, (float)Map::SIZE + 5.0f );
	camera.SetYRotation( -45.f );
	camera.SetTilt( engineData.cameraTilt );
	fov = engineData.fov;

	// The ray runs from the min to the max, with the current (and default)
	// zoom specified in the engineData.
	Ray ray;
	camera.CalcEyeRay( &ray, 0, 0 );
	
	float t1 = ( engineData.cameraMin - ray.origin.y ) / ray.direction.y;
	float t0 = ( engineData.cameraMax - ray.origin.y ) / ray.direction.y;

	cameraRay.origin = ray.origin + t0*ray.direction;
	cameraRay.direction = ray.direction;
	cameraRay.length = t1-t0;

	zoom = ( engineData.cameraHeight - cameraRay.origin.y ) / cameraRay.direction.y;
	zoom /= cameraRay.length;
	defaultZoom = zoom;

	SetPerspective();
	lightDirection.Set( 0.7f, 3.0f, 1.4f );
	lightDirection.Normalize();
	depthFunc = 0;
}


Engine::~Engine()
{
	delete renderQueue;
	delete map;
	delete spaceTree;
}


void Engine::MoveCameraHome()
{
	camera.SetPosWC( -5.0f, engineData.cameraHeight, (float)map->Height() + 5.0f );
	camera.SetYRotation( -45.f );
	camera.SetTilt( engineData.cameraTilt );
	zoom = defaultZoom;
}


void Engine::MoveCameraXZ( float x, float z )
{
	Ray ray;
	camera.CalcEyeRay( &ray, 0, 0 );

	Vector3F start = { x, 0.0f, z };
	Vector3F pos = start - ray.direction*engineData.cameraHeight*1.4f;

	camera.SetPosWC( pos.x, pos.y, pos.z );
	camera.SetYRotation( -45.f );
	camera.SetTilt( engineData.cameraTilt );
	zoom = defaultZoom;
}


Model* Engine::GetModel( ModelResource* resource )
{
	GLASSERT( resource );
	return spaceTree->AllocModel( resource );
}


void Engine::ReleaseModel( Model* model )
{
	spaceTree->FreeModel( model );
}


void Engine::DrawCamera()
{
	// ---- Model-View --- //
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	camera.DrawCamera();
}


void Engine::PushShadowMatrix()
{
	Matrix4 m;
	m.m12 = -lightDirection.x/lightDirection.y;
	m.m22 = 0.0f;
	if ( shadowMode == SHADOW_Z ) {
		m.m24 = -0.05f;		// y term down
		m.m34 = -0.05f;		// z term - hide the shift error.
	}
	m.m32 = -lightDirection.z/lightDirection.y;

	glPushMatrix();
	glMultMatrixf( m.x );
}


void Engine::ShadowTextureMatrix( Matrix4* c )
{
	Matrix4 t, m;

	float SIZEinv = 1.0f / (float)Map::SIZE;

	// XYZ -> Planar XZ
	/*
	m.m12 = -lightDirection.x/lightDirection.y;
	m.m22 = 0.0f;
	m.m32 = -lightDirection.z/lightDirection.y;
	*/

	// Planar XZ -> UV
	t.m11 = SIZEinv;

	t.m21 = 0.0f;
	t.m22 = 0.0f;
	t.m23 = -SIZEinv;
	t.m24 = 1.0f;

	t.m33 = 0.0f;
	
	//m.Dump();
	//t.Dump();
	//c.Dump();

	//Vector3F testIn = { 64.0f, 1.0f, 64.0f };
	//Vector3F testOut = c * testIn;

	*c = t;	//t*m;
}


void Engine::Draw( int* triCount )
{
	Matrix4 textureMatrix;

	DrawCamera();
	glEnable( GL_DEPTH_TEST );
	if ( depthFunc == 0 ) {
		// Not clear what the default is (from the driver): LEQUAL or LESS. So query and cache.
		glGetIntegerv( GL_DEPTH_FUNC, &depthFunc );	
	}

	/*
		Render phases.					z-write	z-test	lights		notes

		Ground plane lighted			yes		no		light
		Shadow casters/ground plane		no		no		shadow
		Model							yes		yes		light

		(Tree)
		(Particle)

	*/
	glDisable( GL_LIGHTING );

	// -- Ground plane lighted -- //
	LightSimple( false );

	// The depth mask and the depth test should be completely
	// independent. But there seems to be bugs across systems 
	// on this. Rather than figure it all out, set them as one
	// state.
	if ( shadowMode == SHADOW_ONE_PASS ) {
		glDisable( GL_DEPTH_TEST );
		glDepthMask( GL_FALSE );
	}
	else {
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
	}

	// Compute the frustum planes
	Plane planes[6];
	CalcFrustumPlanes( planes );
	PlaneX planesX[6];
	for( int i=0; i<6; ++i ) {
		planesX[i].Convert( planes[i] );
	}
	Model* modelRoot = spaceTree->Query( planesX, 6 );
	
	map->Draw( renderQueue );

	// -- Shadow casters/ground plane -- //
	// Set up the planar projection matrix, with a little z offset
	// to help with z resolution fighting.
	PushShadowMatrix();

	int textureState = 0;
	if ( shadowMode == SHADOW_Z ) {
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_LIGHTING );
		glDepthFunc( GL_ALWAYS );
		textureState = Model::NO_TEXTURE;
		glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );	// keeps white from bleeding outside the map.
	}
	else {
		glBindTexture( GL_TEXTURE_2D, map->GetMapGUID() );
		textureState = Model::TEXTURE_SET;

		renderQueue->BindTextureToVertex( true );
		ShadowTextureMatrix( &textureMatrix );
		renderQueue->SetTextureMatrix( &textureMatrix );
		LightSimple( true );

		//glDisable( GL_DEPTH_TEST );
		//glDepthMask( GL_FALSE );
	}
	GLASSERT( renderQueue->Empty() );
	
	for( Model* model=modelRoot; model; model=model->next ) {
		// Draw model shadows.
		model->Queue( renderQueue, textureState );
	}

	renderQueue->Flush();
	renderQueue->BindTextureToVertex( false );
	CHECK_GL_ERROR;

	renderQueue->SetTextureMatrix( 0 );

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	glPopMatrix();
	
	// Redraw the map, checking z, in shadow.
	if ( shadowMode == SHADOW_Z )
	{
		EnableLights( true, true );
		glEnable( GL_TEXTURE_2D );
		glEnable( GL_LIGHTING );
		CHECK_GL_ERROR;

		glDepthMask( GL_TRUE );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LESS );

		map->Draw( renderQueue );
		CHECK_GL_ERROR;
	}

	CHECK_GL_ERROR;
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_LIGHTING );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glDepthFunc( depthFunc );
	CHECK_GL_ERROR;

	// -- Model -- //
	EnableLights( true, false );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );

	//debug = 0;
	for( Model* model=modelRoot; model; model=model->next ) {
		model->Queue( renderQueue, Model::MODEL_TEXTURE );
	}
	renderQueue->Flush();
	
	EnableLights( false );

	*triCount = renderQueue->GetTriCount();
	renderQueue->ClearTriCount();

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
}


const float gAMBIENT = 0.3f;
const float gDIFFUSE = 0.7f;
const float gSHADOW  = 0.3f;
	
void Engine::EnableLights( bool enable, bool inShadow )
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
		float ambient[4] = { gAMBIENT, gAMBIENT, gAMBIENT, 1.0f };
		float diffuse[4] = { gDIFFUSE, gDIFFUSE, gDIFFUSE, 1.0f };
		Vector3F lightDir = lightDirection;

		if ( inShadow ) {
			diffuse[0] *= gSHADOW;
			diffuse[1] *= gSHADOW;
			diffuse[2] *= gSHADOW;
			diffuse[3] = 1.0f;
		}

		float lightVector4[4] = { lightDir.x, lightDir.y, lightDir.z, 0.0 };	// parallel

		CHECK_GL_ERROR;
		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

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


void Engine::LightSimple( bool inShadow )
{
	const Vector3F normal = { 0.0f, 1.0f, 0.0f };	
	float dot = DotProduct( lightDirection, normal );

	float diffuse = gDIFFUSE;
	if ( inShadow ) {
		// FIXME: not sure what I'm missing in the light equation, but this is too dark.
		// The main reason LightSimple is here is that lighting doesn't look correct in shadows.
		// Something is getting tweaked in the state. (Not a bug, just the difficulties of
		// fixed pipeline lightning. Oh how I miss shaders.)
		//diffuse *= SHADOW;
		diffuse *= 0.6f;
	}
	float light = gAMBIENT + diffuse*dot;

	CHECK_GL_ERROR;
	glColor4f( light, light, light, 1.0f );
}


void Engine::SetPerspective()
{
	float nearPlane = engineData.nearPlane;
	float farPlane = engineData.farPlane;

	GLASSERT( nearPlane > 0.0f );
	GLASSERT( farPlane > nearPlane );
	GLASSERT( fov > 0.0f && fov < 90.0f );

	frustumNear = nearPlane;
	frustumFar = farPlane;

	// Convert from the FOV to the half angle.
	float theta = ToRadian( fov ) * 0.5f;

	// left, right, top, & bottom are on the near clipping
	// plane. (Not an obvious point to my mind.)
	float aspect = (float)(width) / (float)(height);
	frustumTop		= tan(theta) * nearPlane;
	frustumBottom	= -frustumTop;
	frustumLeft		= aspect * frustumBottom;
	frustumRight	= aspect * frustumTop;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustumfX( frustumLeft, frustumRight, frustumBottom, frustumTop, frustumNear, frustumFar );
	
	glMatrixMode(GL_MODELVIEW);
}

bool Engine::UnProject(	const Vector3F& window,
						const Rectangle2I& screen,
						const Matrix4& modelViewProjectionInverse,
						Vector3F* world )
{
	//in[0] = (winx - viewport[0]) * 2 / viewport[2] - 1.0;
	//in[1] = (winy - viewport[1]) * 2 / viewport[3] - 1.0;
	//in[2] = 2 * winz - 1.0;
	//in[3] = 1.0;
	Vector4F in = { (window.x-(float)screen.min.x)*2.0f / float(screen.Width())-1.0f,
					(window.y-(float)screen.min.y)*2.0f / float(screen.Height())-1.0f,
					window.z*2.0f-1.f,
					1.0f };

	Vector4F out;
	MultMatrix4( modelViewProjectionInverse, in, &out );

	if ( out.w == 0.0f ) {
		return false;
	}
	world->x = out.x / out.w;
	world->y = out.y / out.w;
	world->z = out.z / out.w;
	return true;
}


void Engine::CalcModelViewProjectionInverse( grinliz::Matrix4* modelViewProjectionInverse )
{
	Matrix4 modelView;
	glGetFloatv( GL_MODELVIEW_MATRIX, &modelView.x[0] );
	Matrix4 projection;
	glGetFloatv( GL_PROJECTION_MATRIX, &projection.x[0] );

	Matrix4 mvp;
	MultMatrix4( projection, modelView, &mvp );
	mvp.Invert( modelViewProjectionInverse );
}


void Engine::RayFromScreenToYPlane( int x, int y, const Matrix4& mvpi, Ray* ray, Vector3F* out )
{	
	Rectangle2I screen;
	screen.Set( 0, 0, width-1, height-1 );
	Vector3F win0 ={ (float)x, (float)y, 0.0f };
	Vector3F win1 ={ (float)x, (float)y, 1.0f };

	Vector3F p0, p1;

	UnProject( win0, screen, mvpi, &p0 );
	UnProject( win1, screen, mvpi, &p1 );

	Plane plane;
	plane.n.Set( 0.0f, 1.0f, 0.0f );
	plane.d = 0.0;
	
	Vector3F dir = p1 - p0;

	ray->origin = p0;
	ray->direction = dir;
	ray->length = 1.0f;

	float t;
	IntersectLinePlane( p0, p1, plane, out, &t );
}

/*
void Engine::CalcFrustumPlanes( grinliz::Plane* planes )
{
	Ray forward, up, right;
	camera.CalcEyeRay( &forward, &up, &right );

	Vector3F ntl = forward.origin + forward.direction*frustumNear + up.direction*frustumTop    + right.direction*frustumLeft;
	Vector3F ntr = forward.origin + forward.direction*frustumNear + up.direction*frustumTop    + right.direction*frustumRight;
	Vector3F nbl = forward.origin + forward.direction*frustumNear + up.direction*frustumBottom + right.direction*frustumLeft;
	Vector3F nbr = forward.origin + forward.direction*frustumNear + up.direction*frustumBottom + right.direction*frustumRight;

	Plane::CreatePlane( forward.direction,  forward.origin + forward.direction*frustumNear, &planes[NEAR] );
	Plane::CreatePlane( -forward.direction, forward.origin + forward.direction*frustumFar,  &planes[FAR] );

	Plane::CreatePlane( camera.PosWC(), nbl, ntl, &planes[LEFT] );
	Plane::CreatePlane( camera.PosWC(), ntr, nbr, &planes[RIGHT] );
	Plane::CreatePlane( camera.PosWC(), ntl, ntr, &planes[TOP] );
	Plane::CreatePlane( camera.PosWC(), nbr, nbl, &planes[BOTTOM] );
}
*/


void Engine::CalcFrustumPlanes( grinliz::Plane* planes )
{
	Matrix4 projection;
	glGetFloatv( GL_PROJECTION_MATRIX, projection.x );
	Matrix4 modelView;
	glGetFloatv( GL_MODELVIEW_MATRIX, modelView.x );

	// --------- Compute the view frustum ----------- //
	// A strange and ill-documented algorithm from Real Time Rendering, 2nd ed, pg.613
	Matrix4 m;
	MultMatrix4( projection, modelView, &m );

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


Model* Engine::IntersectModel( const grinliz::Ray& ray, bool onlyDraggable )
{
	int FAR = 10*1000;
	Fixed close( FAR );
	Model* m = 0;

	Vector3X origin, dir;
	ConvertVector3( ray.origin, &origin );
	ConvertVector3( ray.direction, &dir );

	Model* root = spaceTree->Query( origin, dir );

	for( ; root; root=root->next )
	{
		if ( !onlyDraggable || root->IsDraggable() ) {
			Vector3X intersect;
			Rectangle3X aabb;
			Fixed t;

			root->CalcHitAABB( &aabb );
			//GLOUTPUT(( "AABB: %.2f,%.2f,%.2f  %.2f,%.2f,%.2f\n", (float)aabb.min.x, (float)aabb.min.y, (float)aabb.min.z,
			//			(float)aabb.max.x, (float)aabb.max.y, (float)aabb.max.z ));
			
			int result = IntersectRayAABBX( origin, dir, aabb, &intersect, &t );
			//GLOUTPUT(( "  result=%d\n", result ));
			if ( result == grinliz::INTERSECT ) {
				if ( t < close ) {
					m = root;
					close = t;
				}
			}
		}
	}
	return m;
}


void Engine::Drag( int action, int x, int y )
{
	switch ( action ) 
	{
		case GAME_DRAG_START:
		{
			GLASSERT( !isDragging );
			isDragging = true;
			draggingModel = 0;

			Matrix4 mvpi;
			Ray ray;

			CalcModelViewProjectionInverse( &dragMVPI );
			RayFromScreenToYPlane( x, y, dragMVPI, &ray, &dragStart );

			draggingModel = IntersectModel( ray, true );
			//GLOUTPUT(( "Model=%x\n", draggingModel ));

			if ( draggingModel ) {
				draggingModelOrigin = draggingModel->Pos();
			}
			else {
				dragStartCameraWC = camera.PosWC();
			}
//			GLOUTPUT(( "Drag start %.1f,%.1f\n", dragStart.x, dragStart.z ));
		}
		break;

		case GAME_DRAG_MOVE:
		{
			GLASSERT( isDragging );

			Vector3F drag;
			Ray ray;
			RayFromScreenToYPlane( x, y, dragMVPI, &ray, &drag );

			Vector3F delta = drag - dragStart;
			delta.y = 0.0f;

			if ( draggingModel ) {
				int dx = LRintf( delta.x );
				int dz = LRintf( delta.z );
				draggingModel->SetPos(	draggingModelOrigin.x + Fixed(dx),
										draggingModelOrigin.y,
										draggingModelOrigin.z + Fixed(dz) );
			}
			else {
				camera.SetPosWC( dragStartCameraWC - delta );
				RestrictCamera();
			}
		}
		break;

		case GAME_DRAG_END:
		{
			GLASSERT( isDragging );
			Drag( GAME_DRAG_MOVE, x, y );
			isDragging = false;
//			GLOUTPUT(( "Drag end\n" ));
		}
		break;

		default:
			GLASSERT( 0 );
			break;
	}
}


void Engine::Zoom( int action, int distance )
{
	switch ( action )
	{
		case GAME_ZOOM_START:
			initZoomDistance = distance;
			initZoom = zoom;
//			GLOUTPUT(( "initZoomStart=%.2f distance=%d initDist=%d\n", initZoom, distance, initZoomDistance ));
			break;

		case GAME_ZOOM_MOVE:
			{
				lastZoomDistance = distance;
				//float z = initZoom * (float)distance / (float)initZoomDistance;	// original. wrong feel.
				float z = initZoom + (float)(distance-initZoomDistance)/800.0f;	// better, but slow out zoom-out, fast at zoom-in
				
				//float z0 = initZoom + (float)(distance-initZoomDistance)/1600.0f;
				//float z1 = initZoom + (float)(distance-initZoomDistance)/200.0f;
				//float z = (1.0f-GetZoom())*z0 + (GetZoom())*z1;
				
//				GLOUTPUT(( "initZoom=%.2f distance=%d initDist=%d\n", initZoom, distance, initZoomDistance ));
				SetZoom( z );
			}
			break;

		default:
			GLASSERT( 0 );
			break;
	}
	//GLOUTPUT(( "Zoom action=%d distance=%d initZoomDistance=%d lastZoomDistance=%d z=%.2f\n",
	//		   action, distance, initZoomDistance, lastZoomDistance, GetZoom() ));
}


void Engine::RestrictCamera()
{
	Ray ray;
	camera.CalcEyeRay( &ray, 0, 0 );
	Vector3F intersect;
	IntersectRayPlane( ray.origin, ray.direction, XZ_PLANE, 0.0f, &intersect );
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

	cameraRay.origin = camera.PosWC() - zoom*cameraRay.length*cameraRay.direction;
	zoom = z;
	camera.SetPosWC( cameraRay.origin + zoom*cameraRay.length*cameraRay.direction );
//	GLOUTPUT(( "zoom=%.2f y=%.2f\n", zoom, camera.PosWC().y ));
}
