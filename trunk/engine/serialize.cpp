/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "serialize.h"
#include "../sqlite3/sqlite3.h"
#include "../shared/gldatabase.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

void ModelHeader::Load( sqlite3* db, const char* n, int *groupStart, int *vertexID, int *indexID )
{
	StrNCpy( name, n, EL_FILE_STRING_LEN );

	sqlite3_stmt* stmt = 0;
	sqlite3_prepare_v2(db, "SELECT * FROM model WHERE name=?;", -1, &stmt, 0 );
	sqlite3_bind_text( stmt, 1, n, -1, 0 );

	if (sqlite3_step(stmt) == SQLITE_ROW) 
	{
		// 0: name
		int binaryID =		sqlite3_column_int(  stmt, 1 );

		// Read the binary data for this object before filling out local fields.
		int size=0;
		BinaryDBReader reader( db );
		reader.ReadSize( binaryID, &size );
		GLASSERT( size == sizeof( ModelHeader ) );
		reader.ReadData( binaryID, size, this );

		*groupStart =	sqlite3_column_int( stmt, 2 );
		nGroups =		sqlite3_column_int( stmt, 3 );
		*vertexID =		sqlite3_column_int( stmt, 4 );
		*indexID =		sqlite3_column_int( stmt, 5 );
	}
	else {
		GLASSERT( 0 );
	}
	sqlite3_finalize(stmt);
}


void ModelGroup::Load( sqlite3* db, int id )
{
	sqlite3_stmt* stmt = 0;
	sqlite3_prepare_v2(db, "SELECT * FROM modelGroup WHERE id=?;", -1, &stmt, 0 );
	sqlite3_bind_int( stmt, 1, id );

	if (sqlite3_step(stmt) == SQLITE_ROW) 
	{
		// 0: id
		textureName[0] = 0;
		StrNCpy( textureName, (const char*)sqlite3_column_text( stmt, 1 ), EL_FILE_STRING_LEN );		// name
		nVertex = sqlite3_column_int(  stmt, 2 );
		nIndex  = sqlite3_column_int(  stmt, 3 );
	}
	else {
		GLASSERT( 0 );
	}
	sqlite3_finalize(stmt);	
}
