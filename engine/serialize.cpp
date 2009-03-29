#include <stdio.h>
#include <stdlib.h>
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


Stream::Stream()
{
	cap = 16;
	buffer = (U8*) malloc( cap );
	ptr = buffer;
	size = 0;
}


Stream::~Stream()
{
	free( buffer );
}


void Stream::Ensure( int newCap ) 
{
	if ( newCap > cap ) {
		newCap = grinliz::CeilPowerOf2( newCap );
		buffer = (U8*)realloc( buffer, newCap );
		cap = newCap;
	}
}


const char*	Stream::ReadStr()
{
	int len = strlen( (const char*)ptr );
	const char* r = (const char*)ptr;
	ptr += (len + 1);
	GLASSERT( ptr < EndSize() );
	return r;
}

U8 Stream::ReadU8()			{ U8 v = 0; Read( &v ); return v; }
U16 Stream::ReadU16()		{ U16 v = 0 ; Read( &v ); return v; }
U32 Stream::ReadU32()		{ U32 v = 0; Read( &v ); return v; }
float Stream::ReadFloat()	{ float v = 0.0f; Read( &v ); return v; }


void Stream::WriteStr( const char* str )
{
	int len = strlen( str );
	Ensure( Size() + len + 1);
	strcpy( (char*)ptr, str );
	ptr += (len+1);
	if ( Tell() > size ) {
		size = Tell();
	}
}

void Stream::WriteU8( U32 value )			{ GLASSERT( value < 256 ); Write( (U8)value ); }
void Stream::WriteU16( U32 value )			{ GLASSERT( value < 0x10000 ); Write( (U16) value ); }
void Stream::WriteU32( U32 value )			{ Write( value ); }
void Stream::WriteFloat( float value )		{ Write( value ); }

