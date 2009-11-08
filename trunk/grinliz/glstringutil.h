/*
Copyright (c) 2000-2006 Lee Thomason (www.grinninglizard.com)
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



#ifndef GRINLIZ_STRINGUTIL_INCLUDED
#define GRINLIZ_STRINGUTIL_INCLUDED

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <string>
#include <vector>

#include <stdlib.h>

#include "gldebug.h"
#include "gltypes.h"

namespace grinliz 
{
	
inline bool StrEqual( const char* a, const char* b ) 
{
	if ( a && b && strcmp( a, b ) == 0 )
		return true;
	return false;
}


/** Loads a text file to the given string. Returns true on success, false
    if the file could not be found.
*/
bool LoadTextFile( const char* filename, std::string* str );

void StrSplitFilename(	const std::string& fullPath, 
						std::string* basePath,
						std::string* name,
						std::string* extension );

void StrFillBuffer( const std::string& str, char* buffer, int bufferSize );
void StrFillBuffer( const char* str, char* buffer, int bufferSize );

struct StrToken {
	enum {
		UNKNOWN,
		STRING,
		NUMBER
	};

	int type;
	std::string str;
	double number;

	StrToken() : type( 0 ), number( 0.0 ) {}

	void InitString( const std::string& str ) {
		type = STRING;
		this->str = str;
	}
	void InitNumber( double num ) {
		type = NUMBER;
		this->number = num;
	}
};

void StrTokenize( const std::string& in, std::vector< StrToken > *tokens );

};	// namespace grinliz

#endif
