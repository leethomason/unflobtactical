#ifndef CONSOLE_WIDGET_INCLUDED
#define CONSOLE_WIDGET_INCLUDED

#include "../gamui/gamui.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

class ConsoleWidget
{
public:
	ConsoleWidget( gamui::Gamui* container );

	void SetOrigin( float x, float y );
	void DoTick( U32 delta );
	void PushMessage( const char* message );
private:
	U32 decay;

	enum { MAX_TEXT = 6, DECAY = 2000 };
	gamui::Gamui*	 gamui;
	gamui::TextLabel text[MAX_TEXT];
};
#endif // CONSOLE_WIDGET_INCLUDED