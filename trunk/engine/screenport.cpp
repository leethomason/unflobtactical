#include "screenport.h"
#include "platformgl.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glmatrix.h"

using namespace grinliz;

void Screenport::PushUI()	
{
	savedProjection = projection;
	savedView = view;
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();					// save projection
	glLoadIdentity();				// projection
	projection.SetIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();					// model
	glLoadIdentity();				// model

	Matrix4 rotate;
	rotate.SetZRotation( 90.0f * (float)Rotation() );

	Matrix4 ortho;
	ortho.SetOrtho( 0, (float)UIWidth(), 0, (float)UIHeight(), -100, 100 );
	view = rotate*ortho;

	glMultMatrixf( view.x );
	uiPushed = true;
}

void Screenport::PopUI()
{

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();					// model
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();					// projection
	glMatrixMode(GL_MODELVIEW);

	projection = savedProjection;
	view = savedView;
	uiPushed = false;
}


void Screenport::SetView( const Matrix4& _view )
{
	GLASSERT( uiPushed == false );
	view = _view;

	glMatrixMode(GL_MODELVIEW);
	// In normalized coordinates.
	glLoadMatrixf( view.x );
}


void Screenport::SetPerspective( float frustumLeft, float frustumRight, float frustumBottom, float frustumTop, float frustumNear, float frustumFar )
{
	GLASSERT( uiPushed == false );

	glMatrixMode(GL_PROJECTION);
	// In normalized coordinates.
	projection.SetFrustum( frustumLeft, frustumRight, frustumBottom, frustumTop, frustumNear, frustumFar );
	glLoadMatrixf( projection.x );
	
	glMatrixMode(GL_MODELVIEW);	
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


void Screenport::ScreenToView( int x0, int y0, int *x1, int *y1 ) const
{
//	GLASSERT( x0>=0 && x0<physicalWidth );
//	GLASSERT( y0>=0 && y0<physicalHeight );

	*x1 = x0;
	*y1 = (screenHeight-1)-y0;
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
}
