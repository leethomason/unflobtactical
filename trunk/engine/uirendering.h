#ifndef UIRENDERING_INCLUDED
#define UIRENDERING_INCLUDED

#include "vertex.h"


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
	grinliz::Vector2< S16 > pos[ICON_COUNT*4];
	grinliz::Vector2F tex[ICON_COUNT*4];
	U16 index[6*ICON_COUNT];
};


#endif // UIRENDERING_INCLUDED