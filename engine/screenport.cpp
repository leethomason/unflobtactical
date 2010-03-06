#include "screenport.h"
#include "platformgl.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glutil.h"

using namespace grinliz;

void Screenport::PushUI() const
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();					// save projection
	glLoadIdentity();				// projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();					// model
	glLoadIdentity();				// model

	glRotatef( 90.0f * (float)Rotation(), 0.0f, 0.0f, 1.0f );

#ifdef USING_ES
	glOrthof( 0.f, (float)ViewWidth(), 0.f, (float)ViewHeight(), -100.f, 100.f );
#else
	glOrtho( 0, UIWidth(), 0, UIHeight(), -100, 100 );
#endif
}

void Screenport::PopUI() const
{

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();					// model
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();					// projection
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

