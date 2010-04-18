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

/**
	A Write-Item. Used when writing to build a tree to be placed in the database.
*/
class WItem
{
public:
	/// Construct a WItem. The name must be unique amongst its siblings.
	WItem( const char* name );
	~WItem();

	/// Quere the name of this item.
	const char* Name()	{ return itemName.c_str(); }

	/// Create a child item by name.
	WItem* CreateChild( const char* name );
	/** Create a child item by number, not this simple creates the string value of the name.
		CreateChild( 1 ) and CreateChild( "1" ) are exactly the same. 
	*/
	WItem* CreateChild( int id );
	/// Get a Child if it exists, create it if the child does not.
	WItem* FetchChild( const char* name );

	/// Add/Set binary data (or long/unique string). Data is compressed.
	void SetData( const char* name, const void* data, int nData );
	/// Add/Set a integer attribute.
	void SetInt( const char* name, int value );
	/// Add/Set a floating point attribute
	void SetFloat( const char* name, float value );
	/// Add/Set a string attribute. Placed in the string pool.
	void SetString( const char* name, const char* str );
	/// Add/Set a boolean attribute.
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

/** Utility class to create a gamedb database. Memory aggressive; meant for tooling
    not for runtime. Basic use:

	# Construct the class
	# Call Root() to get the top node
	# Add you data by addig WItems to the Root(), and WItems to other WItems
	# When all the WItems are set up, call Save()
	# Delete the class. Deleting the class will delete all the WItems attached to the root.
*/
class Writer
{
public:
	Writer();		///<
	~Writer();

	/// Write the database.
	void Save( const char* filename );

	/** Access the root element to be writted. All application Items
	    are children (or sub-children) of the Root.
	*/
	WItem* Root()		{ return root; }

private:

	WItem* root;
};


};	// gamedb
#endif	// GRINLIZ_GAME_DB_WRITER_INCLUDED