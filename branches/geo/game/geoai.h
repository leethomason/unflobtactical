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

#ifndef UFO_ATTACK_GEOAI_INCLUDED
#define UFO_ATTACK_GEOAI_INCLUDED


#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrandom.h"

#include "gamelimits.h"
#include "../engine/ufoutil.h"

class SpaceTree;
class GeoMapData;
class Chit;
class RegionData;
class ChitBag;
class BaseChit;

class GeoAI
{
public:

	GeoAI( const GeoMapData& data );
	virtual ~GeoAI()	{}

	void GenerateAlienShip( int type, grinliz::Vector2F* start, grinliz::Vector2F* dest, const RegionData* data, const ChitBag& chitBag );

protected:
	int ComputeBasesInRegion( int region, const ChitBag& chitBag );

	const GeoMapData& geoMapData; 
	grinliz::Random random;

	// temporary for ComputeBasesInRegion
	enum {
		MAX_BASES = 4	// copied to a scary # of places
	};
	const BaseChit* baseInRegion[MAX_BASES];

};


#endif // UFO_ATTACK_GEOAI_INCLUDED