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
	clipInUI2D.Set( 0, 0, UIWidth(), UIHeight() );
	clipInUI3D.Set( 0, 0, UIWidth(), UIHeight() );

	// Get the actual pixel size. Then adjust it to be the correct ratio.
	// The pixel ration of the screen is 480x320.
	glGetIntegerv( GL_VIEWPORT, (GLint*)viewport );
	if ( rotation & 1 ) 
		viewPortScale = (float)viewport[2] / (float)320;
	else
		viewPortScale = (float)viewport[2] / (float)480;
}


void Screenport::SetUI( const Rectangle2I* clip )	
{
	if ( clip ) {
		clipInUI2D = *clip;
	}
	else {
		clipInUI2D.Set( 0, 0, UIWidth(), UIHeight() );
	}

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
	uiMode = false;

	if ( clip ) {
		clipInUI3D = *clip;
	}
	else {
		clipInUI3D.Set( 0, 0, UIWidth(), UIHeight() );
	}

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
		GLASSERT( 0 );	// need to fix this...
		float ratio = (float)clipInUI3D.Height() / (float)clipInUI3D.Width();
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

		case 2:
		case 3:
		default:
			GLASSERT( 0 );
			break;
	}
}


void Screenport::ScreenToView( int x0, int y0, Vector2I* view ) const
{
	view->x = x0;
	view->y = (screenHeight-1)-y0;
}


bool Screenport::ScreenToWorld( const grinliz::Vector3F& s, const grinliz::Matrix4& mvpi, grinliz::Vector3F* world ) const
{
	Vector2I v;
	ScreenToView( LRintf(s.x), LRintf(s.y), &v );
	Vector4F in = { (float)(v.x-clipInUI3D.min.x)*2.0f / (float)clipInUI3D.Width() - 1.0f,
					(float)(v.y-clipInUI3D.min.y)*2.0f / (float)clipInUI3D.Height() - 1.0f,
					s.z*2.0f-1.f,
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
	Matrix4 mvpi;
	ViewProjectionInverse3D( &mvpi );

	Vector3F win0 = { (float)x, (float)y, 0 };
	Vector3F win1 = { (float)x, (float)y, 1 };
	Vector3F p0, p1;

	//UnProject( win0, clip, mvpi, &p0 );
	//UnProject( win1, clip, mvpi, &p1 );
	ScreenToWorld( win0, mvpi, &p0 );
	ScreenToWorld( win1, mvpi, &p1 );

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

	Rectangle2I clip;
	UIToScissor( clipInUI3D.min.x, clipInUI3D.min.y, clipInUI3D.Width(), clipInUI3D.Height(), &clip );

	// Normalize to view.
	// r in range [-1,1] which maps to screen[0,Width()-1]

	v->x = Interpolate( -1.0f, 0.0f,
						1.0f,  (float)clip.Width(),
						r.x/r.w );
	v->y = Interpolate( -1.0f, 0.0f,
						1.0f,  (float)clip.Height(),
						r.y/r.w );

//	v->x = (r.x / r.w + 1.0f)*(float)clip.Width()
//	v->y = (r.y / r.w + 1.0f)*(float)clip.Height()*0.5f;	
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
			clip->Set(	viewport[0] + LRintf( (float)y * viewPortScale ),
						viewport[1] + LRintf( (float)x * viewPortScale ), 
						viewport[0] + LRintf( (float)(y+h-1) * viewPortScale ),
						viewport[1] + LRintf( (float)(x+w-1) * viewPortScale ));
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
