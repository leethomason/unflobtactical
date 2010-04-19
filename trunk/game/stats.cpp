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

#include "stats.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glrandom.h"
#include "gamelimits.h"
#include "../tinyxml/tinyxml.h"
#include "unit.h"

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

	GLASSERT( armor >= 0 && armor <= ARMOR_AMOUNT*NUM_ARMOR );
	totalHP =	Interpolate(	0, 0,
								TRAIT_MAX+ARMOR_AMOUNT*NUM_ARMOR, TRAIT_MAX,
								levSTR + armor );

	totalTU = Interpolate(	0.0f,						(float)MIN_TU,
							(float)TRAIT_MAX,			(float)MAX_TU,
							(float)(levDEX + levSTR)*0.5f );
	// be sure...
	if ( totalTU > (float)MAX_TU )
		totalTU = (float)MAX_TU;

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
//	element->SetAttribute( "hp", hp );
//	element->SetDoubleAttribute( "tu", tu );
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

//		ele->QueryValueAttribute( "hp", &hp );
//		ele->QueryValueAttribute( "tu", &tu );
	}
}


