#include "gamedbwriter.h"
#include "../grinliz/glstringutil.h"
#include "../zlib/zlib.h"
#include "gamedb.h"

#include <algorithm>


#pragma warning ( disable : 4996 )
using namespace gamedb;


void WriteU32( FILE* fp, U32 val )
{
	fwrite( &val, sizeof(U32), 1, fp );
}


Writer::Writer()
{
	root = new WItem( "root" );
}

Writer::~Writer()
{
	delete root;

}


void Writer::Save( const char* filename )
{
	FILE* fp = fopen( filename, "wb" );
	GLASSERT( fp );
	if ( !fp ) {
		return;
	}

	// Get all the strings used for anything to build a string pool.
	std::set< std::string > stringSet;
	root->EnumerateStrings( &stringSet );

	// Sort them.
	std::vector< std::string > pool;
	for( std::set< std::string >::iterator itr = stringSet.begin(); itr != stringSet.end(); ++itr ) {
		pool.push_back( *itr );
	}
	std::sort( pool.begin(), pool.end() );

	// --- Header --- //
	HeaderStruct headerStruct;	// come back later and fill in.
	fwrite( &headerStruct, sizeof(HeaderStruct), 1, fp );

	// --- String Pool --- //
	headerStruct.nString = pool.size();
	U32 mark = ftell( fp ) + 4*pool.size();		// position of first string.
	for( unsigned i=0; i<pool.size(); ++i ) {
		WriteU32( fp, mark );
		mark += pool[i].size() + 1;				// size of string and null terminator.
	}
	for( unsigned i=0; i<pool.size(); ++i ) {
		fwrite( pool[i].c_str(), pool[i].size()+1, 1, fp );
	}
	// Move to a 32-bit boundary.
	while( ftell(fp)%4 ) {
		fputc( 0, fp );
	}

	// --- Items --- //
	headerStruct.offsetToItems = ftell( fp );
	std::vector< WItem::MemSize > dataPool;
	std::vector< U8 > buffer;

	root->Save( fp, pool, &dataPool );

	// --- Data Description --- //
	headerStruct.offsetToDataDesc = ftell( fp );

	// reserve space for offsets
	std::vector< DataDescStruct > ddsVec;
	ddsVec.resize( dataPool.size() );
	fwrite( &ddsVec[0], sizeof(DataDescStruct)*ddsVec.size(), 1, fp );

	// --- Data --- //
	headerStruct.offsetToData = ftell( fp );
	headerStruct.nData = dataPool.size();

	for( unsigned i=0; i<dataPool.size(); ++i ) {
		WItem::MemSize* m = &dataPool[i];

		ddsVec[i].offset = ftell( fp );
		ddsVec[i].size = m->size;

		int compressed = 0;
		uLongf compressedSize = 0;

		if ( m->size > 20 ) {
			buffer.resize( m->size*8 / 10 );
			compressedSize = buffer.size();

			int result = compress( (Bytef*)&buffer[0], &compressedSize, (const Bytef*)m->mem, m->size );
			if ( result == Z_OK && compressedSize < (uLongf)m->size )
				compressed = 1;
		}

		if ( compressed ) {
			fwrite( &buffer[0], compressedSize, 1, fp );
			ddsVec[i].compressedSize = compressedSize;
		}
		else {
			fwrite( m->mem, m->size, 1, fp );
			ddsVec[i].compressedSize = m->size;
		}
	}
	unsigned totalSize = ftell( fp );

	// --- Data Description, revisited --- //
	fseek( fp, headerStruct.offsetToDataDesc, SEEK_SET );
	fwrite( &ddsVec[0], sizeof(DataDescStruct)*ddsVec.size(), 1, fp );

	// Go back and patch header:
	fseek( fp, 0, SEEK_SET );
	fwrite( &headerStruct, sizeof(headerStruct), 1, fp );
	fclose( fp );

	GLOUTPUT(( "Database write complete. size=%dk stringPool=%dk tree=%dk data=%dk\n",
			   totalSize / 1024,
			   headerStruct.offsetToItems / 1024,
			   ( headerStruct.offsetToData - headerStruct.offsetToItems ) / 1024,
			   ( totalSize - headerStruct.offsetToData ) / 1024 ));
}


WItem::WItem( const char* name )
{
	GLASSERT( name && *name );
	itemName = name;
}


WItem::~WItem()
{
	{
		std::map<std::string, WItem*>::iterator itr;
		for(itr = child.begin(); itr != child.end(); ++itr) {
			delete (*itr).second;
		}
	}
	{
		// Free the copy of the memory.
		for( std::map<std::string, Attrib>::iterator itr = data.begin(); itr != data.end(); ++itr) {
			(*itr).second.Free();
		}
	}
	// Everything else cleaned by destructors.
}


void WItem::EnumerateStrings( std::set< std::string >* stringSet )
{
	(*stringSet).insert( itemName );

	for( std::map<std::string, Attrib>::iterator itr = data.begin(); itr != data.end(); ++itr) {
		(*stringSet).insert( (*itr).first );
		if ( (*itr).second.type == ATTRIBUTE_STRING ) {
			(*stringSet).insert( (*itr).second.stringVal );	// strings are both key and value!!
		}
	}
	for( std::map<std::string, WItem*>::iterator itr = child.begin(); itr != child.end(); ++itr) {
		(*itr).second->EnumerateStrings( stringSet );
	}
}


