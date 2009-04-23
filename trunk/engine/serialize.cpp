#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serialize.h"

void TextureHeader::Load( FILE* fp )
{
	fread( this, sizeof(TextureHeader), 1, fp );
}

void ModelHeader::Load( FILE* fp )
{
	fread( this, sizeof(ModelHeader), 1, fp );
}


void ModelGroup::Load( FILE* fp )
{
	fread( this, sizeof(ModelGroup), 1, fp );
}


UFOStream::UFOStream()
{
	cap = 16;
	buffer = (U8*) malloc( cap );
	ptr = buffer;
	size = 0;
}


UFOStream::~UFOStream()
{
	free( buffer );
}


void UFOStream::Ensure( int newCap ) 
{
	if ( newCap > cap ) {
		int delta = ptr - buffer;

		newCap = grinliz::CeilPowerOf2( newCap );
		buffer = (U8*)realloc( buffer, newCap );
		GLOUTPUT(( "newcap=%d cap=%d\n", newCap, cap ));
		
		cap = newCap;
		ptr = buffer + delta;
	}
}


const char*	UFOStream::ReadStr()
{
	int len = strlen( (const char*)ptr );
	const char* r = (const char*)ptr;
	ptr += (len + 1);
	GLASSERT( ptr < EndSize() );
	return r;
}

U8 UFOStream::ReadU8()			{ U8 v = 0; Read( &v ); return v; }
U16 UFOStream::ReadU16()		{ U16 v = 0 ; Read( &v ); return v; }
U32 UFOStream::ReadU32()		{ U32 v = 0; Read( &v ); return v; }
float UFOStream::ReadFloat()	{ float v = 0.0f; Read( &v ); return v; }


void UFOStream::WriteStr( const char* str )
{
	int len = strlen( str );
	Ensure( Size() + len + 1);
	strcpy( (char*)ptr, str );
	ptr += (len+1);
	if ( Tell() > size ) {
		size = Tell();
	}
}

void UFOStream::WriteU8( U32 value )			{ GLASSERT( value < 256 ); Write( (U8)value ); }
void UFOStream::WriteU16( U32 value )			{ GLASSERT( value < 0x10000 ); Write( (U16) value ); }
void UFOStream::WriteU32( U32 value )			{ Write( value ); }
void UFOStream::WriteFloat( float value )		{ Write( value ); }

