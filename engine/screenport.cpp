#include "screenport.h"
#include "platformgl.h"

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
	glOrthof( 0.f, (float)ViewWidth(), 0.f, (float)ViewHeight, -1.f, 1.f );
#else
	glOrtho( 0, ViewWidth(), 0, ViewHeight(), -1, 1 );
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
			*y1 = (physicalWidth-1)-x0;
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
	GLASSERT( x0>=0 && x0<physicalWidth );
	GLASSERT( y0>=0 && y0<physicalHeight );

	*x1 = x0;
	*y1 = (physicalHeight-1)-y0;
}
