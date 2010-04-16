#ifndef GRINLIZ_GAME_DB_READER_INCLUDED
#define GRINLIZ_GAME_DB_READER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "gamedb.h"

namespace gamedb
{
class Reader;


class Item
{
public:
	const char* Name() const;

	int NumChildren() const				{ return ((const ItemStruct*)this)->nChildren; }
	const Item* Child( int i ) const;
	const Item* Child( const char* name ) const;

	int NumAttributes() const;

	const char* AttributeName( int i ) const;
	// Return the index of the named attribute. -1 if not found.
	int AttributeIndex( const char* name ) const;

	bool HasAttribute( const char* n ) const	{ return AttributeType( n ) != ATTRIBUTE_INVALID; }
	int AttributeType( int i ) const; 
	int AttributeType( const char* ) const; 

	int			GetDataSize( int i ) const;				// returns -1 on failure
	int			GetDataSize( const char* ) const;

	void		GetData( int i, void* mem, int memSize ) const;
	void		GetData( const char*, void* mem, int memSize ) const;
	
	int			GetDataID( int i ) const;
	int			GetDataID( const char* ) const;

	int			GetInt( int i ) const;
	int			GetInt( const char* ) const;

	float		GetFloat( int i ) const;
	float		GetFloat( const char* ) const;

	const char*	GetString( int i ) const;
	const char*	GetString( const char* ) const;

	bool		GetBool( int i ) const;
	bool		GetBool( const char* ) const;

private:
	Item();
	Item( const Item& rhs );
	void operator=( const Item& rhs );

	const AttribStruct* AttributePtr( int i ) const;
};


class Reader
{
public:
	Reader();
	~Reader();

	//bool Init( const void* mem, int size );
	bool Init( const char* fp );

	const Item* Root() const					{ return root; }
	
	static const Reader* GetContext( const Item* item );

	int GetDataSize( int dataID ) const;
	void GetData( int dataID, void* mem, int memSize ) const;
	const void* AccessData( const Item* item, const char* name, int* size=0 ) const;

	const void* BaseMem() const					{ return mem; }
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

	mutable void* buffer;
	mutable int bufferSize;
	mutable void* access;
	mutable int accessSize;

	const Item* root;
};


};	// gamedb
#endif	// GRINLIZ_GAME_DB_READER_INCLUDED
