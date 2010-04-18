#ifndef GRINLIZ_GAME_DB_SHARED_INCLUDED
#define GRINLIZ_GAME_DB_SHARED_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"

namespace gamedb
{
/** gamedb Implements a very simple heirarchical database. It is intended to be run in 2 stages:
	1. A write-only stage, in the build tools, where large amount of processing and memory
	   are assumed. Class gamedb::Writer implements the file creation logic.
    2. A read-only stage at runtime. Minimal resources are used to load and store
	   the file. gamedb::Reader implements the run time code. 

    This pattern maps to the toolig of some video games. Generally speaking you should 
	never have Writer and Reader in the same executable.

	The database heirarchical and has no tables. It is organized like XML, with one important
	exception: the order of Items and Attributes are not preserved. They will be sorted
	for fast run-time access.
	
	Example:

	textures
		palette [width=64 height=64 pixels=[] ]
		font [width=128 height=128 pixels=[] ]
			metrics [pixels=[] ]

	Has the structure:
	item
		item [attribute attribute attribute]
		item [attribute atrribute attribute]
			item [attribute]

	All Items and attributes have a name.
	An Item can contain any number of child Items.
	An Item can have any number of Attributes.
	Attributes can be of the following types:
		* ATTRIBUTE_DATA	Is a binary block of data (or block of text). DATA is stored
							compressed in the file and loaded on demand.
		* ATTRIBUTE_INT		A 32 bit integer.
		* ATTRIBUTE_FLOAT	A 32 bit floating point number.
		* ATTRIBUTE_STRING	A string value. An ATTRIBUTE_STRING is stored in the string table,
							so attribute strings should be 1) short and/or 2) repeated. Longer
							strings should be stored as ATTRIBUTE_DATA.
		* ATTRIBUTE_BOOL	A boolean value.
		* ATTRIBUTE_INVALID	An attribute is undefined or NULL.
*/
	
/*
	File Layout

	Header

	// String Pool
	U32 offset[nEntries]		offset to string itself. Strings are sorted.
	U8 strings[]
	(pad32)

	// Items
	[nItems]
		ItemStruct
		U32 childOffset[nChildren]
		AttribStruct[nAttrib];

	DataDescScruct[nData]

	Data
		U8  data[]
*/

	class Item;	
	class WItem;

	/** The types allowed in the attribute list. */
	enum {
		ATTRIBUTE_DATA,
		ATTRIBUTE_INT,
		ATTRIBUTE_FLOAT,
		ATTRIBUTE_STRING,
		ATTRIBUTE_BOOL,
		ATTRIBUTE_INVALID
	};
	struct HeaderStruct	
	{
		U32 offsetToItems;
		U32 offsetToDataDesc;
		U32 offsetToData;		// also how much of the file to read
		U32 nString;			// number of entries in the string pool
		U32 nData;				// number of entries in the data chunk
	};

	struct ItemStruct
	{
		U32 nameID;
		U32 nChildren;
		U32 nAttrib;
	};

	struct AttribStruct
	{
		void SetKeyType( int key, int type ) {
			keyType = (key&0xfff) | (type<<24);
		}

		U32	keyType;		// lower 24 bits in name of the key, high 8 bits is type
		union {
			U32 dataID;
			int intVal;
			float floatVal;
			U32 stringID;
		};

	};

	struct DataDescStruct
	{	
		U32 size;
		U32 compressedSize;
		U32 offset;
	};

};	// gamedb
#endif // GRINLIZ_GAME_DB_SHARED_INCLUDED