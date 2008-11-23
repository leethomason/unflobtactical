#include "platformgl.h"
#include "map.h"

void Map::Draw()
{
	const float dx = (float)Map::SIZE;
	const float dy = (float)Map::SIZE;

	float v[12] = {	
					0.f, 0.f, dy,	  
					dx,  0.f, dy,
					dx,  0.f, 0.f, 
					0.f, 0.f, 0.f 
				  };

	float t[8] = {	
					0.f, 0.f, 
					dx,  0.f,
					dx,	 dy,
					0.f, dy
				 };
	float n[12] = {	
					0.f, 1.f, 0.f,
					0.f, 1.f, 0.f,
					0.f, 1.f, 0.f,
					0.f, 1.f, 0.f
				  };

	glColor4f( 1.f, 1.f, 1.f, 1.f );

	glVertexPointer( 3, GL_FLOAT, 0, v );
	glTexCoordPointer( 2, GL_FLOAT, 0, t );
	glNormalPointer( GL_FLOAT, 0, n );

	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
}

