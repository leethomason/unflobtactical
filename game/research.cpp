#include "research.h"
#include "../grinliz/glstringutil.h"


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


Research::Research( const gamedb::Reader* _database )
{
	database = _database;
	current = 0;

	const gamedb::Item* researchItem = database->Root()->Child( "tree" )->Child( "research" );
	GLASSERT( researchItem );
	nTasks = researchItem->NumChildren();
	taskArr = new Task[nTasks];

	// Populate all the tasks.
	for( int i=0; i<nTasks; ++i ) {
		const gamedb::Item* item = researchItem->Child( i );
		memset( &taskArr[i], 0, sizeof( Task ) );
		taskArr[i].name = item->Name();
		taskArr[i].rp = 0;
		static const int CONVERSION_TO_RESEARCHER_SECONDS = 8;
		taskArr[i].rpRequired = CONVERSION_TO_RESEARCHER_SECONDS * item->GetInt( "points" );

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
				taskArr[i].item[nItem++] = reqItem->GetString( "item" );
			}
			else {
				GLASSERT( 0 );
			}
		}
	}

	qsort( taskArr, nTasks, sizeof(Task), SortTasks );
	current = taskArr;

	for( int i=0; i<nTasks; ++i ) {
		map.Add( taskArr[i].name, &taskArr[i] ); 
		taskArr[i].map = &map;
	}
}


Research::~Research()
{
	delete [] taskArr;
}


void Research::DoResearch( int rp )
{
	if ( current && current->rp < current->rpRequired ) {
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
	for( int i=0; i<nTasks; ++i ) {
		Task* task = &taskArr[i];
		if ( !task->IsComplete() ) {
			for( int k=0; k<MAX_ITEMS_REQUIRED; ++k ) {
				if ( task->item[k] && grinliz::StrEqual( task->item[k], name ) ) {
					task->item[k] = 0;
				}
			}
		}
	}
}


bool Research::Task::HasPreReq() const
{
	for( int i=0; i<MAX_PREREQ; ++i ) {
		if ( prereq[i] ) {
			Task* other = 0;
			map->Query( prereq[i], &other );
			if ( other && !other->IsComplete() )
				return false;
		}
	}
	return true;
}


int Research::GetStatus( const char* name ) const
{
	Task* task = 0;
	map.Query( name, &task );
	if ( task ) {
		return task->IsComplete() ? TECH_RESEARCH_COMPLETE : TECH_NOT_RESEARCHED;
	}
	return TECH_FREE;
}
