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
class RingEffect;
class BaseChit;
class UFOChit;
class Chit;
class Storage;
class ItemDefArr;

static const int GEO_MAP_X = 20;
static const int GEO_MAP_Y = 10;
static const float GEO_MAP_XF = (float)GEO_MAP_X;
static const float GEO_MAP_YF = (float)GEO_MAP_Y;
static const int GEO_REGIONS = 6;
static const int MAX_BASES = 4;

static const int MAX_CROP_CIRCLE_INFLUENCE = 6;
static const int CROP_CIRCLE_INFLUENCE = 1;
static const int MIN_CITY_ATTACK_INFLUENCE = 4;		// needed to attack cities
static const int CITY_ATTACK_INFLUENCE = 2;
static const int MIN_BASE_ATTACK_INFLUENCE = 7;
static const int MIN_OCCUPATION_INFLUENCE = 9;
static const int MAX_INFLUENCE = 10;

static const float UFO_HEIGHT = 0.5f;
static const float UFO_SPEED[3] = { 1.00f, 0.85f, 0.70f };
static const float CARGO_SPEED  = 0.40f;
static const float UFO_HP[3]    = { 4, 8, 20 };
static const float UFO_ACCEL = 0.2f;	// units/second2

static const float UFO_LAND_TIME  = 10*1000;
static const float UFO_CRASH_TIME = 20*1000;

static const U32	MISSILE_FREQ		= 1500;
static const U32	GUN_FREQ			=  800;
static const float	MISSILE_SPEED		= 1.5f;		// faster than UFOs.
static const U32	MISSILE_LIFETIME	= 3000;		// range = lifetime * speed
static const float	GUN_SPEED			= 2.0f;
static const U32	GUN_LIFETIME		=  800;
static const float	MISSILE_HEIGHT		= 0.6f;


inline U32	 MissileFreq( int type )  { return ( type == 0 ) ? GUN_FREQ : MISSILE_FREQ; }
inline float MissileRange( int type ) { return ( type == 0 ) ? (float)GUN_LIFETIME * GUN_SPEED / 1000.0f : (float)MISSILE_LIFETIME * MISSILE_SPEED / 1000.0f; }
inline float MissileSpeed( int type ) { return ( type == 0 ) ? GUN_SPEED : MISSILE_SPEED; }
inline U32   MissileLifetime( int type ) { return ( type == 0 ) ? GUN_LIFETIME : MISSILE_LIFETIME; }




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
	int NumLand() const { return numLand; }

private:
	int Find( U8* choiceBuffer, int bufSize, int region, int required, int excluded ) const;

	int Type( U16 d ) const { return d & 0xff; }
	int Region( U16 d ) const { return d >> 8; }

	// low 8 bits type, high 8 bits region#
	int numLand;
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

	void AddBase( const grinliz::Vector2I& loc )		{ grinliz::Vector2I* v = base.Push(); *v = loc; }
	void RemoveBase( const grinliz::Vector2I& loc ) {
		for( unsigned i=0; i<base.Size(); ++i ) {
			if ( base[i] == loc ) {
				base.SwapRemove( i );
				return;
			}
		}
		GLASSERT( 0 );
	}
	int NumBases() const							{ return base.Size(); }
	const grinliz::Vector2I& Base( int i ) const	{ return base[i]; }

	void Init( const ItemDefArr& itemDefArr );
	void Free();

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

	Storage* GetStorage() { return storage; }

private:
	void Push( int d )	{
		for( int i=HISTORY-1; i>0; --i ) {
			history[i] = history[i-1];
		}
		history[0] = d;
	}

	Storage* storage;
	CArray< grinliz::Vector2I, MAX_BASES > base;
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
	struct Missile {
		int type;
		grinliz::Vector2F	pos;
		grinliz::Vector2F	velocity;
		U32					time;			// accumulates time each frame
		U32					lifetime;		// reduces as time is used up
	};

	void SetMapLocation();
	void InitPlaceBase();
	void InitLandTiles();
	void FireBaseWeapons();
	void UpdateMissiles( U32 deltaTime );

	void PlaceBase( const grinliz::Vector2I& map );
	bool CanSendCargoPlane( const grinliz::Vector2I& base );

	void HandleItemTapped( const gamui::UIItem* item );
	
	enum {
		CM_NONE,
		CM_BASE,
		CM_UFO
	};
	void InitContextMenu( int type, Chit* chit );
	void UpdateContextMenu();

	BaseChit* IsBaseLocation( int x, int y );
	int CalcBaseChits( BaseChit* array[MAX_BASES] );	// fills an array with all the base chits.

	GeoMap* geoMap;
	SpaceTree* tree;
	GeoAI*	geoAI;

	grinliz::Vector2F	dragStart, dragLast;
	U32					lastAlienTime;
	U32					timeBetweenAliens;
	U32					missileTimer[2];
	Chit*				contextChit;
	
	int					cash;

	grinliz::Random		random;

	gamui::PushButton	helpButton, researchButton;
	gamui::ToggleButton baseButton;
	gamui::Image		cashBackground;
	gamui::TextLabel	cashText;

	enum {
		CONTEXT_CARGO,
		CONTEXT_EQUIP,
		CONTEXT_BUILD,
		MAX_CONTEXT = 6
	};
	gamui::PushButton	context[MAX_CONTEXT];

	AreaWidget*			areaWidget[GEO_REGIONS];
	RegionData regionData[GEO_REGIONS];
	CDynArray< Chit* >	chitArr;
	CDynArray< Missile > missileArr;
	GeoMapData			geoMapData;
};





#endif // UFO_ATTACK_GEO_SCENE_INCLUDED