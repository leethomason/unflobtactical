#ifndef UFO_ATTACK_DITHER_INCLUDED
#define UFO_ATTACK_DITHER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "SDL.h"


enum {		// channels	bytes
	RGBA16,	// 4444		2
	RGB16,	// 565		2
	ALPHA,	// 8		1
};


void DitherTo16( const SDL_Surface* inSurface, int format, bool invert, U16* target );


#endif  // UFO_ATTACK_DITHER_INCLUDED