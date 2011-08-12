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

#include "research.h"
#include "../grinliz/glstringutil.h"
#include "../engine/serialize.h"
#include "../tinyxml/tinyxml.h"
#include "../grinliz/glstringutil.h"
#include "item.h"

using namespace grinliz;

int Research::SortTasks( const void* _a, const void* _b )
{
	const Task* a = (const Task*)_a;
	const Task* b = (const Task*)_b;

	if ( a->rpRequired < b->rpRequired )
		return -1;
	else if ( a->rpRequired > b->rpRequired )
		return 1;
	return a->name - b->name;	// just to give deterministic sort across machines
}


Research::Research( const gamedb::Reader* _database, const ItemDefArr& itemDefArr, int totalResearchSeconds )
{
	database = _database;
	current = 0;

	const gamedb::Item* researchItem = database->Root()->Child( "tree" )->Child( "research" );
	GLASSERT( researchItem );

	nTasks = researchItem->NumChildren();
	taskArr = new Task[nTasks];

	// Populate all the tasks.
	int itemMax=0;
	int totalRP = 0;
	for( int i=0; i<nTasks; ++i ) {
		const gamedb::Item* item = researchItem->Child( i );
		memset( &taskArr[i], 0, sizeof( Task ) );
		taskArr[i].name = item->Name();
		taskArr[i].rp = 0;
		taskArr[i].rpRequired = item->GetInt( "points" );
		totalRP += taskArr[i].rpRequired;

		int nReq = item->NumChildren();
		int nPrereq = 0;
		int nItem = 0;

		for( int req=0; req<nReq; ++req ) {
			const gamedb::Item* reqItem = item->Child( req );
			if ( reqItem->HasAttribute( "research" ) ) {
				GLASSERT( nPrereq < MAX_PREREQ );
				taskArr[i].prereq[nPrereq++] = reqItem->GetString( "research" );
			}
			else if ( reqItem->HasAttribute( "item" ) ) {
				GLASSERT( nItem < MAX_ITEMS_REQUIRED );
				const char* itemName = reqItem->GetString( "item" );
				GLASSERT( itemDefArr.Query( itemName ) );
				taskArr[i].item[nItem++] = itemName;
				++itemMax;
			}
			else {
				GLASSERT( 0 );
			}
		}
	}

	for( int i=0; i<nTasks; ++i ) {
		taskArr[i].rpRequired = taskArr[i].rpRequired * totalResearchSeconds / totalRP;
	}

	qsort( taskArr, nTasks, sizeof(Task), SortTasks );
	current = 0;

	for( int i=0; i<nTasks; ++i ) {
		taskMap.Add( taskArr[i].name, &taskArr[i] ); 
		taskArr[i].taskMap = &taskMap;
		taskArr[i].itemMap = &itemMap;
	}

	itemArr = new Item[itemMax];
	nItems = 0;
	for( int i=0; i<nTasks; ++i ) {
		for( int k=0; k<MAX_ITEMS_REQUIRED; ++k ) {
			const char* item = taskArr[i].item[k];
			if ( item && !itemMap.Query( item, 0 ) ) {
				itemArr[nItems].found = false;
				itemArr[nItems].name = item;
				itemMap.Add( item, &itemArr[nItems] );
				++nItems;
			}
		}
	}
}


Research::~Research()
{
	delete [] taskArr;
	delete [] itemArr;
}


void Research::DoResearch( int rp )
{
	if ( Current() && Current()->rp < Current()->rpRequired ) {
		current->rp += rp;
		if ( current->rp >= current->rpRequired ) {
			current->rp = current->rpRequired;
		}
	}
}


const char* Research::GetDescription( const Task* task )
{
	const gamedb::Item* item = database->Root()->Child( "tree" )->Child( "research" )->Child( task->name );
	GLASSERT( item );
	return (const char*) database->AccessData( item, "text" );
}


