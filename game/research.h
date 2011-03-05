#ifndef UFOATTACK_RESEARCH_INCLUDED
#define UFOATTACK_RESEARCH_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glstringutil.h"
#include "../shared/glmap.h"
#include "../shared/gamedbreader.h"

class ItemDefArr;
class ItemDef;

class Research
{
public:
	enum { 
		MAX_ITEMS_REQUIRED = 4,
		MAX_PREREQ = 4
	};

	struct Task {
		const char* name;
		int rp;
		int rpRequired;
		const char*	item[MAX_ITEMS_REQUIRED];	// note this can have null pointer gaps as items get acquired
		const char* prereq[MAX_PREREQ];
		const CStringMap<Research::Task*>* map;			

		bool IsComplete() const { return rp == rpRequired; }
		bool HasItems() const { for( int i=0; i<MAX_ITEMS_REQUIRED; ++i ) { if ( item[i] ) return false; } return true; }
		bool HasPreReq() const;

	};

	Research( const gamedb::Reader* database );
	~Research();

	void DoResearch( int researcherSeconds );
	bool ResearchReady() const  { return current && current->IsComplete(); }
	void SetItemAcquired( const char* name );
	
	const Task* Current() const { return current; }
	void SetCurrent( const char* name ) {
		current = 0; 
		if ( name ) {
			Task* t = 0;
			map.Query( name, &t );
			if ( t ) current = t;
		}
	}

	const char* GetDescription( const Task* task );
	const Task* TaskArr() const { return taskArr; }
	int NumTasks() const { return nTasks; }

private:
	static int SortTasks( const void* _a, const void* _b );

	CStringMap<Research::Task*> map;
	const gamedb::Reader* database;
	Task* current;
	Task* taskArr;
	int nTasks;
};


#endif // UFOATTACK_RESEARCH_INCLUDED