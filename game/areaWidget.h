#ifndef AREA_WIDGET_INCLUDED
#define AREA_WIDGET_INCLUDED

#include "../gamui/gamui.h"

class Game;


class AreaWidget
{
public:
	AreaWidget( Game* game,
				gamui::Gamui* container,
				const char* name );

	void SetOrigin( float x, float y );

private:
	gamui::TextLabel	name;
	gamui::DigitalBar	bar;
};


#endif // AREA_WIDGET_INCLUDED
