#ifndef GEO_CHITS_INCLUDED
#define GEO_CHITS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"

#include "unit.h"

class SpaceTree;
class BaseChit;
class UFOChit;
class CargoChit;
class Model;
class Storage;
class ItemDefArr;


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

		MSG_CARGO_ARRIVED,
	};
	virtual int DoTick( U32 deltaTime ) = 0;

	virtual bool Parked() const { return false; }

	grinliz::Vector2I MapPos() const { 
		grinliz::Vector2I v = { (int)pos.x, (int)pos.y }; 
		return Cylinder<int>::Normalize( v );
	}
	void SetMapPos( int x, int y );
	
	const grinliz::Vector2F Pos() const { return Cylinder<float>::Normalize( pos ); }
	void SetPos( float x, float y );

	virtual BaseChit*	IsBaseChit()	{ return 0; }
	virtual UFOChit*	IsUFOChit()		{ return 0; }
	virtual CargoChit*  IsCargoChit()	{ return 0; }

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
	BaseChit( SpaceTree* tree, const grinliz::Vector2I& pos, int id, const ItemDefArr& _itemDefArr );
	~BaseChit();

	virtual BaseChit* IsBaseChit()			{ return this; }
	virtual int DoTick(  U32 deltaTime )	{ return MSG_NONE; }

	// 0 short range, 1 long range
	float MissileRange( const int type ) const;
	int ID() const { return id; }

	Storage* GetStorage() { return storage; }
	
	Unit* GetUnits() { return units; }
	int NumUnits() { return MAX_TERRANS; }	// fixme

private:
	int id;
	Storage* storage;
	Unit units[MAX_TERRANS];
};


class CargoChit : public Chit
{
public:
	CargoChit( SpaceTree* tree, const grinliz::Vector2I& start, const grinliz::Vector2I& end );
	~CargoChit();

	virtual int DoTick( U32 deltaTime );
	virtual CargoChit* IsCargoChit() { return this; }

	grinliz::Vector2I Base() { return baseInMap; }

private:
	grinliz::Vector2F city;
	grinliz::Vector2F base;
	bool goingToBase;
	grinliz::Vector2I cityInMap;
	grinliz::Vector2I baseInMap;
};

#endif // GEO_CHITS_INCLUDED