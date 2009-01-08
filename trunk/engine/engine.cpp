#include "engine.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glmath.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"
#include "platformgl.h"
#include "../game/cgame.h"

using namespace grinliz;


Engine::Engine( int _width, int _height, const EngineData& _engineData ) 
	:	width( _width ), 
		height( _height ), 
		shadowMode( SHADOW_Z ), 
		isDragging( false ), 
		engineData( _engineData ),
		initZoomDistance( 0 )
{
	camera.SetPosWC( -5.0f, engineData.cameraHeight, (float)Map::SIZE + 5.0f );
	camera.SetYRotation( -45.f );
	camera.SetTilt( engineData.cameraTilt );
	fov = engineData.fov;

	// The ray runs from the min to the max, with the current (and default)
	// zoom specified in the engineData.
	Ray ray;
	camera.CalcEyeRay( &ray );
	
	float t1 = ( engineData.cameraMin - ray.origin.y ) / ray.direction.y;
	float t0 = ( engineData.cameraMax - ray.origin.y ) / ray.direction.y;

	cameraRay.origin = ray.origin + t0*ray.direction;
	cameraRay.direction = ray.direction;
	cameraRay.length = t1-t0;

	zoom = ( engineData.cameraHeight - cameraRay.origin.y ) / cameraRay.direction.y;
	zoom /= cameraRay.length;
	defaultZoom = zoom;

	// Link up the circular list of models.
	// First, link the sentinel to itself:
	modelPoolRoot.next = &modelPoolRoot;
	modelPoolRoot.prev = &modelPoolRoot;
	// Then everyone else:
	for( int i=0; i<EL_MAX_MODELS; ++i ) {
		modelPool[i].Link( &modelPoolRoot );
	}

	SetPerspective();
	lightDirection.Set( 1.0f, 3.0f, 2.0f );
	lightDirection.Normalize();
}

Engine::~Engine()
{
#ifdef DEBUG
	// Un-released models?
	int count = 0;
	for( Model* model=modelPoolRoot.next; model != &modelPoolRoot; model=model->next ) {
		++count;
	}
	GLASSERT( count == EL_MAX_MODELS );
#endif
}


Model* Engine::GetModel( ModelResource* resource )
{
	GLASSERT( resource );
	GLASSERT( modelPoolRoot.next != &modelPoolRoot );	// All tapped out!!

	if ( modelPoolRoot.next != &modelPoolRoot ) {
		Model* model = modelPoolRoot.next;
		model->UnLink();
		model->Init( resource );
		return model;
	}
	return 0;
}

void Engine::ReleaseModel( Model* model )
{
	GLASSERT( model->next == 0 );
	GLASSERT( model->prev == 0 );
	// Link
	model->Link( &modelPoolRoot );
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
	
	map.Draw();

	// -- Shadow casters/ground plane -- //
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

	for( int i=0; i<EL_MAX_MODELS; ++i ) {	// OPT: not all models are always used.
		if ( modelPool[i].ShouldDraw() ) {
			modelPool[i].Draw( false );
		}
	}
	glPopMatrix();

	
	EnableLights( true, true );
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_LIGHTING );

	if ( shadowMode == SHADOW_DST_BLEND ) {
		glEnable( GL_BLEND );
		glBlendFunc( GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA );
		map.Draw();
		glDisable( GL_BLEND );
	}
	else if ( shadowMode == SHADOW_Z ) {
		glDepthMask( GL_TRUE );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LESS );
		map.Draw();
	}
	
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_LIGHTING );
	glDepthFunc( depthFunc );
	
	// -- Model -- //
	EnableLights( true, false );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );

	for( int i=0; i<EL_MAX_MODELS; ++i ) {	// OPT: not all models are always used.
		if ( modelPool[i].ShouldDraw() ) {
			modelPool[i].Draw();
		}
	}
	
	EnableLights( false );
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
	//

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


void Engine::RayFromScreenToYPlane( int x, int y, const Matrix4& mvpi, Vector3F* out )
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

	float t;
	IntersectLinePlane( p0, p1, plane, out, &t );
}


void Engine::Drag( int action, int x, int y )
{
	switch ( action ) 
	{
		case GAME_DRAG_START:
		{
			GLASSERT( !isDragging );
			isDragging = true;

			Matrix4 mvpi;
			CalcModelViewProjectionInverse( &dragMVPI );
			RayFromScreenToYPlane( x, y, dragMVPI, &dragStart );
			dragStartCameraWC = camera.PosWC();
			GLOUTPUT(( "Drag start %.1f,%.1f\n", dragStart.x, dragStart.z ));
		}
		break;

		case GAME_DRAG_MOVE:
		{
			GLASSERT( isDragging );

			Vector3F drag;
			RayFromScreenToYPlane( x, y, dragMVPI, &drag );
			
			Vector3F delta = drag - dragStart;
			delta.y = 0.0f;

			camera.SetPosWC( dragStartCameraWC - delta );
			RestrictCamera();
		}
		break;

		case GAME_DRAG_END:
		{
			GLASSERT( isDragging );
			Drag( GAME_DRAG_MOVE, x, y );
			isDragging = false;
			GLOUTPUT(( "Drag end\n" ));
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
			GLOUTPUT(( "initZoomStart=%.2f distance=%d initDist=%d\n", initZoom, distance, initZoomDistance ));
			break;

		case GAME_ZOOM_MOVE:
			{
				float z = initZoom * (float)distance / (float)initZoomDistance;
				GLOUTPUT(( "initZoom=%.2f distance=%d initDist=%d\n", initZoom, distance, initZoomDistance ));
				SetZoom( z );
			}
			break;

		default:
			GLASSERT( 0 );
			break;
	}
}


void Engine::RestrictCamera()
{
	Ray ray;
	camera.CalcEyeRay( &ray );
	Vector3F intersect;
	IntersectRayPlane( ray.origin, ray.direction, XZ_PLANE, 0.0f, &intersect );
	GLOUTPUT(( "Intersect %.1f, %.1f, %.1f\n", intersect.x, intersect.y, intersect.z ));

	const float SIZE = (float)Map::SIZE;

	if ( intersect.x < 0.0f ) {
		camera.DeltaPosWC( -intersect.x, 0.0f, 0.0f );
	}
	if ( intersect.x > SIZE ) {
		camera.DeltaPosWC( -(intersect.x-SIZE), 0.0f, 0.0f );
	}
	if ( intersect.z < 0.0f ) {
		camera.DeltaPosWC( 0.0f, 0.0f, -intersect.z );
	}
	if ( intersect.z > SIZE ) {
		camera.DeltaPosWC( 0.0f, 0.0f, -(intersect.z-SIZE) );
	}
}



void Engine::SetZoom( float z )
{
	z = Clamp( z, 0.0f, 1.0f );

	cameraRay.origin = camera.PosWC() - zoom*cameraRay.length*cameraRay.direction;
	zoom = z;
	camera.SetPosWC( cameraRay.origin + zoom*cameraRay.length*cameraRay.direction );
	GLOUTPUT(( "zoom=%.2f y=%.2f\n", zoom, camera.PosWC().y ));
}
