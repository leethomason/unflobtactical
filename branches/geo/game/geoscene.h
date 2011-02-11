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

#ifndef UFO_ATTACK_GEO_SCENE_INCLUDED
#define UFO_ATTACK_GEO_SCENE_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glrandom.h"

#include "scene.h"

#include "../engine/ufoutil.h"
#include "../engine/texture.h"
#include "../engine/surface.h"
#include "../engine/model.h"

class UIImage;
class UIButtonBox;
class UIButtonGroup;
class AreaWidget;
class Model;
class GeoMap;
class GeoAI;


static const int GEO_MAP_X = 20;
static const int GEO_MAP_Y = 10;
static const int GEO_REGIONS = 6;

static const int MAX_CROP_CIRCLE_INFLUENCE = 6;
static const int CROP_CIRCLE_INFLUENCE = 1;
static const int MIN_CITY_ATTACK_INFLUENCE = 4;		// needed to attack cities
static const int CITY_ATTACK_INFLUENCE = 2;
static const int MIN_OCCUPATION_INFLUENCE = 9;


class GeoMapData
{
	friend class GeoScene;

	enum { 
		MAX_CITIES_PER_REGION = 6
	};


public:
	enum {
		WATER	= 0,
		FARM	= 0x01,
		FOREST	= 0x02,
		DESERT  = 0x04,
		TUNDRA	= 0x08,
		CITY	= 0x10, 
	};

	void Init( const char* map, grinliz::Random* random );

	void Choose( grinliz::Random* random, int region, int required, int excluded, grinliz::Vector2I* position ) const;
	bool IsWater( int x, int y ) const 
	{
		GLASSERT( x >= 0 && x < GEO_MAP_X );
		GLASSERT( y >= 0 && y < GEO_MAP_Y );
		return (data[y*GEO_MAP_X+x] == 0);
	}
	int GetRegion( int x, int y ) const
	{
		GLASSERT( x >= 0 && x < GEO_MAP_X );
		GLASSERT( y >= 0 && y < GEO_MAP_Y );
		GLASSERT( !IsWater( x, y ) );
		return (data[y*GEO_MAP_X+x] >> 8);
	}
	int GetType( int x, int y ) const
	{
		GLASSERT( x >= 0 && x < GEO_MAP_X );
		GLASSERT( y >= 0 && y < GEO_MAP_Y );
		return (data[y*GEO_MAP_X+x] & 0xff );
	}
	int NumCities( int region ) const 
	{
		GLASSERT( region >= 0 && region < GEO_REGIONS );
		return numCities[region];
	}
	grinliz::Vector2I City( int region, int index ) const 
	{
		GLASSERT( region >= 0 && region < GEO_REGIONS );
		GLASSERT( index >= 0 && index < NumCities( region ) );
		grinliz::Vector2I v;
		v.y = city[region*MAX_CITIES_PER_REGION+index] / GEO_MAP_X;
		v.x = city[region*MAX_CITIES_PER_REGION+index] - v.y*GEO_MAP_X;
		return v;
	}
	int CapitalID( int region ) const 
	{
		GLASSERT( region >= 0 && region < GEO_REGIONS );
		return capital[region];
	}
	grinliz::Vector2I Capital( int region ) const 
	{
		int id = CapitalID( region );
		return City( region, id );
	}
	bool IsCity( int x, int y ) const 
	{
		return ( Type( data[GEO_MAP_X*y+x] ) == CITY ); 
	}

private:
	int Find( U8* choiceBuffer, int bufSize, int region, int required, int excluded ) const;

	int Type( U16 d ) const { return d & 0xff; }
	int Region( U16 d ) const { return d >> 8; }

	// low 8 bits type, high 8 bits region#
	U16 data[GEO_MAP_X*GEO_MAP_Y];
	grinliz::Rectangle2I bounds[GEO_REGIONS];

	U8 numCities[GEO_REGIONS];
	U8 capital[GEO_REGIONS];						// index to capital (also a city)
	U8 city[GEO_REGIONS*MAX_CITIES_PER_REGION];	// index to cities (one will be a capital)
};


class RegionData 
{
public:
	enum {
		HISTORY = 5
	};
	int influence;
	int history[HISTORY];
	bool occupied;			// true if this is currently under alien occupation

	void Init()	{ 
		influence=0; 
		for( int i=0; i<HISTORY; ++i ) history[i] = 1; 
		occupied = false;
	}
	void Success()			{ Push( 2 ); }
	void UFODestroyed()		{ Push( 0 ); }

