	/*
 Copyright (c) 2010 Lee Thomason

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#ifndef GRINLIZ_GAME_DB_READER_INCLUDED
#define GRINLIZ_GAME_DB_READER_INCLUDED

#include <stdio.h>
#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "gamedb.h"

namespace gamedb
{
class Reader;


/** Node of the gamedb.
	Most functions can return the requested Item or Attribute be searching on the name - 
	GetInt( "foo" ) - or by searching on the array index, for example GetInt( 2 ). The array
	index form is generally used if you are enumerating the attributes. Generally the name form
	is used when you know what data you want.
*/
class Item
{
public:
	/// Get the name of this node.
	const char* Name() const;

	/// Number of child nodes.
	int NumChildren() const				{ return ((const ItemStruct*)this)->nChildren; }
	/// Return the child item.
	const Item* Child( int i ) const;
	/// Return the child item.
	const Item* Child( const char* name ) const;

	/// Number of attributes in this Item
	int NumAttributes() const;

	/// Return the name of the attribute. 0 if out of range.
	const char* AttributeName( int i ) const;
	/// Return the index of the attribute. -1 if not found.
	int AttributeIndex( const char* name ) const;

	/// Return true if the attribute exists.
	bool HasAttribute( const char* n ) const	{ return AttributeType( n ) != ATTRIBUTE_INVALID; }
	/// Return the type of the attribute (ATTRIBUTE_DATA, etc.)
	int AttributeType( int i ) const; 
	/// Return the type of the attribute (ATTRIBUTE_DATA, etc.)
	int AttributeType( const char* ) const; 

	int			GetDataSize( int i ) const;			///< Returns size of an ATTRIBUTE_DATA, -1 or failure.
	int			GetDataSize( const char* ) const;	///< Returns size of an ATTRIBUTE_DATA, -1 or failure.

	/** Returns an offset to the raw (possibly compressed) data. NOTE, this is
		past the end of the memory loaded, so this is an offset and size into
		the file.
	*/
	void		GetDataInfo( int i, int* offset, int* size, bool* compressed ) const;		
	/** Returns an offset to the raw (possibly compressed) data. NOTE, this is
		past the end of the memory loaded, so this is an offset and size into
		the file.
	*/
	void		GetDataInfo( const char*, int* offset, int* size, bool* compressed ) const;

	/** Copies uncompressed data to 'mem'. 'memSize' must match the value from GetDataSize() */
	void		GetData( int i, void* mem, int memSize ) const;			
	/** Copies uncompressed data to 'mem'. 'memSize' must match the value from GetDataSize() */
	void		GetData( const char*, void* mem, int memSize ) const;
	
	int			GetDataID( int i ) const;
	int			GetDataID( const char* ) const;

	int			GetInt( int i ) const;			///< Returns value of an ATTRIBUTE_INT
	int			GetInt( const char* ) const;	///< Returns value of an ATTRIBUTE_INT

	float		GetFloat( int i ) const;		///< Returns value of an ATTRIBUTE_FLOAT
	float		GetFloat( const char* ) const;	///< Returns value of an ATTRIBUTE_FLOAT

	const char*	GetString( int i ) const;		///< Returns value of an ATTRIBUTE_STRING
	const char*	GetString( const char* ) const;	///< Returns value of an ATTRIBUTE_STRING

	bool		GetBool( int i ) const;			///< Returns value of an ATTRIBUTE_BOOL
	bool		GetBool( const char* ) const;	///< Returns value of an ATTRIBUTE_BOOL

private:
	Item();
	Item( const Item& rhs );
	void operator=( const Item& rhs );

	const AttribStruct* AttributePtr( int i ) const;
};


/**
	Utility class to read a gamedb. 

	Usage:
	# Construct a Reader
	# The Root() object allows access to the top of the Item tree
	# Navigate and query Items
	# At the end of program execution, or when all resources are loaded, delete Reader

	Threading note: currently all Readers should be on the same thread. That could
	be fixed by adding a semaphore to GetContext()
*/
class Reader
{
public:
	/// Construct the reader.
	Reader();
	~Reader();

	/** Initialize the object. This will hold a read-binary FILE* for the lifetime of Reader.
		@return true if filename could be opened and read.
				false if error.
	*/
	bool Init( const char* filename, int offset=0 );

	/// Access the root of the database.
	const Item* Root() const					{ return root; }
	
	static const Reader* GetContext( const Item* item );

	int GetDataSize( int dataID ) const;
	void GetData( int dataID, void* mem, int memSize ) const;

	/** Utility function to access binary data without having to do memory management in the
		host programm. Given an item, and binary attribute, returns a pointer to the uncompressed
		data. The data length is returned, if requested. The data is null terminated so can
		always safely be treated as a string.

		WARNING: Any call to access ATTRIBUTE_DATA anywhere in the tree will invalidate the cache.
		AccessData should be called, and the data consumed or copied. It is very transient.
	*/
	const void* AccessData( const Item* item, const char* name, int* size=0 ) const;

	const void* BaseMem() const					{ return mem; }
	int OffsetFromStart() const					{ return offset; }	///< Offset from the start of the file (passed in)

	const char* GetString( int id ) const;
	int GetStringID( const char* str ) const;

	// Debug dump
	void RecWalk( const Item* item, int depth );

private:
	static Reader* readerRoot;
	Reader* next;

	FILE* fp;

	void* mem;
	const void* endMem;
	int memSize;
	int offset;	// offset to read from file start

	mutable void* buffer;
	mutable int bufferSize;
	mutable void* access;
	mutable int accessSize;

	const Item* root;
};


};	// gamedb
#endif	// GRINLIZ_GAME_DB_READER_INCLUDED
