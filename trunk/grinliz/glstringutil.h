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
	if ( a && b ) {
		if ( a == b ) 
			return true;
		else if ( strcmp( a, b ) == 0 )
			return true;
	}
	return false;
}


// Reimplements SAFE strncpy, safely cross-compiler. Always returns a null-terminated string.
// The strncpy definition is terrifying: not guarenteed to be null terminated.
void StrNCpy( char* dst, const char* src, size_t bufferSize );
int SNPrintf(char *str, size_t size, const char *format, ...);


template< int ALLOCATE >
class CStr
{
public:
	CStr()							{	GLASSERT(sizeof(*this) == ALLOCATE );		// not required for class to work, but certainly the intended design
										buf[0] = 0; }
	CStr( const char* src )			{	GLASSERT(sizeof(*this) == ALLOCATE );
										GLASSERT( strlen( src ) < (ALLOCATE-1));
										*buf = 0;
										if ( src ) 
											StrNCpy( buf, src, ALLOCATE ); 
									}
	CStr( int value )				{	GLASSERT(sizeof(*this) == ALLOCATE );		// not required for class to work, but certainly the intended design
										buf[0] = 0; 
										SNPrintf( buf, ALLOCATE, "%d", value );
									}
	~CStr()	{}

	const char* c_str()	const			{ return buf; }
	int size() const					{ return strlen( buf ); }
	bool empty() const					{ return buf[0] == 0; }

	int Length() const 					{ return strlen( buf ); }
	void ClearBuf()						{ memset( buf, 0, ALLOCATE ); }
	void Clear()						{ buf[0] = 0; }

	bool operator==( const char* str ) const						{ return buf && str && strcmp( buf, str ) == 0; }
	template < class T > bool operator==( const T& str ) const		{ return buf && strcmp( buf, str.c_str() ) == 0; }

	void operator=( const char* src )	{	
		GLASSERT( src );
		if (src) {
			GLASSERT( strlen( src ) < (ALLOCATE-1) );
			StrNCpy( buf, src, ALLOCATE ); 
		}
		else {
			buf[0] = 0;
		}
	}
	void operator=( int value ) {
		SNPrintf( buf, ALLOCATE, "%d", value );
	}

	void operator+=( const char* src ) {
		GLASSERT( src );
		if ( src ) {
			int len = size();
			if ( len < ALLOCATE-1 )
				StrNCpy( buf+len, src, ALLOCATE-len );
		}
	}


private:
	char buf[ALLOCATE];
};


class CStrRef
{
public:
	CStrRef() : buf( 0 ), allocated( 0 )	{}
	void Attach( char* mem, int memLen )	{ buf = mem; allocated = memLen; GLASSERT( buf && allocated ); }
	~CStrRef()	{}

	const char* c_str()	const			{ GLASSERT( buf && allocated ); return buf; }
	int size() const					{ GLASSERT( buf && allocated ); return (buf) ? strlen( buf ) : 0; }
	bool empty() const					{ GLASSERT( buf && allocated ); return (buf) ? buf[0] == 0 : true; }

	int Length() const 					{ GLASSERT( buf && allocated ); return (buf) ? strlen( buf ) : 0; }
	void Clear()						{ GLASSERT( buf && allocated ); if ( buf ) buf[0] = 0; }
	int Allocated() const				{ return allocated; }
	int MaxSize() const					{ return allocated-1; }

	bool operator==( const char* str ) const	{ GLASSERT( buf && allocated ); return buf && str && strcmp( buf, str ) == 0; }

	template < class T > bool operator==( const T& str ) const		{ GLASSERT( buf && allocated ); return buf && strcmp( buf, str.c_str() ) == 0; }

	void operator=( const char* src )	{	
		GLASSERT( buf && allocated ); 
		GLASSERT( src );
		if ( buf && allocated ) {
			if (src) {
				GLASSERT( strlen( src ) < (size_t)(allocated-1) );
				StrNCpy( buf, src, allocated ); 
			}
			else {
				buf[0] = 0;
			}
		}
	}


private:
	char *buf;
	int allocated;
};


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
