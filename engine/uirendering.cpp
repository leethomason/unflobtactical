#include "uirendering.h"
#include "platformgl.h"
using namespace grinliz;

void UFODrawIcons( const IconInfo* icons, int width, int height, int rotation )
{
	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();					// save projection
	glLoadIdentity();				// projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();					// model
	glLoadIdentity();				// model

	glRotatef( 90.0f * (float)rotation, 0.0f, 0.0f, 1.0f );
	glOrtho( 0, width, 0, height, -1, 1 );

	int v[8];
	Fixed t[8];

	while( icons && icons->size.x > 0 )
	{
		t[0] = icons->tMin.x;			t[1] = icons->tMin.y;
		t[2] = icons->tMax.x;			t[3] = icons->tMin.y;
		t[4] = icons->tMax.x;			t[5] = icons->tMax.y;
		t[6] = icons->tMin.x;			t[7] = icons->tMax.y;

		v[0] = icons->pos.x;				v[1] = icons->pos.y;
		v[2] = icons->pos.x+icons->size.x;	v[3] = icons->pos.y;
		v[4] = icons->pos.x+icons->size.x;	v[5] = icons->pos.y+icons->size.y;
		v[6] = icons->pos.x;				v[7] = icons->pos.y+icons->size.y;

		#if TARGET_OS_IPHONE		
		glVertexPointer(   2, GL_INT, 0, v );
		glTexCoordPointer( 2, GL_FIXED, 0, t );  
		#else
		{
			float tF[8];
			for( int j=0; j<8; ++j ) {
				tF[j] = t[j];
			}
			glVertexPointer(   2, GL_INT, 0, v );
			glTexCoordPointer( 2, GL_FLOAT, 0, tF );  
		}
		#endif
		CHECK_GL_ERROR;
		glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
		
		++icons;
	}


	glPopMatrix();					// model
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();					// projection
	glMatrixMode(GL_MODELVIEW);

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
}