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

#include "screenport.h"
#include "platformgl.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glmatrix.h"

using namespace grinliz;


Screenport::Screenport( int screenWidth, int screenHeight, int rotation )
{
	this->screenWidth = screenWidth;
	this->screenHeight = screenHeight;
	this->rotation = rotation;
	GLASSERT( rotation >= 0 && rotation < 4 );

	for( int i=0; i<4; ++i )
		this->viewport[i] = 0;
	uiMode = false;
	clipInUI2D.Set( 0, 0, UIWidth()-1, UIHeight()-1 );
	clipInUI3D.Set( 0, 0, UIWidth()-1, UIHeight()-1 );
	viewPortScale = 0.0f;
}


void Screenport::SetUI( const Rectangle2I* clip )	
{
	if ( viewPortScale == 0.0f ) {
		// Get the actual pixel size. Then adjust it to be the correct ratio.
		// The pixel ration of the screen is 480x320.
		// Deferred to here; doesn't work on the iPhone in the initializer.
		glGetIntegerv( GL_VIEWPORT, (GLint*)viewport );
		if ( rotation & 1 ) 
			viewPortScale = (float)viewport[2] / (float)320;
		else
			viewPortScale = (float)viewport[2] / (float)480;
	}
	
	if ( clip ) {
		clipInUI2D = *clip;
	}
	else {
		clipInUI2D.Set( 0, 0, UIWidth()-1, UIHeight()-1);
	}
	GLASSERT( clipInUI2D.IsValid() );
	GLASSERT( clipInUI2D.min.x >= 0 && clipInUI2D.max.x < UIWidth() );
	GLASSERT( clipInUI2D.min.y >= 0 && clipInUI2D.max.y < UIHeight() );

	SetViewport( 0 );

	Rectangle2I scissor;
	UIToScissor( clipInUI2D.min.x, clipInUI2D.min.y, clipInUI2D.max.x, clipInUI2D.max.y, &scissor );
	glEnable( GL_SCISSOR_TEST );
	glScissor( scissor.min.x, scissor.min.y, scissor.max.x, scissor.max.y );
	
	view2D.SetIdentity();
	Matrix4 r, t;
	r.SetZRotation( (float)(90*Rotation() ));
	
	// the tricky bit. After rotating the ortho display, move it back on screen.
	switch (Rotation()) {
		case 0:
			break;
		case 1:
			t.SetTranslation( 0, (float)(-ScreenWidth()), 0 );	
			break;
			
		default:
			GLASSERT( 0 );	// work out...
			break;
	}
	view2D = r*t;
	
	projection2D.SetIdentity();
	projection2D.SetOrtho( 0, (float)ScreenWidth(), 0, (float)ScreenHeight(), -100, 100 );

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();				// projection

	// Set the ortho matrix, help the driver
	//glMultMatrixf( projection.x );
	glOrthofX( 0, ScreenWidth(), 0, ScreenHeight(), -100, 100 );
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();				// model
	glMultMatrixf( view2D.x );

	uiMode = true;
	CHECK_GL_ERROR;
}


void Screenport::SetView( const Matrix4& _view )
{
	GLASSERT( uiMode == false );
	view3D = _view;

	glMatrixMode(GL_MODELVIEW);
	// In normalized coordinates.
	glLoadMatrixf( view3D.x );
	CHECK_GL_ERROR;
}


void Screenport::SetPerspective( float near, float far, float fovDegrees, const grinliz::Rectangle2I* clip )
{
	if ( viewPortScale == 0.0f ) {
		// Get the actual pixel size. Then adjust it to be the correct ratio.
		// The pixel ration of the screen is 480x320.
		// Deferred to here; doesn't work on the iPhone in the initializer.
		glGetIntegerv( GL_VIEWPORT, (GLint*)viewport );
		if ( rotation & 1 ) 
			viewPortScale = (float)viewport[2] / (float)320;
		else
			viewPortScale = (float)viewport[2] / (float)480;
	}
	
	uiMode = false;

	if ( clip ) {
		clipInUI3D = *clip;
	}
	else {
		clipInUI3D.Set( 0, 0, UIWidth()-1, UIHeight()-1 );
	}
	GLASSERT( clipInUI3D.IsValid() );
	GLASSERT( clipInUI3D.min.x >= 0 && clipInUI3D.max.x < UIWidth() );
	GLASSERT( clipInUI3D.min.y >= 0 && clipInUI3D.max.y < UIHeight() );
	
	SetViewport( &clipInUI3D );

	GLASSERT( uiMode == false );
	GLASSERT( near > 0.0f );
	GLASSERT( far > near );

	frustum.zNear = near;
	frustum.zFar = far;

	// Convert from the FOV to the half angle.
	float theta = ToRadian( fovDegrees * 0.5f );
	float tanTheta = tanf( theta );
	float longSide = tanTheta * frustum.zNear;

	// left, right, top, & bottom are on the near clipping
	// plane. (Not an obvious point to my mind.)
	
	if ( Rotation() & 1 ) {
		float ratio = (float)clipInUI3D.Height() / (float)clipInUI3D.Width();
		
		// frustum is in original screen coordinates.
		frustum.top		=  longSide;
		frustum.bottom	= -longSide;
		
		frustum.left	= -ratio * longSide;
		frustum.right	=  ratio * longSide;
	}
	else {
		// Since FOV is specified as the 1/2 width, the ratio
		// is the height/width (different than gluPerspective)
		float ratio = (float)clipInUI3D.Height() / (float)clipInUI3D.Width();

		frustum.top		= ratio * longSide;
		frustum.bottom	= -frustum.top;

		frustum.left	= -longSide;
		frustum.right	=  longSide;
	}
	
	glMatrixMode(GL_PROJECTION);
	// In normalized coordinates.
	projection3D.SetFrustum( frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.zNear, frustum.zFar );
	// Give the driver hints:
	glLoadIdentity();
	glFrustumfX( frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.zNear, frustum.zFar );
	
	glMatrixMode(GL_MODELVIEW);	
	CHECK_GL_ERROR;
}


