#ifndef UFOATTACK_SERIALIZE_INCLUDED
#define UFOATTACK_SERIALIZE_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "enginelimits.h"

struct SDL_RWops;

struct TextureHeader
{
	char		name[EL_FILE_STRING_LEN];	// name of the texture
	U32			format;						// texture format (OpenGL constant)
	U32			type;						// texture type (OpenGL constant)
	U16			width;
	U16			height;

	void Set( const char* name, U32 format, U32 type, U16 width, U16 height );

	void Save( SDL_RWops* ops );
	void Load( FILE* fp );
};

#endif // UFOATTACK_SERIALIZE_INCLUDED
