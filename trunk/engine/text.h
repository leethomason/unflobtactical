#ifndef UFOATTACK_TEXT_INCLUDED
#define UFOATTACK_TEXT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

const int CONSOLE_WIDTH = 40;
const int CONSOLE_HEIGHT = 15;

void UFOInitDrawText( U32 textTextureID );
void UFODrawText( int x, int y, const char* format, ... );


#endif // UFOATTACK_TEXT_INCLUDED