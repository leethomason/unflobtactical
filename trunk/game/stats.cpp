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
	int levSTR = Clamp( STR() + TRAIT_RANK_BONUS*Rank(), 1, TRAIT_MAX );
	int levDEX = Clamp( DEX() + TRAIT_RANK_BONUS*Rank(), 1, TRAIT_MAX );
	int levPSY = Clamp( PSY() + TRAIT_RANK_BONUS*Rank(), 1, TRAIT_MAX );

	hp = totalHP =				Clamp( levSTR + armor, 1, TRAIT_MAX );

	tu = totalTU = Interpolate(	0.0f,						(float)MIN_TU,
								(float)TRAIT_MAX,			(float)MAX_TU,
								(float)(levDEX + levSTR)*0.5f );
	// be sure...
	if ( tu > (float)MAX_TU )
		tu = (float)MAX_TU;

	accuracy = Interpolate(		0.0f,						ACC_WORST_SHOT,
								(float)TRAIT_MAX,			ACC_BEST_SHOT,
								(float)levDEX );

	reaction = Interpolate(		0.0f,						REACTION_FAST,
								(float)TRAIT_MAX,			REACTION_SLOW,
								(float)(levDEX + levPSY)*0.5f );
}


void Stats::Save( TiXmlElement* doc ) const
{
	TiXmlElement* element = new TiXmlElement( "Stats" );
	element->SetAttribute( "STR", _STR );
	element->SetAttribute( "DEX", _DEX );
	element->SetAttribute( "PSY", _PSY );
	element->SetAttribute( "rank", rank );
	element->SetAttribute( "armor", armor );
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
		ele->QueryValueAttribute( "level", &rank );
		ele->QueryValueAttribute( "rank", &rank );
		ele->QueryValueAttribute( "armor", &armor );
		CalcBaselines();
		ele->QueryValueAttribute( "hp", &hp );
		ele->QueryValueAttribute( "tu", &tu );
	}
}


