#include "../grinliz/gldebug.h"
#include "gldatabase.h"
#include "../zlib/zlib.h"

#include <memory.h>


#define CHECK_AND_RETURN				\
	GLASSERT( sqlResult == SQLITE_OK );	\
	if ( sqlResult != SQLITE_OK )		\
		return sqlResult;


int DBCreateBinaryTable( sqlite3* db )
{
	int sqlResult = sqlite3_exec( db, 
								  "CREATE TABLE binary "
								  "(id INT NOT NULL PRIMARY KEY, size INT, compressed INT, data BLOB);",
								  0, 0, 0 );

	CHECK_AND_RETURN;
	return sqlResult;
}


int DBWriteBinary( sqlite3* db, const void* data, int sizeInBytes, int* index, 
				   void* compressionBuf, int compressionBufSize )
{
	static int idPool=1;
	int compressed = 0;
	uLongf compressedSize = compressionBufSize;

	if ( compressionBuf ) {
		int result = compress( (Bytef*)compressionBuf, &compressedSize, (const Bytef*)data, sizeInBytes );
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
		sqlResult =			sqlite3_bind_blob( stmt, 4, compressionBuf, compressedSize, SQLITE_TRANSIENT );
	else
		sqlResult =			sqlite3_bind_blob( stmt, 4, data, sizeInBytes, SQLITE_TRANSIENT );

	CHECK_AND_RETURN;

						sqlite3_step( stmt );
	sqlResult =			sqlite3_finalize(stmt);

	*index = idPool;
	++idPool;
	CHECK_AND_RETURN;
	return sqlResult;	
}


int DBReadBinarySize( sqlite3* db, int index, int *sizeInBytes )
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


int DBReadBinaryData( sqlite3* db, int index, int sizeInBytes, void* data )
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