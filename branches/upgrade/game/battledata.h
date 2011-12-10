#ifndef UFOATTACK_BATTLEDATA_INCLUDED
#define UFOATTACK_BATTLEDATA_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

#include "item.h"
#include "unit.h"


/*	If a battle is in progress, this saves the data
	across the many scenes. (BattleScene, EndScene,
	etc.) Too many crashing errors when passing data
	between them.
*/
class BattleData
{
public:
	BattleData( const ItemDefArr& arr ) : storage( 0, 0, arr )
	{
		Init();
	}
	void Init() {
		for( int i=0; i<MAX_UNITS; ++i )
			units[i].Free();
		dayTime = true;
		scenario = 0;
		storage.Clear();
	}
	void ClearStorage() { storage.Clear(); }

	enum {
		VICTORY		= 1,
		DEFEAT		= 2,
		TIE			= 3,
	};

	int CalcResult() const;
	bool IsBattleOver() const {
		int result = CalcResult();
		return (result==VICTORY) || (result==DEFEAT);
	}

	void Save( FILE* fp, int depth );
	void Load( const TiXmlElement* doc );

	const Unit* Units( int start=0 ) const  { GLASSERT( start >= 0 && start < MAX_UNITS ); return units + start; }
	const Storage& GetStorage() const		{ return storage; }
	Storage* StoragePtr()					{ return &storage; }

	void CopyUnit( int slot, const Unit& copyFrom );

	Unit* UnitsPtr() { return units; }
	Unit* AlienPtr() {
		for( int i=ALIEN_UNITS_START; i<ALIEN_UNITS_END; ++i ) units[i].Free();
		return units + ALIEN_UNITS_START;
	}
	Unit* CivPtr() {
		for( int i=CIV_UNITS_START; i<CIV_UNITS_END; ++i ) units[i].Free();
		return units + CIV_UNITS_START;
	}

	void SetDayTime( bool dayTime )		{ this->dayTime = dayTime; }
	bool GetDayTime() const				{ return dayTime; } 
	void SetScenario( int scenario )	{ this->scenario = scenario; }
	int GetScenario() const				{ return scenario; }

private:
	BattleData( const BattleData& );	// not supported

	Unit units[MAX_UNITS];
	
	bool dayTime;
	int scenario;

	Storage storage;
};

#endif // UFOATTACK_BATTLEDATA_INCLUDED
