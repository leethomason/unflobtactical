#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
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


UFOStream::UFOStream( const char* _name )
{
	GLASSERT( strlen( _name ) < EL_FILE_STRING_LEN );
	strcpy( name, _name );

	cap = 400;
	buffer = (U8*) malloc( cap );
	ptr = buffer;
	size = 0;
	bit = 0;
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

void UFOStream::BeginReadBits()
{
	bit = 0;
}

void UFOStream::EndReadBits()
{
	if ( bit ) {
		bit = 0;
		++ptr;
	}
}


void UFOStream::ReadU32Arary( U32 len, U32* arr )
{
	U32 bits = ReadU8();
	BeginReadBits();
	for( U32 i=0; i<len; ++i ) {
		arr[i] = ReadBits( bits );
	}
	EndReadBits();
}


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

void UFOStream::WriteU32Arary( U32 len, const U32* arr )
{
	// Scan for size.
	U32 value = 0;
	for( U32 i=0; i<len; ++i ) {
		value |= arr[i];
	}
	U32 size = 1;
	while( ((U32)1<<size) < value )
		++size;

	// Write the data
	WriteU8( size );
	BeginWriteBits();
	for( U32 i=0; i<len; ++i ) {
		WriteBits( size, arr[i] );
	}
	EndWriteBits();
}


void UFOStream::BeginWriteBits()
{
	WriteU8( 0 );
	--ptr;
}



U32 UFOStream::ReadBits( U32 nBits )
{
	const U32 maskArr[8] = { 1, 3, 7, 15, 31, 63, 127, 255 };
	GLASSERT( nBits <= 32 );
	U32 value = 0;
	U32 bitsRemaining = nBits;

	while ( bitsRemaining ) {
		U32 nBitsInChunk = grinliz::Min( bitsRemaining, 8-bit );
		U32 mask = maskArr[nBitsInChunk-1];

		value |= ( ( (*ptr >> bit ) & mask ) << (bitsRemaining-nBitsInChunk) );

		bit				+= nBitsInChunk;
		bitsRemaining	-= nBitsInChunk;

		if ( bit == 8 ) {
			++ptr;
			bit = 0;
		}
	}
	return value;
}


void UFOStream::WriteBits( U32 nBits, U32 value )
{
	// Pack in the high bits first.
	const U32 maskArr[8] = { 1, 3, 7, 15, 31, 63, 127, 255 };

	GLASSERT( nBits <= 32 );
	GLASSERT( value < ((U32)1<<nBits) );
	U32 bitsRemaining = nBits;

	while( bitsRemaining > 0 ) 
	{
		U32 nBitsInChunk = grinliz::Min( bitsRemaining, 8-bit );
		U32 mask = maskArr[nBitsInChunk-1];

		*ptr |= ( ( (value>>(bitsRemaining-nBitsInChunk) ) & mask ) << bit );

		bit				+= nBitsInChunk;
		bitsRemaining	-= nBitsInChunk;

		if ( bit == 8 ) {
			++ptr;
			WriteU8( 0 );
			--ptr;

			bit = 0;
		}
	}
}


void UFOStream::EndWriteBits()
{
	if ( bit ) {
		++ptr;
		bit = 0;
	}
}


void UFOStream::WriteToDisk()
{
	std::string name( Name() );
	name += std::string( ".ufodat" );

	FILE* fp = fopen( name.c_str(), "wb" );
	GLASSERT( fp );
	if ( fp ) {
		fwrite( buffer, 1, size, fp );
		fclose( fp );
	}
}


bool UFOStream::ReadFromDisk( const char* path )
{
	std::string name;

	if ( path ) {
		name = path;
	}
	else {
		name = Name();
		name += std::string( ".ufodat" );
	}

	FILE* fp = fopen( name.c_str(), "rb" );
	if ( fp ) {
		fseek( fp, 0, SEEK_END );
		size_t sz = ftell( fp );
		fseek( fp, 0, SEEK_SET );

		Ensure( sz );
		size = sz;
		fread( buffer, 1, size, fp );
		ptr = buffer;
		bit = 0;

		fwrite( buffer, 1, size, fp );
		fclose( fp );
		return true;
	}
	return false;
}
