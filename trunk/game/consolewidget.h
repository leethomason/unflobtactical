#ifndef CONSOLE_WIDGET_INCLUDED
#define CONSOLE_WIDGET_INCLUDED
#if 0 
#include "../gamui/gamui.h"

class ConsoleWidget
{
public:
	ConsoleWidget( gamui::Gamui* container );

	void SetOrigin( float x, float y );
	void DoTick( U32 delta );
	void PushMessage( const char* message );
};
#endif
#endif // CONSOLE_WIDGET_INCLUDED