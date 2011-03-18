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
	void SetInfluence( float x );
	void SetTraits( int i );

private:
	gamui::TextLabel	name;
	gamui::DigitalBar	bar;
	gamui::Image		trait[2];
};


#endif // AREA_WIDGET_INCLUDED
