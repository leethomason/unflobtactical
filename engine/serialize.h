/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UFOATTACK_SERIALIZE_INCLUDED
#define UFOATTACK_SERIALIZE_INCLUDED

/*	WARNING everything assumes little endian. Nead to rework
	save/load if this changes.
*/

#include <stdio.h>
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glvector.h"
#include "../shared/gamedbreader.h"
#include "enginelimits.h"

struct SDL_RWops;

/*
	The TextureHeader is followed by pixel data. (Based on the format.)
*/
/*
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
*/

struct ModelGroup
{
	char					textureName[EL_FILE_STRING_LEN];
	U16						nVertex;
	U16						nIndex;

	void Set( const char* textureName, int nVertex, int nIndex );
	void Load( const gamedb::Item* item );
};


struct ModelHeader
{
	// flags
	enum {
		BILLBOARD			= 0x01,		// if this is a billboard and always faces the camera
		UPPER_LEFT			= 0x02,		// if the origin is upper left (center is standard)
		ROTATE_SHADOWS		= 0x04,		// for billboards, the shadow turns to face the light
		RESOURCE_NO_SHADOW	= 0x08,		// model casts no shadow
	};

	char					name[EL_FILE_STRING_LEN];	// name must be first - used later in sleazy sort trick in GetModelResource()
	U16						nTotalVertices;		// in all groups
	U16						nTotalIndices;
	U16						flags;
	U16						nGroups;
	grinliz::Rectangle3F	bounds;
	grinliz::Vector3F		trigger;			// location for gun
	float					eye;				// location model "looks from"
	float					target;				// height of chest shot

	void Set( const char* name, int nGroups, int nTotalVertices, int nTotalIndices,
			  const grinliz::Rectangle3F& bounds );

	void Load(	const gamedb::Item* );	// database connection
};

/*
class UFOStream
{
public:
	enum {
		MAGIC0 = 0x01234567,
		MAGIC1 = 0xfedcba98,
	};

	UFOStream( const char* name );
	~UFOStream();

	const char* Name() const	{ return name; }

	void SeekSet( int offset )	{	GLASSERT( offset == 0 || offset < size ); 
									if ( offset < size ) 
										ptr = buffer + offset; 
									bit = 0;
								}
	int Size()					{ return size; }
	const U8* MemBase()			{ return buffer; }

	int Tell()					{ GLASSERT( ptr ); GLASSERT( buffer ); return (int)(ptr - buffer); }
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
	bool	ReadBool()	{ return ReadU8() != 0; }
	U8		ReadU8();
	U16		ReadU16();
	U32		ReadU32();	
	float	ReadFloat();
	const char*	ReadStr();
	void    ReadU32Arary( U32 len, U32* arr );

	void BeginReadBits();
	U32 ReadBits( U32 nBits );
	void EndReadBits();

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

	void WriteBool( bool value )	{ WriteU8( value ? 1 : 0 ); }	// optimize for bit writing in the future?
	void WriteU8( U32 value );
	void WriteU16( U32 value );
	void WriteU32( U32 value );
	void WriteFloat( float value );
	void WriteStr( const char* str );
	void WriteU32Arary( U32 len, const U32* arr );
	U8* WriteMem( int size );

	void BeginWriteBits();
	void WriteBits( U32 nBits, U32 bits );
	void EndWriteBits();

	UFOStream* next;	// used by the stream manager.

private:
	void Ensure( int newCap );

	char name[EL_FILE_STRING_LEN];
	U8* buffer;
	U8* ptr;
	U32 bit;
	int cap;
	int size;
	const U8*	EndCap()	{ GLASSERT( buffer ); return buffer + cap; }
	const U8*	EndSize()	{ GLASSERT( buffer ); return buffer + size; }
};
*/

#endif // UFOATTACK_SERIALIZE_INCLUDED
