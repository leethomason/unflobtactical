#ifndef UFOATTACK_SERIALIZE_INCLUDED
#define UFOATTACK_SERIALIZE_INCLUDED

/*	WARNING everything assumes little endian. Nead to rework
	save/load if this changes.
*/

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrectangle.h"
#include "enginelimits.h"

struct SDL_RWops;

/*
	The TextureHeader is followed by pixel data. (Based on the format.)
*/
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


struct ModelGroup
{
	char					textureName[EL_FILE_STRING_LEN];
	U16						nVertex;
	U16						nIndex;

	void Set( const char* textureName, int nVertex, int nIndex );
	void Save( SDL_RWops* ops );
	void Load( FILE* fp );
};

/*
	ModelHeader
	ModelGroups	groups[]
	Vertex		vertexData[]	// all groups
	U16			indexData[]		// all groups
*/
struct ModelHeader
{
	enum {
		BILLBOARD = 0x01,
	};

	char					name[EL_FILE_STRING_LEN];
	U16						flags;
	U16						nGroups;
	U16						nTotalVertices;		// in all groups
	U16						nTotalIndices;
	grinliz::Rectangle3F	bounds;

	void Set( const char* name, int nGroups, int nTotalVertices, int nTotalIndices,
			  const grinliz::Rectangle3F& bounds );
	void Save( SDL_RWops* ops );
	void Load( FILE* fp );
};


#endif // UFOATTACK_SERIALIZE_INCLUDED
