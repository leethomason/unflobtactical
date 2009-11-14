#include "../grinliz/gldebug.h"
#include "gldatabase.h"
#include "../zlib/zlib.h"

#include <memory.h>


#define CHECK_AND_RETURN				\
	GLASSERT( sqlResult == SQLITE_OK );	\
	if ( sqlResult != SQLITE_OK )		\
		return sqlResult;


BinaryDBWriter::BinaryDBWriter( sqlite3* db, bool compressWrites )
{
	int sqlResult = sqlite3_exec( db, 
								  "CREATE TABLE IF NOT EXISTS binary "
								  "(id INT NOT NULL PRIMARY KEY, size INT, compressed INT, data BLOB);",
								  0, 0, 0 );

	GLASSERT( sqlResult == SQLITE_OK );
	if ( compressWrites ) {
		buffer.resize( 200 );
	}

	idPool = 1;
	this->db = db;
}


int BinaryDBWriter::Write( const void* data, int sizeInBytes, int* index )
{
	int compressed = 0;
	uLongf compressedSize = 0;

	if ( sizeInBytes > 256 && !buffer.empty() ) {
		buffer.resize( (sizeInBytes*8) / 10 );
		compressedSize = buffer.size();

		int result = compress( (Bytef*)&buffer[0], &compressedSize, (const Bytef*)data, sizeInBytes );
		if ( result == Z_OK )
			compressed = 1;
	}

	sqlite3_stmt* stmt = 0;

	int sqlResult =		sqlite3_prepare_v2( db, "INSERT INTO binary VALUES (?,?,?,?);", -1, &stmt, 0 );
	CHECK_AND_RETURN;
	sqlResult =			sqlite3_bind_int( stmt, 1, idPool );
	CHECK_AND_RETURN;
	sqlResult =			sqlite3_bind_int( stmt, 2, sizeInBytes );
	CHECK_AND_RETURN;
	sqlResult =			sqlite3_bind_int( stmt, 3, compressed );
	CHECK_AND_RETURN;

	if ( compressed )
		sqlResult =			sqlite3_bind_blob( stmt, 4, &buffer[0], compressedSize, 0 );
	else
		sqlResult =			sqlite3_bind_blob( stmt, 4, data, sizeInBytes, 0 );

	CHECK_AND_RETURN;

						sqlite3_step( stmt );
	sqlResult =			sqlite3_finalize(stmt);

	*index = idPool;
	++idPool;
	CHECK_AND_RETURN;
	return sqlResult;	
}


BinaryDBReader::BinaryDBReader( sqlite3* db )
{
	this->db = db;
}


int BinaryDBReader::ReadSize( int index, int *sizeInBytes )
{
	sqlite3_stmt* stmt = 0;
	int sqlResult =		sqlite3_prepare_v2(db, "select * from binary where id=?;", -1, &stmt, 0 );
	CHECK_AND_RETURN;

	// 1 based
	sqlResult =			sqlite3_bind_int(stmt, 1, index);

	if (sqlite3_step(stmt) == SQLITE_ROW) 
	{
		// 0 id
		*sizeInBytes = sqlite3_column_int(stmt, 1);		// the size expanded, not the compressed size
	}
	else {
		GLASSERT( 0 );
		return 1;
	}

	sqlResult =		sqlite3_finalize(stmt);
	CHECK_AND_RETURN;
	return sqlResult;
}


int BinaryDBReader::ReadData( int index, int sizeInBytes, void* data )
{
  	sqlite3_stmt* stmt = 0;
	int sqlResult =		sqlite3_prepare_v2(db, "select * from binary where id=?;", -1, &stmt, 0 );

	sqlResult =		sqlite3_bind_int(stmt, 1, index);
	CHECK_AND_RETURN;

	if (sqlite3_step(stmt) == SQLITE_ROW) 
	{
		int s = sqlite3_column_int(stmt, 1);
		GLASSERT( sizeInBytes == s );

		int compressed = sqlite3_column_int(stmt, 2);
		if ( compressed ) {
			uLongf size = sizeInBytes;
			int result = uncompress(	(Bytef*) data, 
										&size, 
										(const Bytef*)sqlite3_column_blob(stmt, 3), 
										sqlite3_column_bytes( stmt, 3 ) );
			GLASSERT( result == Z_OK );
		}
		else {
			memcpy(data, sqlite3_column_blob(stmt, 3), sizeInBytes);
		}
	}
	else {
		GLASSERT( 0 );
		return 1;
	}

	sqlResult =		sqlite3_finalize(stmt);
	CHECK_AND_RETURN;
	return sqlResult;
}