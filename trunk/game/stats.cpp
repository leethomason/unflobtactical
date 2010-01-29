#include "stats.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glrandom.h"
#include "gamelimits.h"
#include "../tinyxml/tinyxml.h"

using namespace grinliz;


int Stats::GenStat( grinliz::Random* rand, int min, int max ) 
{
	return min + grinliz::LRintf( (float)(max-min) * rand->DiceUniform( 3, 6 ) );
}


void Stats::CalcBaselines()
{
	int levSTR = STR() + TRAIT_LEVEL_BONUS*Level();
	int levDEX = DEX() + TRAIT_LEVEL_BONUS*Level();
	int levPSY = PSY() + TRAIT_LEVEL_BONUS*Level();

	hp = totalHP =				levSTR;

	tu = totalTU = Interpolate(	(float)TRAIT_SOLDIER_LOW,  NORMAL_TU*0.5f,
								(float)TRAIT_SOLDIER_HIGH, NORMAL_TU*1.5f,
								(float)(levDEX + levSTR)*0.5f );
	tu = Clamp( tu, 2.0f, (float)MAX_TU );

	accuracy = Interpolate(		(float)TRAIT_SOLDIER_LOW,	ACC_BAD_SHOT,
								(float)TRAIT_SOLDIER_HIGH,	ACC_GOOD_SHOT,
								(float)levDEX );

	accuracy = Max( accuracy, 0.01f );	// no one is a perfect shot.

	reaction = Interpolate(		(float)TRAIT_SOLDIER_LOW,	REACTION_FAST,
								(float)TRAIT_SOLDIER_HIGH,	REACTION_SLOW,
								(float)levDEX );
}


void Stats::Save( TiXmlElement* doc ) const
{
	TiXmlElement* element = new TiXmlElement( "Stats" );
	element->SetAttribute( "STR", _STR );
	element->SetAttribute( "DEX", _DEX );
	element->SetAttribute( "PSY", _PSY );
	element->SetAttribute( "level", level );
	element->SetAttribute( "hp", hp );
	element->SetDoubleAttribute( "tu", tu );
	doc->LinkEndChild( element );
}


void Stats::Load( const TiXmlElement* parent )
{
	const TiXmlElement* ele = parent->FirstChildElement( "Stats" );
	GLASSERT( ele );
	if ( ele ) {
		ele->QueryValueAttribute( "STR", &_STR );
		ele->QueryValueAttribute( "DEX", &_DEX );
		ele->QueryValueAttribute( "PSY", &_PSY );
		ele->QueryValueAttribute( "level", &level );
		CalcBaselines();
		ele->QueryValueAttribute( "hp", &hp );
		ele->QueryValueAttribute( "tu", &tu );
	}
}


