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
#include "../grinliz//glgeometry.h"
#include "../engine/serialize.h"

using namespace grinliz;


int Stats::GenStat( grinliz::Random* rand, int min, int max ) 
{
	int s = min + grinliz::LRintf( (float)(max-min) * rand->DiceUniform( 3, 6 ) );
	return Clamp( s, 1, TRAIT_MAX );
}


void Stats::CalcBaselines()
{
	int levSTR = Clamp( STR() + TRAIT_RANK_BONUS*Rank(), 1, TRAIT_MAX );
	int levDEX = Clamp( DEX() + TRAIT_RANK_BONUS*Rank(), 1, TRAIT_MAX );
	int levPSY = Clamp( PSY() + TRAIT_RANK_BONUS*Rank(), 1, TRAIT_MAX );

	totalHP =					levSTR;

	totalTU = Interpolate(		0.0f,						(float)MIN_TU,
								(float)TRAIT_MAX,			(float)MAX_TU,
								(float)(levDEX + levSTR)*0.5f );
	// be sure...
	if ( totalTU > (float)MAX_TU )
		totalTU = (float)MAX_TU;

	accuracy = Interpolate(		0.0f,					ACC_WORST_SHOT,
								(float)TRAIT_MAX,		ACC_BEST_SHOT,
								(float)levDEX);

	reaction = Interpolate(		0.0f,					REACTION_FAST,
								(float)TRAIT_MAX,		REACTION_SLOW,
								(float)(levDEX + levPSY)*0.5f );
	constitution = (float)levPSY;
}


void Stats::Save( FILE* fp, int depth ) const
{
	/*
	TiXmlElement* element = new TiXmlElement( "Stats" );
	element->SetAttribute( "STR", _STR );
	element->SetAttribute( "DEX", _DEX );
	element->SetAttribute( "PSY", _PSY );
	element->SetAttribute( "rank", rank );
	doc->LinkEndChild( element );
	*/
	XMLUtil::OpenElement( fp, depth, "Stats" );
	XMLUtil::Attribute( fp, "STR", _STR );
	XMLUtil::Attribute( fp, "DEX", _DEX );
	XMLUtil::Attribute( fp, "PSY", _PSY );
	XMLUtil::Attribute( fp, "rank", rank );
	XMLUtil::SealCloseElement( fp );
}


void Stats::Load( const TiXmlElement* parent )
{
	const TiXmlElement* ele = parent->FirstChildElement( "Stats" );
	GLASSERT( ele );
	if ( ele ) {
		ele->QueryIntAttribute( "STR", &_STR );
		ele->QueryIntAttribute( "DEX", &_DEX );
		ele->QueryIntAttribute( "PSY", &_PSY );
		ele->QueryIntAttribute( "level", &rank );
		ele->QueryIntAttribute( "rank", &rank );
		CalcBaselines();
	}
}


void BulletSpread::Generate( U32 seed, grinliz::Vector2F* result )
{
	const static float MULT = 1.f / 65535.f;

	Random rand( seed );
	rand.Rand();
	rand.NormalVector2D( &result->x );
	float u = rand.Uniform();
	result->x *= u;
	result->y *= u;
}


void BulletSpread::Generate( U32 seed, const Accuracy& accuracy, float distance, const grinliz::Vector3F& dir, const grinliz::Vector3F& target, grinliz::Vector3F* targetPrime )
{
	Vector2F spread;
	Generate( seed, &spread );
	spread.x *= distance * accuracy.RadiusAtOne();
	spread.y *= distance * accuracy.RadiusAtOne();

	Vector3F normal = dir; 
	normal.Normalize();

	const static Vector3F UP = { 0, 1, 0 };
	Vector3F right;
	CrossProduct( normal, UP, &right );

	*targetPrime = target + spread.x*right + spread.y*UP;
}


float BulletSpread::ComputePercent( const Accuracy& accuracy, const BulletTarget& target )
{
	// Tests:
	// Game accuracy: predicted=0.40 actual=0.32
	// Game accuracy: predicted=0.36 actual=0.34
	// Game accuracy: predicted=0.59 actual=0.46
	// Game accuracy: predicted=0.53 actual=0.39
	// average:					0.47        0.38
	//
	// with KLUGDE & bullseye: Game accuracy: predicted=0.29 actual=0.29. Groovy. Done with this, hopefully.

	static const float KLUDGE = 0.38f / 0.47f;
	static const float EPS = 0.1f;

	GLASSERT( target.distance > 0 );
	GLASSERT( target.width > 0 );
	GLASSERT( target.height > 0 );

	Vector2F center = { target.width*0.50f, target.height*0.65f };
	
	Rectangle2F bullseye, board;
	board.Set( 0, 0, target.width, target.height );
	bullseye.Set(	center.x-target.width*EPS, center.y-target.height*EPS,
					center.x+target.width*EPS, center.y+target.height*EPS );

	float hit = 0;

	const int SAMPLES = 117;
	for( int i=0; i<SAMPLES; ++i ) {
		Vector2F v;
		Generate( i, &v );

		Vector2F pos;
		pos.x = center.x + v.x*target.distance*accuracy.RadiusAtOne();
		pos.y = center.y + v.y*target.distance*accuracy.RadiusAtOne();

		if ( bullseye.Contains( pos ) ) {
			hit += 1.0f;
		}
		else if ( board.Contains( pos ) ) {
			hit += KLUDGE;
		}
	}
	float result = hit / (float)SAMPLES;
	return Clamp( result, 0.0f, 0.95f );
}
