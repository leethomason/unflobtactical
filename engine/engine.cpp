#include "engine.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glmath.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"
#include "platformgl.h"

using namespace grinliz;


Engine::Engine( int _width, int _height, const EngineData& _engineData ) 
	: width( _width ), height( _height ), isDragging( false ), engineData( _engineData )
{
	camera.SetPosWC( -5.0f, engineData.cameraHeight, (float)Map::SIZE+5.0f );
	camera.SetYRotation( -45.f );
	camera.SetTilt( engineData.cameraTilt );
	fov = engineData.fov;

	// model[0] is the sentinel for the available model pool
	modelPool[0].next = &modelPool[1];
	modelPool[0].prev = &modelPool[EL_MAX_MODELS-1];
	modelPool[EL_MAX_MODELS-1].next = &modelPool[0];
	modelPool[EL_MAX_MODELS-1].prev = &modelPool[EL_MAX_MODELS-2];

	for( int i=1; i<(EL_MAX_MODELS-1); ++i ) {
		modelPool[i].prev = &modelPool[i-1];
		modelPool[i].next = &modelPool[i+1];
	}

	SetPerspective();
}

Engine::~Engine()
{
#ifdef DEBUG
	// Un-released models?
	int count = 0;
	for( Model* model=modelPool[0].next; model != &modelPool[0]; model=model->next ) {
		++count;
	}
	GLASSERT( count == EL_MAX_MODELS-1 );
#endif
}

Model* Engine::GetModel( ModelResource* resource )
{
	GLASSERT( resource );
	GLASSERT( modelPool[0].next != &modelPool[0] );

	if ( modelPool[0].next != &modelPool[0] ) {
		Model* model = modelPool[0].next;
		// Unlink.
		model->prev->next = model->next;
		model->next->prev = model->prev;
		model->next = model->prev = 0;

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
	model->prev = &modelPool[0];
	model->next = modelPool[0].next;
	modelPool[0].next->prev = model;
	modelPool[0].next = model;
}


void Engine::DrawCamera()
{
	// ---- Model-View --- //
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	const Matrix4& m = camera.CameraMat();

	// Rotation step
	glMultMatrixf(m.x);

	// Translate the eye to the origin
	const Vector3F& cameraWC = camera.PosWC();
	glTranslatef( -cameraWC.x, -cameraWC.y, -cameraWC.z );
}


void Engine::Draw()
{
	DrawCamera();
	// Render components
	map.Draw();
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


void Engine::DragStart( int x, int y )
{

	GLASSERT( !isDragging );
	isDragging = true;

	Matrix4 mvpi;
	CalcModelViewProjectionInverse( &dragMVPI );
	RayFromScreenToYPlane( x, y, dragMVPI, &dragStart );
	dragStartCameraWC = camera.PosWC();
	GLOUTPUT(( "Drag start %.1f,%.1f\n", dragStart.x, dragStart.z ));
}


void Engine::DragMove( int x, int y )
{
	GLASSERT( isDragging );

	Vector3F drag;
	RayFromScreenToYPlane( x, y, dragMVPI, &drag );

	Vector3F delta = drag - dragStart;
	delta.y = 0.0f;

	//GLOUTPUT(( "delta %.1f,%.1f  deltaC %.1f,%.1f,%.f\n", delta.x, delta.z,
	//		   deltaC.x, deltaC.y, deltaC.z ));

	camera.SetPosWC( dragStartCameraWC - delta );
}


void Engine::DragEnd( int x, int y )
{
	GLASSERT( isDragging );
	DragMove( x, y );
	isDragging = false;
	GLOUTPUT(( "Drag end\n" ));
}
