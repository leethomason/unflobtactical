/*
Copyright (c) 2000-2005 Lee Thomason (www.grinninglizard.com)
Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#include "glstringutil.h"


bool grinliz::LoadTextFile( const char* filename, std::string* str )
{
#pragma warning ( push )
#pragma warning ( disable : 4996 )	// fopen is unsafe. For video games (what this library is for) that seems extreme.

	FILE* fp = fopen( filename, "r" );

#pragma warning (pop)

	if ( !fp )
		return false;

	const int LEN = 1024;
	char buffer[ LEN ];

	while( fgets( buffer, LEN, fp ) ) 
	{
		str->append( buffer );
	}
	fclose( fp );
	return true;
}


void grinliz::StrSplitFilename(	const std::string& fullPath, 
								std::string* base,
								std::string* name,
								std::string* extension )
{
	std::string path = fullPath;
	if ( base )
		*base = "";
	if ( name )
		*name = "";
	if ( extension )
		*extension = "";

	unsigned dotPos = path.rfind( '.' );
	if ( dotPos < path.size() ) {
		if ( extension )
			*extension = path.substr( dotPos, path.size() );
		path = path.substr( 0, dotPos );
	}

	unsigned slashPos = path.rfind( '/' );
	unsigned backPos = path.rfind( '\\' );
	unsigned colonPos = path.rfind( ':' );

	unsigned pos = 0;
	bool found = false;
	if ( slashPos < path.size() && slashPos > pos ) {
		pos = slashPos;
		found = true;
	}
	if ( backPos < path.size() && backPos > pos ) {
		pos = backPos;
		found = true;
	}
	if ( colonPos < path.size() && colonPos > pos ) {
		pos = colonPos;
		found = true;
	}
	if ( found && pos < path.size() ) {
		if ( base )
			*base = path.substr( 0, pos+1 );
		path = path.substr( pos+1, path.size() );
	}
	if ( name )
		*name = path;
}


int grinliz::SNPrintf(char *str, size_t size, const char *format, ...)
{
    va_list     va;

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    int result = vsnprintf_s( str, size, _TRUNCATE, format, va );
#else
	// Reading the spec, the size does seem correct. The man pages
	// say it will aways be null terminated (whereas the strcpy is not.)
	// Pretty nervous about the implementation, so force a null after.
    int result = vsnprintf( str, size, format, va );
	str[size-1] = 0;

#endif
    va_end( va );

	return result;
}


void grinliz::StrNCpy( char* dst, const char* src, size_t len )
{
	GLASSERT( len > 0 );
	if ( len > 0 && dst && src ) {
		const char* p = src;
		char* q = dst;
		int n = len-1;

		while( *p && n ) {
			*q++ = *p++;
			--n;
		}
		*q = 0;
		dst[len-1] = 0;
		GLASSERT( strlen( dst ) < len );
	}
}


void grinliz::StrFillBuffer( const std::string& str, char* buffer, int bufferSize )
{
	StrNCpy( buffer, str.c_str(), bufferSize );

	if ( bufferSize - 1 - (int)str.size() > 0 ) {
		memset( buffer + str.size() + 1, 0, bufferSize - str.size() - 1 );
	}
	GLASSERT( buffer[bufferSize-1] == 0 );
}


void grinliz::StrFillBuffer( const char* str, char* buffer, int bufferSize )
{
	StrNCpy( buffer, str, bufferSize );

	int size = strlen( str );

	if ( bufferSize - 1 - size > 0 ) {
		memset( buffer + size + 1, 0, bufferSize - size - 1 );
	}
	GLASSERT( buffer[bufferSize-1] == 0 );
}

static const char* SkipWhiteSpace( const char* p ) 
{
	while( p && *p && isspace( *p ) ) {
		++p;
	}
	return p;
}

static const char* FindWhiteSpace( const char* p ) 
{
	while( p && *p && !isspace( *p ) ) {
		++p;
	}
	return p;
}

void grinliz::StrTokenize( const std::string& in, std::vector< StrToken > *tokens )
{
	const char* p = in.c_str();
	p = SkipWhiteSpace( p );
	const char* q = FindWhiteSpace( p );

	tokens->clear();

	while ( p && *p && q ) {
		StrToken token;

		std::string str;
		str.assign( p, q-p );

		if ( isalpha( *p ) ) {
			token.InitString( str );
		}
		else if (    isdigit( *p ) 
			      || *p == '-' 
				  || *p == '+' ) {
			token.InitNumber( atof( str.c_str() ) );
		}
		tokens->push_back( token );

		p = SkipWhiteSpace( q );
		q = FindWhiteSpace( p );
	}
}

