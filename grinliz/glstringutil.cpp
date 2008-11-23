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
	FILE* fp = fopen( filename, "r" );
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
	*base = "";
	*name = "";
	*extension = "";

	unsigned dotPos = path.rfind( '.' );
	if ( dotPos < path.size() ) {
		*extension = path.substr( dotPos, path.size() );
		path = path.substr( 0, dotPos );
	}

	unsigned slashPos = path.rfind( '/' );
	unsigned backPos = path.rfind( '\\' );
	unsigned colonPos = path.rfind( ':' );

	unsigned pos = 0;
	if ( slashPos < path.size() && slashPos > pos ) {
		pos = slashPos;
	}
	if ( backPos < path.size() && backPos > pos ) {
		pos = backPos;
	}
	if ( colonPos < path.size() && colonPos > pos ) {
		pos = colonPos;
	}
	if ( pos < path.size() ) {
		*base = path.substr( 0, pos+1 );
		path = path.substr( pos+1, path.size() );
	}
	*name = path;
}


void grinliz::StrFillBuffer( const std::string& str, char* buffer, int bufferSize )
{
	strncpy( buffer, str.c_str(), bufferSize );

	if ( bufferSize - 1 - (int)str.size() > 0 ) {
		memset( buffer + str.size() + 1, 0, bufferSize - str.size() - 1 );
	}
	GLASSERT( buffer[bufferSize-1] == 0 );
}


void grinliz::StrFillBuffer( const char* str, char* buffer, int bufferSize )
{
	strncpy( buffer, str, bufferSize );

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

