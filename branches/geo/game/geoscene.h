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

static const float UFO_HEIGHT = 0.5f;
static const float UFO_SPEED[3] = { 1.00f, 0.85f, 0.70f };
static const float UFO_HP[3]    = { 4, 8, 20 };
static const float UFO_ACCEL = 0.2f;	// units/second2

static const float UFO_LAND_TIME  = 10*1000;
static const float UFO_CRASH_TIME = 20*1000;

static const U32 MISSILE_FREQ		= 1500;
static const U32 GUN_FREQ			=  800;
static const float MISSILE_SPEED	= 1.5f;		// faster than UFOs.
static const U32   MISSILE_LIFETIME = 3000;		// range = lifetime * speed
static const float GUN_SPEED		= 2.0f;
static const U32   GUN_LIFETIME		=  800;

static const float MISSILE_HEIGHT = 0.6f;


inline U32	 MissileFreq( int type )  { return ( type == 0 ) ? GUN_FREQ : MISSILE_FREQ; }
inline float MissileRange( int type ) { return ( type == 0 ) ? (float)GUN_LIFETIME * GUN_SPEED / 1000.0f : (float)MISSILE_LIFETIME * MISSILE_SPEED / 1000.0f; }
inline float MissileSpeed( int type ) { return ( type == 0 ) ? GUN_SPEED : MISSILE_SPEED; }
inline U32   MissileLifetime( int type ) { return ( type == 0 ) ? GUN_LIFETIME : MISSILE_LIFETIME; }


template < class T > 
class Cylinder
{
public:
	static bool InBounds( const grinliz::Vector2<T>& a ) {
		return a.x >= 0 && a.x < GEO_MAP_X;
	}

	static grinliz::Vector2<T> Normalize( const grinliz::Vector2<T>& a )
	{
		grinliz::Vector2<T> v = a;
		int it=0;
		while ( it<4 && v.x >= GEO_MAP_X )	v.x -= GEO_MAP_X, ++it;
		while ( it<4 && v.x < 0 )			v.x += GEO_MAP_X, ++it;
		GLASSERT( InBounds( v ) );
		return v;
	}
	static T LengthSquared( const grinliz::Vector2<T>& a, const grinliz::Vector2<T>& b ) {
		T x0 = Min( a.x, b.x );
		T x1 = Max( a.x, b.x );

		T dx = x1 - x0;
		// but wait, dx can't be greater than 1/2 the size (it is a cylinder)
		if ( dx > GEO_MAP_X/2 ) {
			// try the other way
			dx = (x0+GEO_MAP_X)-x1;
		}

		T dy = a.y - b.y;
		return dx*dx + dy*dy;
	}

	static float Length( const grinliz::Vector2<T>& a, const grinliz::Vector2<T>& b ) {
		return sqrtf( LengthSquared( a, b ) );
	}
};


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
	CArray< grinliz::Vector2I, MAX_BASES > base;
};


class Chit
{
protected:
	Chit( SpaceTree* tree );
public:
	virtual ~Chit();

	enum {
		MSG_NONE,
		MSG_DONE,						// delete this chit, left orbit, time up

		MSG_UFO_AT_DESTINATION,			// create a crop circle, attack a city, occupy a capital
		MSG_CROP_CIRCLE_COMPLETE,
		MSG_CITY_ATTACK_COMPLETE,
		MSG_BASE_ATTACK_COMPLETE,
		MSG_UFO_CRASHED,
	};
	virtual int DoTick( U32 deltaTime ) = 0;

	virtual bool Parked() const { return false; }

	grinliz::Vector2I MapPos() const { 
		grinliz::Vector2I v = { (int)pos.x, (int)pos.y }; 
		return Cylinder<int>::Normalize( v );
	}
	void SetMapPos( int x, int y ) 
	{ 
		GLASSERT( x > -GEO_MAP_X && x < GEO_MAP_X*3 );
		pos.Set( (float)x+0.5f, (float)y+0.5f );
		pos = Cylinder<float>::Normalize( pos );
	}
	
	const grinliz::Vector2F Pos() const { return Cylinder<float>::Normalize( pos ); }
	void SetPos( float x, float y ) { 
		GLASSERT( x > -GEO_MAP_X && x < GEO_MAP_X*3 );
		pos.Set( x, y ); 
	}

