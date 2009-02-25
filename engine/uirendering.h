#ifndef UIRENDERING_INCLUDED
#define UIRENDERING_INCLUDED

#include "vertex.h"

/*
struct IconInfo
{
	int iconID;

	grinliz::Vector2I	pos;	// pixel position
	grinliz::Vector2I	size;

	Vector2X	tMin;	// texture
	Vector2X	tMax;

	void Set( int iconID, int posX, int posY, int sizeX, int sizeY,
			  float tx0, float ty0, float tx1, float ty1 )
	{
		this->iconID = iconID;
		this->pos.x = posX;
		this->pos.y = posY;
		this->size.x = sizeX;
		this->size.y = sizeY;
		this->tMin.x = tx0;
		this->tMin.y = ty0;
		this->tMax.x = tx1;
		this->tMax.y = ty1;
	}
};
*/


class UIWidgets
{
public:
	UIWidgets();
	~UIWidgets()	{}

	enum {
		ICON_CAMERA_HOME,
		ICON_FOG_OF_WAR,
		ICON_COUNT
	};

	int QueryTap( int x, int y );	
	void Draw( int width, int height, int rotation );

private:
	grinliz::Vector2I pos[ICON_COUNT*4];
	grinliz::Vector2F tex[ICON_COUNT*4];
};


// set texture and color before calling
//void UFODrawIcons( const IconInfo* icons, int screenWidth, int screenHeight, int rotation );


#endif // UIRENDERING_INCLUDED