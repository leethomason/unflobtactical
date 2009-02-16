
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


Engine::Engine( int _width, int _height, const EngineData& _engineData ) 
	:	width( _width ), 
		height( _height ), 
		shadowMode( SHADOW_Z ), 
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


void Engine::Draw()
{
	DrawCamera();

	/*
		Render phases.					z-write	z-test	lights		notes

		Ground plane lighted			yes		no		light
		Shadow casters/ground plane		no		no		shadow
		Model							yes		yes		light

		(Tree)
		(Particle)

	*/

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	// -- Ground plane lighted -- //
	EnableLights( true, false );

	// The depth mask and the depth test should be completely
	// independent. But there seems to be bugs across systems 
	// on this. Rather than figure it all out, set them as one
	// state.
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	
	map->Draw();

	// -- Shadow casters/ground plane -- //
	// Set up the planar projection matrix, with a little z offset
	// to help with z resolution fighting.
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

	glDisable( GL_TEXTURE_2D );
	glDisable( GL_LIGHTING );

	// Not clear what the default is (from the driver): LEQUAL or LESS. So query.
	int depthFunc;
	glGetIntegerv( GL_DEPTH_FUNC, &depthFunc );
	glDepthFunc( GL_ALWAYS );

	// Compute the frustum planes
	Plane planes[6];
	CalcFrustumPlanes( planes );
	PlaneX planesX[6];
	for( int i=0; i<6; ++i ) {
		planesX[i].Convert( planes[i] );
	}


	Model* modelRoot = spaceTree->Query( planesX, 6 );
	GLASSERT( renderQueue->Empty() );

	glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );	// keeps white from bleeding outside the map.
	for( Model* model=modelRoot; model; model=model->next ) {
		//model->Draw( false );	// shadow pass.
		model->Queue( renderQueue, false );
	}
	renderQueue->Flush();

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	glPopMatrix();

	
	EnableLights( true, true );
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_LIGHTING );

	if ( shadowMode == SHADOW_DST_BLEND ) {
		glEnable( GL_BLEND );
		glBlendFunc( GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA );
		map->Draw();
		glDisable( GL_BLEND );
	}
	else if ( shadowMode == SHADOW_Z ) {
		glDepthMask( GL_TRUE );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LESS );
		map->Draw();
	}
	
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_LIGHTING );
	glDepthFunc( depthFunc );
	
	// -- Model -- //
	EnableLights( true, false );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );

	for( Model* model=modelRoot; model; model=model->next ) {
		//model->Draw( true );
		model->Queue( renderQueue, true );
	}
	renderQueue->Flush();
	
	EnableLights( false );

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
		float ambient[4]  = { 0.3f, 0.3f, 0.3f, 1.0f };
		float diffuse[4]	= { 0.7f, 0.7f, 0.7f, 1.0f };
		Vector3F lightDir = lightDirection;

		if ( inShadow ) {
			const float SHADOW_FACTOR = 0.2f;
			diffuse[0] *= SHADOW_FACTOR;
			diffuse[1] *= SHADOW_FACTOR;
			diffuse[2] *= SHADOW_FACTOR;
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
