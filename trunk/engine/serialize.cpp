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

void ModelHeader::Load( const gamedb::Item* item )
{
	StrNCpy( name, item->Name(), EL_FILE_STRING_LEN );

	// Read the binary data for this object before filling out local fields.
	int size = item->GetDataSize( "header" );
	GLASSERT( size == sizeof( ModelHeader ) );
	item->GetData( "header", this, size );
}


void ModelGroup::Load( const gamedb::Item* item )
{
	StrNCpy( textureName, item->GetString( "textureName" ), EL_FILE_STRING_LEN );
	nVertex = item->GetInt( "nVertex" );
	nIndex = item->GetInt( "nIndex" );
}
