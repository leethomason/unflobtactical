#ifndef GRINLIZ_GAME_DB_WRITER_INCLUDED
#define GRINLIZ_GAME_DB_WRITER_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <set>

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"

namespace gamedb
{


class WItem
{
public:
	WItem( const char* name );
	~WItem();

	const char* Name()	{ return itemName.c_str(); }

	WItem* CreateChild( const char* name );
	WItem* CreateChild( int id );

	WItem* GetChild( const char* name );

	void SetData( const char* name, const void* data, int nData );
	void SetInt( const char* name, int value );
	void SetFloat( const char* name, float value );
	void SetString( const char* name, const char* str );
	void SetBool( const char* name, bool value );


	void EnumerateStrings( std::set< std::string >* stringSet );

	struct MemSize {
		const void* mem;
		int size;
	};
	void Save(	FILE* fp, 
				const std::vector< std::string >& stringPool, 
				std::vector< MemSize >* dataPool );

private:

	struct Attrib
	{
		void Clear()	{ type=0; data=0; dataSize=0; intVal=0; floatVal=0; stringVal.clear(); }
		void Free();

		int type;		// ATTRIBUTE_INT, etc.
		
		void* data;
		int dataSize;

		int intVal;
		float floatVal;
		std::string stringVal;
	};

	int FindString( const std::string& str, const std::vector< std::string >& stringPool );

	std::string itemName;
	std::map<std::string, WItem*> child;

	std::map<std::string, Attrib> data;
};


class Writer
{
public:
	Writer();
	~Writer();

	void Save( const char* filename );

	WItem* Root()		{ return root; }

private:

	WItem* root;
};


};	// gamedb
#endif	// GRINLIZ_GAME_DB_WRITER_INCLUDED