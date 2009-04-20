#ifndef UFOATTACK_SERIALIZE_INCLUDED
#define UFOATTACK_SERIALIZE_INCLUDED

/*	WARNING everything assumes little endian. Nead to rework
	save/load if this changes.
*/

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glvector.h"
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
// Associated with a model in a stream. Specifically, a model
// that can be used on the path.
struct ItemResource
{
	enum {
		STREAM_SIZE			= 0x01,		// if the size isn't 1x1
		ORIGIN_UPPER_LEFT	= 0x02,
		STREAM_HP			= 0x04,
		STREAM_FLAMMABLE	= 0x08,
		STREAM_EXPLOSIVE	= 0x10,
		STREAM_PATHER_0		= 0x20,
		STREAM_PATHER_1		= 0x40
	};
	U8 cx;
	U8 cz;
	int hp;	// 0 is destroyed,
};
*/

/*
	ModelHeader
	ModelGroups	groups[]
	Vertex		vertexData[]	// all groups
	U16			indexData[]		// all groups
*/
struct ModelHeader
{
	// flags
	enum {
		BILLBOARD	= 0x01,
		UPPER_LEFT	= 0x02,
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


class UFOStream
{
public:
	enum {
		MAGIC0 = 0x01234567,
		MAGIC1 = 0xfedcba98,
	};

	UFOStream();
	~UFOStream();

	void SeekSet( int offset )	{ GLASSERT( offset < size ); if ( offset < size ) ptr = buffer + offset; }
	int Size()					{ return size; }
	int Tell()					{ GLASSERT( ptr ); GLASSERT( buffer ); return ptr - buffer; }
	void Dump( const char* name ) {
		GLOUTPUT(( "Stream '%s' size=%d cap=%d\n", name, size, cap ));
	}

	template< typename T > void Read( T* value ) 
	{
		GLASSERT( ptr + sizeof(T) <= EndSize() );
		if ( ptr + sizeof(T) <= EndSize() ) {
			*value = *((T*)ptr);
			ptr += sizeof( T );
		}
	}
	U8		ReadU8();
	U16		ReadU16();
	U32		ReadU32();	
	float	ReadFloat();

	const char*	ReadStr();

	template< typename T > void Write( const T& value ) 
	{
		Ensure( size + sizeof(value) );
		GLASSERT( ptr + sizeof(T) <= buffer + cap );
		
		T* p = (T*)ptr;
		*p = value;
		ptr += sizeof( T );

		if ( Tell() > size ) {
			size = Tell();
		}
	}

	void WriteU8( U32 value );
	void WriteU16( U32 value );
	void WriteU32( U32 value );
	void WriteFloat( float value );

	void WriteStr( const char* str );

private:
	void Ensure( int newCap );

	U8* buffer;
	U8* ptr;
	int cap;
	int size;
	const U8*	EndCap()	{ GLASSERT( buffer ); return buffer + cap; }
	const U8*	EndSize()	{ GLASSERT( buffer ); return buffer + size; }
};

#endif // UFOATTACK_SERIALIZE_INCLUDED