WItem* WItem::GetChild( const char* name )
{
	GLASSERT( name && *name );
	std::map<std::string, WItem*>::iterator itr = child.find( name );
	if ( itr == child.end() )
		return CreateChild( name );
	return (*itr).second;
}


WItem* WItem::CreateChild( const char* name )
{
	GLASSERT( name && *name );
	GLASSERT( child.find( std::string( name ) ) == child.end() );
	
	WItem* witem = new WItem( name );
	child[ name ] = witem;
	return witem;
}


WItem* WItem::CreateChild( int id )
{
	char buffer[32];
	grinliz::SNPrintf( buffer, 32, "%d", id );
	return CreateChild( buffer );
}


void WItem::Attrib::Free()
{
	if ( type == ATTRIBUTE_DATA ) {
		free( data );
	}
	Clear();
}


void WItem::SetData( const char* name, const void* d, int nData )
{
	GLASSERT( name && *name );
	GLASSERT( d && nData );

	void* mem = malloc( nData );
	memcpy( mem, d, nData );

	Attrib a; a.Clear();
	a.type = ATTRIBUTE_DATA;
	a.data = mem;
	a.dataSize = nData;

	data[ name ] = a;
}


void WItem::SetInt( const char* name, int value )
{
	GLASSERT( name && *name );

	Attrib a; a.Clear();
	a.type = ATTRIBUTE_INT;
	a.intVal = value;

	data[ name ] = a;
}


void WItem::SetFloat( const char* name, float value )
{
	GLASSERT( name && *name );

	Attrib a; a.Clear();
	a.type = ATTRIBUTE_FLOAT;
	a.floatVal = value;

	data[ name ] = a;
}


void WItem::SetString( const char* name, const char* str )
{
	GLASSERT( name && *name );

	Attrib a; a.Clear();
	a.type = ATTRIBUTE_STRING;
	a.stringVal = str;

	data[ name ] = a;
}


void WItem::SetBool( const char* name, bool value )
{
	GLASSERT( name && *name );
	
	Attrib a; a.Clear();
	a.type = ATTRIBUTE_BOOL;
	a.intVal = value ? 1 : 0;

	data[ name ] = a;
}


int WItem::FindString( const std::string& str, const std::vector< std::string >& stringPool )
{
	unsigned low = 0;
	unsigned high = stringPool.size();

    while (low < high) {
		unsigned mid = low + (high - low) / 2;
		if ( stringPool[mid] < str )
			low = mid + 1; 
		else
			high = mid; 
	}
    if ((low < stringPool.size() ) && ( stringPool[low] == str))
		return low;

	GLASSERT( 0 );	// should always succeed!
	return 0;
}


void WItem::Save(	FILE* fp, 
					const std::vector< std::string >& stringPool, 
					std::vector< MemSize >* dataPool )
{
	ItemStruct itemStruct;
	itemStruct.nameID = FindString( itemName, stringPool );
	itemStruct.nAttrib = data.size();
	itemStruct.nChildren = child.size();

	fwrite( &itemStruct, sizeof(itemStruct), 1, fp );

	// Reserve the child locations.
	int markChildren = ftell( fp );
	for( unsigned i=0; i<child.size(); ++i )
		WriteU32( fp, 0 );
	
	// And now write the attributes:
	AttribStruct aStruct;

	for( std::map<std::string, Attrib>::iterator itr = data.begin(); itr != data.end(); ++itr) {
		
		Attrib* a = &(itr->second);
		aStruct.SetKeyType( FindString( itr->first, stringPool ), a->type );

		switch ( a->type ) {
			case ATTRIBUTE_DATA:
				{
					int id = dataPool->size();
					MemSize m = { a->data, a->dataSize };
					dataPool->push_back( m );
					aStruct.dataID = id;
				}
				break;

			case ATTRIBUTE_INT:
				aStruct.intVal = a->intVal;
				break;

			case ATTRIBUTE_FLOAT:
				aStruct.floatVal = a->floatVal;
				break;

			case ATTRIBUTE_STRING:
				aStruct.stringID = FindString( a->stringVal, stringPool );
				break;

			case ATTRIBUTE_BOOL:
				aStruct.intVal = a->intVal;
				break;

			default:
				GLASSERT( 0 );
		}
		fwrite( &aStruct, sizeof(aStruct), 1, fp );
	}

	// Save the children
	std::map<std::string, WItem*>::iterator itr;
	int n = 0;

	for(itr = child.begin(); itr != child.end(); ++itr, ++n ) {
		// Back up, write the current position to the child offset.
		int current = ftell(fp);
		fseek( fp, markChildren+n*4, SEEK_SET );
		WriteU32( fp, current );
		fseek( fp, current, SEEK_SET );

		// save
		itr->second->Save( fp, stringPool, dataPool );
	}
}