	float Score() const			
	{
		int hScore = 0;
		for( int i=0; i<HISTORY; ++i ) {
			hScore += history[i];
		}
		return grinliz::Max( ((float)influence*0.10f) * ((float)hScore*0.5f), 0.2f );	// the .2 adds some randomness to the alien actions
	}

private:
	void Push( int d )	{
		for( int i=HISTORY-1; i>0; --i ) {
			history[i] = history[i-1];
		}
		history[0] = d;
	}
};


class Chit
{
protected:
	Chit( SpaceTree* tree );
public:
	virtual ~Chit();

	enum {
		MSG_DONE,						// delete this chit, left orbit, time up
		MSG_UFO_AT_DESTINATION,			// create a crop circle, attack a city, occupy a capital
		MSG_CROP_CIRCLE_COMPLETE,
		MSG_CITY_ATTACK_COMPLETE
	};
	virtual void DoTick( U32 deltaTime, CDynArray<int>* message ) = 0;

	grinliz::Vector2I MapPos() const { grinliz::Vector2I v = { (int)pos.x, (int)pos.y }; return v; }
	virtual bool Parked() const { return false; }

protected:
	grinliz::Vector2F pos;		// in map units
	grinliz::Vector2F dest;		// in map units
	Model* model[2];			// use 2 models, so that the camera can fake scrolling. the spacetree will sort it all out.
	SpaceTree* tree;
};


// FIXME: move to memory pools?
class TravellingUFO : public Chit
{
public:
	enum {
		AI_NONE,		// no ai, needs processing
		TRAVELLING,		// on route to destination
		ORBIT,			// accelerating to orbit
		CRASHED,		// crashed UFO

		CITY_ATTACK,	// attacking a city
		OCCUPATION,		// a battleship has occupied a capital. It will not leave (until destroyed)
		CROP_CIRCLE,	// chillin' and making crop circles
	};

	enum {
		SCOUT,
		FRIGATE,
		BATTLESHIP,
		NUM_TYPES
	};
	TravellingUFO(	SpaceTree* tree, 
					int type, 
					const grinliz::Vector2F& start, 
					const grinliz::Vector2F& dest );
	~TravellingUFO();

	virtual void DoTick( U32 deltaTime, CDynArray<int>* message );

	virtual bool Parked() const
	{
		if ( ai == CITY_ATTACK || ai == CRASHED || ai == OCCUPATION || ai == CROP_CIRCLE ) {
			return true;
		}
		return false;
	}

	int Type() const { return type; }
	void SetAI( int _ai ) 
	{
		GLASSERT( ai == AI_NONE );
		ai = _ai;
	}

private:
	void EmitEntryExitBurn( U32 deltaTime, const grinliz::Vector3F& p0, const grinliz::Vector3F& p1, bool entry );
	void Decal( U32 timer, float a, float b );
	void RemoveDecal();

	int type;
	int ai;
	float velocity;
	
	U32 effectTimer;
	U32 jobTimer;

	Model* decal[2];
};


class CropCircle : public Chit
{
public:
	CropCircle( SpaceTree* tree, const grinliz::Vector2I& pos, U32 seed );
	~CropCircle();

	virtual void DoTick( U32 deltaTime, CDynArray<int>* message );

private:
	enum {
		CROP_CIRCLE_TIME_SECONDS = 20
	};
	U32 jobTimer;
};


class CityChit : public Chit
{
public:
	CityChit( SpaceTree* tree, const grinliz::Vector2I& pos, bool capital );
	~CityChit()	{}

	virtual void DoTick(  U32 deltaTime, CDynArray<int>* message )	{}

private:
	bool capital;
};


class GeoScene : public Scene
{
public:
	GeoScene( Game* _game );
	virtual ~GeoScene();

	virtual void Activate();
	virtual void DeActivate();

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	
	{ 
		clip3D->SetInvalid(); 
		clip2D->SetInvalid(); 
		return RENDER_2D | RENDER_3D;
	}	

	virtual void DoTick( U32 currentTime, U32 deltaTime );
	virtual void Debug3D();


private:
	void SetMapLocation();
	GeoMap* geoMap;
	SpaceTree* tree;
	GeoAI*	geoAI;

	grinliz::Vector2F	dragStart;
	U32					lastAlienTime;
	U32					timeBetweenAliens;

	grinliz::Random		random;
	CDynArray<int>		message;

	AreaWidget*			areaWidget[GEO_REGIONS];
	RegionData regionData[GEO_REGIONS];
	CDynArray< Chit* >	chitArr;
	GeoMapData			geoMapData;
};





#endif // UFO_ATTACK_GEO_SCENE_INCLUDED