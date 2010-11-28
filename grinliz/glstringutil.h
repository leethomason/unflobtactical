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

#include <stdlib.h>
#include <string.h>

#include "gldebug.h"
#include "gltypes.h"


namespace grinliz 
{
	
inline int HexLowerCharToInt( int c ) {
	GLASSERT( (c>='0'&&c<='9') || (c>='a'&&c<='f') );
	if ( c <= '9' )
		return c - '0';
	else
		return 10 + c - 'a';

	//const int bit6 = (c&0x40) >> 6;
	//return bit6*( c - 'a' + 10 ) + (1-bit6)*( c - '0');
}

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


/*
	A class that wraps a c-array of characters.
*/
template< int ALLOCATE >
class CStr
{
public:
	CStr()							{	GLASSERT(sizeof(*this) == ALLOCATE );		// not required for class to work, but certainly the intended design
										buf[0] = 0; 
										Validate();
									}
	CStr( const char* src )			{	GLASSERT(sizeof(*this) == ALLOCATE );
										GLASSERT( strlen( src ) < (ALLOCATE-1));
										*buf = 0;
										if ( src ) 
											StrNCpy( buf, src, ALLOCATE ); 
										Validate();
									}
	CStr( int value )				{	GLASSERT(sizeof(*this) == ALLOCATE );		// not required for class to work, but certainly the intended design
										buf[0] = 0; 
										SNPrintf( buf, ALLOCATE, "%d", value );
										Validate();
									}
	~CStr()	{}

	const char* c_str()	const			{ return buf; }
	int size() const					{ return strlen( buf ); }
	bool empty() const					{ return buf[0] == 0; }

	int Length() const 					{ return strlen( buf ); }
	void ClearBuf()						{ memset( buf, 0, ALLOCATE ); }
	void Clear()						{ buf[0] = 0; }

	bool operator==( const char* str ) const						{ return buf && str && strcmp( buf, str ) == 0; }
	char operator[]( int i ) const									{ GLASSERT( i>=0 && i<ALLOCATE-1 ); return buf[i]; }
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
		Validate();
	}
	void operator=( int value ) {
		SNPrintf( buf, ALLOCATE, "%d", value );
		Validate();
	}

	void operator+=( const char* src ) {
		GLASSERT( src );
		if ( src ) {
			int len = size();
			if ( len < ALLOCATE-1 )
				StrNCpy( buf+len, src, ALLOCATE-len );
		}
		Validate();
	}


private:
#ifdef DEBUG
	void Validate() {
		GLASSERT( strlen(buf) < ALLOCATE );	// strictly less - need space for null terminator
	}
#else
	void Validate() {}
#endif
	char buf[ALLOCATE];
};


/*
	Simple replace for std::string, to remove stdlib dependencies.
*/
class GLString
{
public:
	GLString() : m_buf( 0 ), m_allocated( 0 ), m_size( 0 )							{}
	GLString( const GLString& rhs ) : m_buf( 0 ), m_allocated( 0 ), m_size( 0 )		{ init( rhs ); }
	GLString( const char* rhs ) : m_buf( 0 ), m_allocated( 0 ), m_size( 0 )			{ init( rhs ); }
	~GLString()																		{ delete [] m_buf; }

	void operator=( const GLString& rhs )		{ init( rhs ); }
	void operator=( const char* rhs )			{ init( rhs ); }

	void operator+= ( const GLString& rhs )		{ append( rhs ); }
	void operator+= ( const char* rhs )			{ append( rhs ); }
	void operator+= ( char c )					{ char s[2] = { c, 0 }; append( s ); }

	bool operator==( const GLString& rhs ) const	{ return compare( rhs.c_str() ) == 0; }
	bool operator==( const char* rhs ) const		{ return compare( rhs ) == 0; }
	bool operator!=( const GLString& rhs ) const	{ return compare( rhs.c_str() ) != 0; }
	bool operator!=( const char* rhs ) const		{ return compare( rhs ) != 0; }

	char operator[]( unsigned i ) const				{ GLASSERT( i < size() );
													  return m_buf[i];
													}

	unsigned find( char c )	const					{	if ( m_buf ) {
															const char* p = strchr( m_buf, c );
															return ( p ) ? (p-m_buf) : size();
														}
														return 0;
													}
	unsigned rfind( char c )	const				{	if ( m_buf ) {
															const char* p = strrchr( m_buf, c );
															return ( p ) ? (p-m_buf) : size();
														}
														return 0;
													}
	GLString substr( unsigned pos, unsigned n );

	void append( const GLString& str );
	void append( const char* );
	void append( const char* p, int n );
	int compare( const char* str ) const			{ return strcmp( m_buf, str ); }

	unsigned size() const							{ return m_size; }
	const char* c_str() const						{ return m_buf; }

private:
	void ensureSize( unsigned s );
	void init( const GLString& str );
	void init( const char* );
#ifdef DEBUG	
	void validate();
#else
	void validate()	{}
#endif



	char*		m_buf;
	unsigned	m_allocated;
	unsigned	m_size;
};


/** Loads a text file to the given string. Returns true on success, false
    if the file could not be found.
*/
//bool LoadTextFile( const char* filename, std::string* str );

void StrSplitFilename(	const GLString& fullPath, 
						GLString* basePath,
						GLString* name,
						GLString* extension );

void StrFillBuffer( const GLString& str, char* buffer, int bufferSize );
void StrFillBuffer( const char* str, char* buffer, int bufferSize );


struct StrToken {
	enum {
		UNKNOWN,
		STRING,
		NUMBER
	};

	int type;
	GLString str;
	double number;

	StrToken() : type( 0 ), number( 0.0 ) {}

	void InitString( const GLString& str ) {
		type = STRING;
		this->str = str;
	}
	void InitNumber( double num ) {
		type = NUMBER;
		this->number = num;
	}
};

void StrTokenize( const GLString& in, int maxTokens, StrToken *tokens, int* nTokens );

};	// namespace grinliz


#endif
