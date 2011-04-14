#ifndef UFOATTACK_RESEARCH_INCLUDED
#define UFOATTACK_RESEARCH_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glstringutil.h"
#include "../shared/glmap.h"
#include "../shared/gamedbreader.h"

class ItemDefArr;
class ItemDef;
class TiXmlElement;

class Research
{
public:
	struct Item;

	enum { 
		MAX_ITEMS_REQUIRED = 4,
		MAX_PREREQ = 4
	};

	struct Task {
		const char* name;
		int rp;
		int rpRequired;
		const char*	item[MAX_ITEMS_REQUIRED];
		const char* prereq[MAX_PREREQ];
		const CStringMap<Research::Task*>* taskMap;			
		const CStringMap<Research::Item*>* itemMap;			

		bool IsComplete() const { return rp == rpRequired; }
		
		bool HasItems() const;
		bool HasPreReq() const;
	};

	struct Item {
		const char* name;
		bool found;
	};

	Research( const gamedb::Reader* database, const ItemDefArr& itemDefArr, int totalResearchTime );
	~Research();

	void DoResearch( int researcherSeconds );
	bool ResearchReady() const			{ return Current() && Current()->IsComplete(); }
	bool ResearchInProgress() const		{ return current != 0; }

	void KickResearch();	// if no research being done, do something
	void SetItemAcquired( const char* name );
	
	const Task* Current() const { return current; }
	void SetCurrent( const char* name ) {
		current = 0; 
		if ( name ) {
			Task* t = 0;
			taskMap.Query( name, &t );
			if ( t ) current = t;
		}
	}

	const char* GetDescription( const Task* task );
	const Task* TaskArr() const { return taskArr; }
	int NumTasks() const { return nTasks; }

	enum {
		TECH_FREE,
		TECH_NOT_RESEARCHED,
		TECH_RESEARCH_COMPLETE
	};
	int GetStatus( const char* name ) const;

	void Save( FILE* fp, int depth );
	void Load( const TiXmlElement* doc );

private:
	static int SortTasks( const void* _a, const void* _b );

	CStringMap<Research::Task*> taskMap;
	CStringMap<Research::Item*> itemMap;

	const gamedb::Reader* database;

	Task* current;
	
	Task* taskArr;
	Item* itemArr;
	int nTasks;
	int nItems;
};


#endif // UFOATTACK_RESEARCH_INCLUDED