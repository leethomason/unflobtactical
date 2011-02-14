#include "geoai.h"
#include "geoscene.h"

using namespace grinliz;


GeoAI::GeoAI( const GeoMapData& _data ) : geoMapData( _data )
{
}


void GeoAI::GenerateAlienShip( int type, grinliz::Vector2F* start, grinliz::Vector2F* end, const RegionData* data )
{
	float score[GEO_REGIONS*2] = { 0 };
	Vector2I dest = { -1, -1 };

	if ( dest.x < 0 ) {
		// Can the aliens occupy a capital or attack a base?
		if ( type == UFOChit::BATTLESHIP ) {
			// Base attack is a dicey business.
			if ( random.Bit() ) {
				// Consider it.
				int count=0;
				for( int i=0; i<GEO_REGIONS; ++i ) {
					score[i] = 0;
					if (    data[i].influence >= MIN_BASE_ATTACK_INFLUENCE 
						 && data[i].NumBases() )
					{
						++count;
						score[i] = data[i].Score();
					}
				}
				if ( count > 0 ) {
					int region = random.Select( score, GEO_REGIONS );
					int id = random.Rand( data[region].NumBases() );
					dest = data[region].Base( id );
				}
			}

			// We may or may not have found a match. Now
			// look at occupation options.
			if ( dest.x < 0 ) {
				int count=0;
				for( int i=0; i<GEO_REGIONS; ++i ) {
					score[i] = 0;
					if (    data[i].influence >= MIN_OCCUPATION_INFLUENCE 
						 &&	!data[i].occupied )
					{
						++count;
						score[i] = data[i].Score();
					}
				}
				if ( count > 0 ) {
					int region = random.Select( score, GEO_REGIONS );
					int cityID = geoMapData.CapitalID( region );
					dest = geoMapData.City( region, cityID );
				}
			}
		}
	}
	if ( dest.x < 0 ) {
		// Can the aliens attack a city?
		if (    type == UFOChit::BATTLESHIP 
			 || type == UFOChit::FRIGATE ) 
		{
			int count=0;
			for( int i=0; i<GEO_REGIONS; ++i ) {
				score[i] = 0;
				if (    data[i].influence >= MIN_CITY_ATTACK_INFLUENCE 
					 &&	!data[i].occupied )
				{
					++count;
					score[i] = data[i].Score();
				}
			}
			if ( count > 0 ) {
				int region = random.Select( score, GEO_REGIONS );
				int cityID = random.Rand( geoMapData.NumCities(region) );
				dest = geoMapData.City( region, cityID );
			}
		}
	}

	// Crop circle!
	if ( dest.x < 0 ) {
		int count = 0;
		for( int i=0; i<GEO_REGIONS; ++i ) {
			score[i] = 0;
			if ( !data[i].occupied ) {
				if ( type == UFOChit::SCOUT )
					score[i] = 1;						// make the scouts scout.
				else
					score[i] = data[i].Score();			// be more careful with the heavy ships
			}
		}
		int region = random.Select( score, GEO_REGIONS );
		geoMapData.Choose( &random, region, 0, GeoMapData::CITY, &dest );
		GLASSERT( !geoMapData.IsCity( dest.x, dest.y ) );
	}

	GLASSERT( dest.x >= 0 );

	end->Set( (float)dest.x + 0.5f, (float)dest.y + 0.5f );
	float len = (float)GEO_MAP_X * (0.7f + random.Uniform()*0.2f );
	start->x = end->x + len*(float)random.Sign();
	start->y = (float)GEO_MAP_Y * random.Uniform();

#ifdef DEBUG
	{
		// Scouts should never end at cities.
		if ( type == UFOChit::SCOUT ) {
			GLASSERT( !geoMapData.IsCity( dest.x, dest.y ) );
		}
	}
#endif
}