	virtual BaseChit*	IsBaseChit()	{ return 0; }
	virtual UFOChit*	IsUFOChit()		{ return 0; }
	void SetDestroyed()					{ destroyed = true; }
	bool IsDestroyed()					{ return destroyed; }

protected:
	grinliz::Vector2F pos;		// in map units - NOT normalized
	grinliz::Vector2F dest;		// in map units - NOT normalized
	Model* model[2];			// use 2 models, so that the camera can fake scrolling. the spacetree will sort it all out.
	SpaceTree* tree;

private:
	bool destroyed;
};


// FIXME: move to memory pools?
class UFOChit : public Chit
{
public:
	enum {
		AI_NONE,		// no ai, needs processing
		AI_TRAVELLING,	// on route to destination
		AI_ORBIT,		// accelerating to orbit

		AI_CRASHED,		// crashed UFO
		AI_CITY_ATTACK,	// attacking a city
		AI_BASE_ATTACK, 
		AI_OCCUPATION,	// a battleship has occupied a capital. It will not leave (until destroyed)
		AI_CROP_CIRCLE,	// chillin' and making crop circles

		AI_PARKED = AI_CRASHED,
	};

	enum {
		SCOUT,
		FRIGATE,
		BATTLESHIP,
		NUM_TYPES
	};
	UFOChit(	SpaceTree* tree, 
				int type, 
				const grinliz::Vector2F& start, 
				const grinliz::Vector2F& dest );
	~UFOChit();

	virtual int DoTick( U32 deltaTime );

	virtual UFOChit* IsUFOChit()		{ return this; }
	
	virtual bool Parked() const			{ return ai >= AI_PARKED; }
	bool Flying() const					{ return ai == AI_TRAVELLING || ai == AI_ORBIT; }
	int Type() const					{ return type; }

	void SetAI( int _ai ); 
	int AI() const						{ return ai; }
	
	float Speed() const					{ return speed; }
	grinliz::Vector2F Velocity() const;
	float Radius() const;
	void DoDamage( float d );

private:
	void EmitEntryExitBurn( U32 deltaTime, const grinliz::Vector3F& p0, const grinliz::Vector3F& p1, bool entry );
	void Decal( U32 timer, float speed, int id );
	void RemoveDecal();

	int type;
	int ai;
	float speed;
	float hp;
	
	U32 effectTimer;
	U32 jobTimer;

	Model* decal[2];
};


class CropCircle : public Chit
{
public:
	CropCircle( SpaceTree* tree, const grinliz::Vector2I& pos, U32 seed );
	~CropCircle();

	virtual int DoTick( U32 deltaTime );

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

	virtual int DoTick( U32 deltaTime )	{ return MSG_NONE; }

private:
	bool capital;
};


class BaseChit : public Chit 
{
public:
	BaseChit( SpaceTree* tree, const grinliz::Vector2I& pos );
	~BaseChit();

	virtual BaseChit* IsBaseChit()			{ return this; }
	virtual int DoTick(  U32 deltaTime )	{ return MSG_NONE; }

	// 0 short range, 1 long range
	float MissileRange( const int type ) const	{ 
		return ( type == 0 ) ? (float)GUN_LIFETIME * GUN_SPEED / 1000.0f : (float)MISSILE_LIFETIME * MISSILE_SPEED / 1000.0f; 
	}

private:
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

	BaseChit* IsBaseLocation( int x, int y );
	void CalcBaseChits( BaseChit* array[MAX_BASES] );	// fills an array with all the base chits.

	GeoMap* geoMap;
	SpaceTree* tree;
	GeoAI*	geoAI;

	grinliz::Vector2F	dragStart, dragLast;
	U32					lastAlienTime;
	U32					timeBetweenAliens;
	U32					missileTimer[2];

	grinliz::Random		random;

	gamui::PushButton	helpButton, researchButton;
	gamui::ToggleButton baseButton;

	AreaWidget*			areaWidget[GEO_REGIONS];
	RegionData regionData[GEO_REGIONS];
	CDynArray< Chit* >	chitArr;
	CDynArray< Missile > missileArr;
	GeoMapData			geoMapData;
};





#endif // UFO_ATTACK_GEO_SCENE_INCLUDED