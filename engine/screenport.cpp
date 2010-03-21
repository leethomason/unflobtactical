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
	clipInUI.Set( 0, 0, UIWidth(), UIHeight() );
}


void Screenport::SetUI( const Rectangle2I* clip )	
{
	if ( clip ) {
		clipInUI = *clip;
	}
	else {
		clipInUI.Set( 0, 0, UIWidth(), UIHeight() );
	}

	SetViewport( 0 );

	Rectangle2I scissor;
	UIToScissor( clipInUI.min.x, clipInUI.min.y, clipInUI.max.x, clipInUI.max.y, &scissor );
	glEnable( GL_SCISSOR_TEST );
	glScissor( scissor.min.x, scissor.min.y, scissor.max.x, scissor.max.y );
	
	view.SetIdentity();
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
	view = r*t;
	
	projection.SetIdentity();
	projection.SetOrtho( 0, (float)ScreenWidth(), 0, (float)ScreenHeight(), -100, 100 );

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();				// projection

	// Set the ortho matrix, help the driver
	//glMultMatrixf( projection.x );
	glOrthofX( 0, ScreenWidth(), 0, ScreenHeight(), -100, 100 );
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();				// model
	glMultMatrixf( view.x );

	uiMode = true;
	CHECK_GL_ERROR;
}


void Screenport::SetView( const Matrix4& _view )
{
	GLASSERT( uiMode == false );
	view = _view;

	glMatrixMode(GL_MODELVIEW);
	// In normalized coordinates.
	glLoadMatrixf( view.x );
	CHECK_GL_ERROR;
}


void Screenport::SetPerspective( float near, float far, float fovDegrees, const grinliz::Rectangle2I* clip )
{
	uiMode = false;

	if ( clip ) {
		clipInUI = *clip;
	}
	else {
		clipInUI.Set( 0, 0, UIWidth(), UIHeight() );
	}

	SetViewport( &clipInUI );

	GLASSERT( uiMode == false );
	GLASSERT( near > 0.0f );
	GLASSERT( far > near );

	frustum.zNear = near;
	frustum.zFar = far;

	// Convert from the FOV to the half angle.
	float theta = ToRadian( fovDegrees ) * 0.5f;

	// left, right, top, & bottom are on the near clipping
	// plane. (Not an obvious point to my mind.)
	
	if ( Rotation() & 1 ) {
		float ratio = (float)clipInUI.Height() / (float)clipInUI.Width();
		frustum.top		= tan(theta) * frustum.zNear;
		frustum.bottom	= -frustum.top;
		frustum.left	= ratio * frustum.bottom;
		frustum.right	= ratio * frustum.top;
	}
	else {
		float ratio = (float)clipInUI.Width() / (float)clipInUI.Height();
		frustum.top		= tan(theta) * frustum.zNear;
		frustum.bottom	= -frustum.top;
		frustum.left	= ratio * frustum.bottom;
		frustum.right	= ratio * frustum.top;
	}
	

	glMatrixMode(GL_PROJECTION);
	// In normalized coordinates.
	projection.SetFrustum( frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.zNear, frustum.zFar );
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

		case 2:
		case 3:
		default:
			GLASSERT( 0 );
			break;
	}
}


/* static */ bool Screenport::UnProject(	const Vector3F& window,
											const Rectangle2I& screen,
											const Matrix4& modelViewProjectionInverse,
											Vector3F* world )
{
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


void Screenport::ScreenToView( int x0, int y0, Vector2I* view ) const
{
	view->x = x0;
	view->y = (screenHeight-1)-y0;
}


void Screenport::ScreenToWorld( int x, int y, Ray* world ) const
{
	Matrix4 mvp, mvpi;
	MultMatrix4( projection, view, &mvp );
	mvp.Invert( &mvpi );

	Rectangle2I clip;
	UIToScissor( clipInUI.min.x, clipInUI.min.y, clipInUI.Width(), clipInUI.Height(), &clip );

	Vector3F win0 = { (float)x, (float)y, 0 };
	Vector3F win1 = { (float)x, (float)y, 1 };
	Vector3F p0, p1;

	UnProject( win0, clip, mvpi, &p0 );
	UnProject( win1, clip, mvpi, &p1 );

	world->origin = p0;
	world->direction = p1 - p0;

}


void Screenport::UIToScissor( int x, int y, int w, int h, grinliz::Rectangle2I* clip ) const
{
	if ( viewport[2] == 0 ) {
		glGetIntegerv( GL_VIEWPORT, (GLint*)viewport );
	}
	float wScale = (float)viewport[2]/(float)screenWidth;
	float hScale = (float)viewport[3]/(float)screenHeight;

	switch ( rotation ) {
		case 0:
			clip->Set(	viewport[0] + LRintf( (float)x * wScale ),
						viewport[1] + LRintf( (float)y * hScale ),
						viewport[0] + LRintf( (float)(x+w-1) * wScale ), 
						viewport[1] + LRintf( (float)(y+h-1) * hScale ) );
			break;
		case 1:
			clip->Set(	viewport[0] + LRintf( (float)y * hScale ),
						viewport[1] + LRintf( (float)x * wScale ), 
						viewport[0] + LRintf( (float)(y+h-1) * hScale ),
						viewport[1] + LRintf( (float)(x+w-1) * wScale ));
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
		glScissor( scissor.min.x, scissor.min.y, scissor.max.x, scissor.max.y );
		glViewport( scissor.min.x, scissor.min.y, scissor.max.x, scissor.max.y );
	}
	else {
		if ( viewport[2] == 0 ) {
			glGetIntegerv( GL_VIEWPORT, (GLint*)viewport );
		}
		glDisable( GL_SCISSOR_TEST );
		glViewport( viewport[0], viewport[1], viewport[2], viewport[3] );
	}
	CHECK_GL_ERROR;
}