void Research::SetItemAcquired( const char* name )
{
	Item* item=0;
	if ( itemMap.Query( name, &item ) ) {
		item->found = true;
	}

	char buf[20];
	SNPrintf( buf, 20, "%s", name );
	char* p = strstr( buf, "-3" );
	if ( p ) {
		*(p+1) = '2';
		if ( itemMap.Query( buf, &item ) ) {
			item->found = true;
		}
	}

	p = strstr( buf, "-2" );
	if ( p ) {
		*(p+1) = '1';
		if ( itemMap.Query( buf, &item ) ) {
			item->found = true;
		}
	}
}


void Research::KickResearch()
{
	if ( !ResearchInProgress() ) {
		GLASSERT( !current );
		for( int i=0; i<nTasks; ++i ) {
			// Just find the first one and try it. The UI only
			// allows selecting the first 3 tasks, but that number
			// could easily change.
			if ( !taskArr[i].IsComplete() ) {
				if ( taskArr[i].HasPreReq() && taskArr[i].HasItems() ) {
					current = &taskArr[i];
				}
				break;
			}
		}
	}
}


bool Research::Task::HasPreReq() const
{
	for( int i=0; i<MAX_PREREQ; ++i ) {
		if ( prereq[i] ) {
			Task* other = 0;
			taskMap->Query( prereq[i], &other );
			if ( other && !other->IsComplete() )
				return false;
		}
	}
	return true;
}


bool Research::Task::HasItems() const
{
	for( int i=0; i<MAX_ITEMS_REQUIRED; ++i ) {
		if ( item[i] ) {
			Item* other = 0;
			itemMap->Query( item[i], &other );
			GLASSERT( other );
			if ( other && !other->found )
				return false;
		}
	}
	return true;
}


int Research::GetStatus( const char* name ) const
{
	Task* task = 0;
	taskMap.Query( name, &task );
	if ( task ) {
		return task->IsComplete() ? TECH_RESEARCH_COMPLETE : TECH_NOT_RESEARCHED;
	}
	return TECH_FREE;
}


void Research::Save( FILE* fp, int depth )
{
	XMLUtil::OpenElement( fp, depth, "Research" );
	if ( current )
		XMLUtil::Attribute( fp, "current", current-taskArr );
	XMLUtil::SealElement( fp );

	for( int i=0; i<nTasks; ++i ) {
		XMLUtil::OpenElement( fp, depth+1, "Task" );
		XMLUtil::Attribute( fp, "name", taskArr[i].name );
		XMLUtil::Attribute( fp, "rp", taskArr[i].rp );
		XMLUtil::SealCloseElement( fp );
	}
	for( int i=0; i<nItems; ++i ) {
		XMLUtil::OpenElement( fp, depth+1, "Item" );
		XMLUtil::Attribute( fp, "name", itemArr[i].name );
		XMLUtil::Attribute( fp, "found", itemArr[i].found );
		XMLUtil::SealCloseElement( fp );
	}
	XMLUtil::CloseElement( fp, depth, "Research" );
}


void Research::Load( const TiXmlElement* doc )
{
	if ( !doc ) {
		GLASSERT( 0 );
		return;
	}

	GLASSERT( StrEqual( doc->Value(), "Research" ) );
	current = 0;
	int c = -1;
	if ( doc->QueryIntAttribute( "current", &c ) == TIXML_SUCCESS ) {
		current = &taskArr[c];
	}

	for( const TiXmlElement* ele=doc->FirstChildElement( "Task" ); ele; ele=ele->NextSiblingElement( "Task" ) ) {
		Task* t = 0;
		const char* name = ele->Attribute( "name" );
		if ( taskMap.Query( name, &t ) ) {
			ele->QueryIntAttribute( "rp", &t->rp );
		}
	}
	for( const TiXmlElement* ele=doc->FirstChildElement( "Item" ); ele; ele=ele->NextSiblingElement( "Item" ) ) {
		Item* t = 0;
		const char* name = ele->Attribute( "name" );
		if ( itemMap.Query( name, &t ) ) {
			ele->QueryBoolAttribute( "found", &t->found );
		}
	}
}


