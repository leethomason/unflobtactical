#ifndef UFOATTACK_TEXT_INCLUDED
#define UFOATTACK_TEXT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

void UFOInitDrawText( U32 textTextureID, int screenWidth, int screenHeight, int rotation );

void UFODrawText( int x, int y, const char* format, ... );

#endif // UFOATTACK_TEXT_INCLUDED