void Screenport::ViewToUI( int x0, int y0, int *x1, int *y1 ) const
{
	switch ( rotation ) {
		case 0:
			*x1 = x0;
			*y1 = y0;
			break;

		case 1:
			*x1 = y0;
			*y1 = (screenWidth-1)-x0;
			break;

		default:
			GLASSERT( 0 );
			break;
	}
}


void Screenport::UIToView( const grinliz::Vector2I& in, grinliz::Vector2I* out ) const
{
	switch ( rotation ) {
		case 0:
			*out = in;
			break;
		case 1:
			out->x = (screenWidth-1)-in.y;
			out->y = in.x;
			break;
		default:
			GLASSERT( 0 );
			break;
	}
}


bool Screenport::ViewToWorld( const grinliz::Vector3F& v, const grinliz::Matrix4& mvpi, grinliz::Vector3F* world ) const
{
	Vector2I v0, v1;
	Rectangle2I clipInView;
	UIToView( clipInUI3D.min, &v0 );
	UIToView( clipInUI3D.max, &v1 ); 
	clipInView.FromPair( v0.x, v0.y, v1.x, v1.y );
	
	Vector4F in = { (v.x - (float)(clipInView.min.x))*2.0f / (float)clipInView.Width() - 1.0f,
					(v.y - (float)(clipInView.min.y))*2.0f / (float)clipInView.Height() - 1.0f,
					v.z*2.0f-1.f,
					1.0f };

	Vector4F out;
	MultMatrix4( mvpi, in, &out );

	if ( out.w == 0.0f ) {
		return false;
	}
	world->x = out.x / out.w;
	world->y = out.y / out.w;
	world->z = out.z / out.w;
	return true;
}


void Screenport::ScreenToWorld( int x, int y, Ray* world ) const
{
	//GLOUTPUT(( "ScreenToWorld(upper) %d,%d\n", x, y ));

	Vector2I v;
	ScreenToView( x, y, &v );

	Matrix4 mvpi;
	ViewProjectionInverse3D( &mvpi );

	Vector3F win0 = { (float)v.x, (float)v.y, 0 };
	Vector3F win1 = { (float)v.x, (float)v.y, 1 };
	Vector3F p0, p1;

	ViewToWorld( win0, mvpi, &p0 );
	ViewToWorld( win1, mvpi, &p1 );

	world->origin = p0;
	world->direction = p1 - p0;
}


void Screenport::WorldToScreen( const grinliz::Vector3F& p0, grinliz::Vector2F* v ) const
{
	Matrix4 mvp;
	ViewProjection3D( &mvp );

	Vector4F p, r;
	p.Set( p0, 1 );

	r = mvp * p;

	// Normalize to view.
	// r in range [-1,1] which maps to screen[0,Width()-1]
	Vector2I v0, v1;
	Rectangle2I clipInView;
	UIToView( clipInUI3D.min, &v0 );
	UIToView( clipInUI3D.max, &v1 ); 
	clipInView.FromPair( v0.x, v0.y, v1.x, v1.y );
	
	v->x = Interpolate( -1.0f, (float)clipInView.min.x,
						1.0f,  (float)clipInView.max.x,
						r.x/r.w );
	v->y = Interpolate( -1.0f, (float)clipInView.min.y,
						1.0f,  (float)clipInView.max.y,
						r.y/r.w );
}


void Screenport::WorldToUI( const grinliz::Vector3F& p, grinliz::Vector2I* ui ) const
{
	Vector2F v;
	WorldToScreen( p, &v );
	ViewToUI( LRintf(v.x), LRintf(v.y), &ui->x, &ui->y );
}


void Screenport::UIToScissor( int x, int y, int w, int h, grinliz::Rectangle2I* clip ) const
{	
	switch ( rotation ) {
		case 0:
			clip->Set(	viewport[0] + LRintf( (float)x * viewPortScale ),
						viewport[1] + LRintf( (float)y * viewPortScale ),
						viewport[0] + LRintf( (float)(x+w-1) * viewPortScale ), 
						viewport[1] + LRintf( (float)(y+h-1) * viewPortScale ) );
			break;
		case 1:
			clip->Set(	viewport[2] - h - y,
					    viewport[1] + x,
						viewport[2] - y - 1,
						viewport[1] + x + w - 1 );
			break;
		case 2:
		case 3:
		default:
			GLASSERT( 0 );
			break;
	};
}



void Screenport::SetViewport( const grinliz::Rectangle2I* uiClip )
{
	if ( uiClip ) {
		Rectangle2I scissor;
		UIToScissor( uiClip->min.x, uiClip->min.y, uiClip->Width(), uiClip->Height(), &scissor );

		glEnable( GL_SCISSOR_TEST );
		glScissor( scissor.min.x, scissor.min.y, scissor.Width(), scissor.Height() );
		glViewport( scissor.min.x, scissor.min.y, scissor.Width(), scissor.Height() );
	}
	else {
		glDisable( GL_SCISSOR_TEST );
		glViewport( viewport[0], viewport[1], viewport[2], viewport[3] );
	}
	CHECK_GL_ERROR;
}
