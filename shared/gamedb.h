#ifndef GRINLIZ_GAME_DB_SHARED_INCLUDED
#define GRINLIZ_GAME_DB_SHARED_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"

namespace gamedb
{

/*
	File:


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