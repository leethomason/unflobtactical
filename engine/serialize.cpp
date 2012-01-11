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
#include "serialize.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

void ModelHeader::Load( const gamedb::Item* item )
{
	name = item->Name();

	const gamedb::Item* header = item->Child( "header" );
	nTotalVertices = header->GetInt( "nTotalVertices" );
	nTotalIndices = header->GetInt( "nTotalIndices" );
	flags = header->GetInt( "flags" );
	nGroups = header->GetInt( "nGroups" );

	bounds.Zero();
	const gamedb::Item* boundsItem = header->Child( "bounds" );
	if ( boundsItem ) {
		bounds.min.x = boundsItem->GetFloat( "min.x" );
		bounds.min.y = boundsItem->GetFloat( "min.y" );
		bounds.min.z = boundsItem->GetFloat( "min.z" );
		bounds.max.x = boundsItem->GetFloat( "max.x" );
		bounds.max.y = boundsItem->GetFloat( "max.y" );
		bounds.max.z = boundsItem->GetFloat( "max.z" );
	}

	trigger.Set( 0, 0, 0 );
	const gamedb::Item* triggerItem = header->Child( "trigger" );
	if ( triggerItem ) {
		trigger.x = triggerItem->GetFloat( "x" );
		trigger.y = triggerItem->GetFloat( "y" );
		trigger.z = triggerItem->GetFloat( "z" );
	}

	eye = 0;
	target = 0;
	const gamedb::Item* extended = header->Child( "extended" );
	if ( extended ) {
		eye = extended->GetFloat( "eye" );
		target = extended->GetFloat( "target" );
	}
}


void ModelGroup::Load( const gamedb::Item* item )
{
	textureName = item->GetString( "textureName" );
	nVertex = item->GetInt( "nVertex" );
	nIndex = item->GetInt( "nIndex" );
}


void XMLUtil::OpenElement( FILE* fp, int depth, const char* value )
{
	Space( fp, depth );
	fprintf( fp, "<%s ", value );	
}


void XMLUtil::SealElement( FILE* fp ) 
{
	fprintf( fp, ">\n" );
}

void XMLUtil::CloseElement( FILE* fp, int depth, const char* value )
{
	Space( fp, depth );
	fprintf( fp, "</%s>\n", value );
}

void XMLUtil::SealCloseElement( FILE* fp )
{
	fprintf( fp, "/>\n" );
}

void XMLUtil::Text( FILE* fp, const char* text )
{
	fprintf( fp, "%s\n", text );
}


void XMLUtil::Attribute( FILE* fp, const char* name, const char* value )
{
	fprintf( fp, "%s=\"%s\" ", name, value );
}

void XMLUtil::Attribute( FILE* fp, const char* name, int value )
{
	fprintf( fp, "%s=\"%d\" ", name, value );
}

void XMLUtil::Attribute( FILE* fp, const char* name, unsigned value )
{
	fprintf( fp, "%s=\"%d\" ", name, value );
}

void XMLUtil::Attribute( FILE* fp, const char* name, float value )
{
	fprintf( fp, "%s=\"%f\" ", name, value );
}

void XMLUtil::Space( FILE* fp, int depth )
{
	static const int LEN = 32;
	static const int SPACES = 4;

	static const char space[LEN+1] = "                                ";

	if ( depth > LEN / SPACES )
		depth = LEN / SPACES;

	fprintf( fp, "%s", &space[LEN] - depth*SPACES );
}