#ifndef UIRENDERING_INCLUDED
#define UIRENDERING_INCLUDED

#include "vertex.h"


struct IconInfo
{
	int iconID;

	grinliz::Vector2I	pos;	// pixel position
	grinliz::Vector2I	size;

	Vector2X	tMin;
	Vector2X	tMax;
};

// set texture and color before calling
void UFODrawIcons( const IconInfo* icons, int screenWidth, int screenHeight, int rotation );


#endif // UIRENDERING_INCLUDED