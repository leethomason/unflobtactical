#include "battledata.h"
#include "../engine/serialize.h"
#include "../tinyxml/tinyxml.h"

void BattleData::CopyUnit( int i, const Unit& copyFrom )
{
	GLASSERT( i >= 0 && i < MAX_UNITS );
	units[i].Free();
	units[i] = copyFrom;
}


int BattleData::CalcResult() const
{
	int nTerransAlive = Unit::Count( units+TERRAN_UNITS_START, MAX_TERRANS, Unit::STATUS_ALIVE );
	int nAliensAlive  = Unit::Count( units+ALIEN_UNITS_START, MAX_ALIENS, Unit::STATUS_ALIVE );

	int result = TIE;
	if ( nTerransAlive > 0 && nAliensAlive == 0 )
		result = VICTORY;
	else if ( nTerransAlive == 0 && nAliensAlive > 0 )
		result = DEFEAT;
	return result;
}


void BattleData::Save( FILE* fp, int depth )
{
	XMLUtil::OpenElement( fp, depth, "BattleData" );
	XMLUtil::Attribute( fp, "dayTime", dayTime );
	XMLUtil::Attribute( fp, "scenario", scenario );
	XMLUtil::SealElement( fp );

	storage.Save( fp, depth+1 );

	XMLUtil::OpenElement( fp, depth+1, "Units" );
	XMLUtil::SealElement( fp );
	for( int i=0; i<MAX_UNITS; ++i ) {
		units[i].Save( fp, depth+2 );
	}
	XMLUtil::CloseElement( fp, depth+1, "Units" );
	XMLUtil::CloseElement( fp, depth, "BattleData" );
}


void BattleData::Load( const TiXmlElement* doc )
{
	for( int i=0; i<MAX_UNITS; ++i )
		units[i].Free();
	storage.Clear();

	const TiXmlElement* ele = doc->FirstChildElement( "BattleData" );
	GLASSERT( ele );

	ele->QueryBoolAttribute( "dayTime", &dayTime );
	ele->QueryIntAttribute( "scenario", &scenario );

	storage.Load( ele );
	const TiXmlElement* unitsEle = ele->FirstChildElement( "Units" );
	GLASSERT( unitsEle );

	int team[3] = { TERRAN_UNITS_START, CIV_UNITS_START, ALIEN_UNITS_START };
	if ( unitsEle ) {
		for( const TiXmlElement* unitElement = unitsEle->FirstChildElement( "Unit" );
			 unitElement;
			 unitElement = unitElement->NextSiblingElement( "Unit" ) ) 
		{
			int t = 0;
			unitElement->QueryIntAttribute( "team", &t );
			Unit* unit = &units[team[t]];

			unit->Load( unitElement, storage.GetItemDefArr() );
			
			team[t]++;

			GLRELASSERT( team[0] <= TERRAN_UNITS_END );
			GLRELASSERT( team[1] <= CIV_UNITS_END );
			GLRELASSERT( team[2] <= ALIEN_UNITS_END );
		}
	}
}
