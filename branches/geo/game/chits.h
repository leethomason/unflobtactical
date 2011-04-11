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
class CityChit;


template < class T > 
class Cylinder
{
public:
	enum {
		GEO_MAP_X = 20		// copy!
	};
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

	static void ShortestPath( const grinliz::Vector2<T>& _a, const grinliz::Vector2<T>& _b, grinliz::Vector2<T>* delta )
	{
		grinliz::Vector2<T> a = _a;
		grinliz::Vector2<T> b = _b;
		grinliz::Vector2<T> d0 = b - a;
		if ( a.x < b.x )
			a.x += GEO_MAP_X;
		else
			b.x += GEO_MAP_X;
		grinliz::Vector2<T> d1 = b - a;

		*delta = ( d0.LengthSquared() <= d1.LengthSquared() ) ? d0 : d1;
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
	friend class ChitBag;
	Chit* next;
	Chit* prev;
	ChitBag* chitBag;

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
		MSG_UFO_CRASHED,

#ifndef IMMEDIATE_BUY
		MSG_CARGO_ARRIVED,
#endif
		MSG_LANDER_ARRIVED,
	};
	virtual int DoTick( U32 deltaTime )		{ return MSG_NONE; }

	virtual bool Parked() const				{ return false; }

	grinliz::Vector2I MapPos() const { 
		grinliz::Vector2I v = { (int)pos.x, (int)pos.y }; 
		return Cylinder<int>::Normalize( v );
	}
	void SetMapPos( int x, int y );
	void SetMapPos( const grinliz::Vector2I& pos )	{ SetMapPos( pos.x, pos.y ); }
	
	const grinliz::Vector2F Pos() const { return Cylinder<float>::Normalize( pos ); }
	void SetPos( float x, float y );

	virtual BaseChit*	IsBaseChit()	{ return 0; }
	virtual UFOChit*	IsUFOChit()		{ return 0; }
	virtual CargoChit*  IsCargoChit()	{ return 0; }
	virtual CityChit*	IsCityChit()	{ return 0; }

	void SetDestroyed()					{ destroyed = true; }
	bool IsDestroyed()					{ return destroyed; }
	int ID()							{ return id; }
	Chit* Next()						{ return next; }

	virtual void Save( FILE* fp, int depth );
	virtual void Load( const TiXmlElement* doc );

protected:
	grinliz::Vector2F pos;		// in map units - NOT normalized
	Model* model[2];			// use 2 models, so that the camera can fake scrolling. the spacetree will sort it all out.
	SpaceTree* tree;

private:
	bool destroyed;
	int id;
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
		AI_OCCUPATION,	// a battleship has occupied a capital. It will not leave (until destroyed)
		AI_CROP_CIRCLE,	// chillin' and making crop circles

		AI_PARKED,
	};

	enum {
		SCOUT,
		FRIGATE,
		BATTLESHIP,
		BASE,
		NUM_TYPES
	};
	UFOChit(	SpaceTree* tree, 
				int type, 
				const grinliz::Vector2F& start, 
				const grinliz::Vector2F& dest );
	~UFOChit();

	virtual int DoTick( U32 deltaTime );

	virtual UFOChit* IsUFOChit()		{ return this; }
	
	virtual bool Parked() const			{ return ai >= AI_CRASHED; }
	bool Flying() const					{ return ai == AI_TRAVELLING || ai == AI_ORBIT; }
	int Type() const					{ return type; }
	bool CanSendLander( bool battleshipTech ) const			
	{
		if ( type == BATTLESHIP && !battleshipTech )
			return false;
		if ( ai == AI_CRASHED || ai == AI_CITY_ATTACK || ai == AI_CROP_CIRCLE || ai == AI_PARKED )
			return true;
		return false;
	}

	void SetAI( int _ai ); 
	int AI() const						{ return ai; }
	
	float Speed() const					{ return speed; }
	grinliz::Vector2F Velocity() const;
	float Radius() const;
	void DoDamage( float d );

	virtual void Save( FILE* fp, int depth );
	virtual void Load( const TiXmlElement* doc );

private:
	void Init();

	void EmitEntryExitBurn( U32 deltaTime, const grinliz::Vector3F& p0, const grinliz::Vector3F& p1, bool entry );
	void Decal( U32 timer, float speed, int id );
	void RemoveDecal();

	grinliz::Vector2F dest;
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

	virtual void Save( FILE* fp, int depth );
	virtual void Load( const TiXmlElement* doc );

private:
	void Init( U32 seed );
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

	virtual void Save( FILE* fp, int depth );
	virtual void Load( const TiXmlElement* doc );

	virtual CityChit*	IsCityChit()	{ return this; }

private:
	void Init();
	bool capital;
};


class BaseChit : public Chit 
{
public:
	enum {
		FACILITY_LANDER,
		FACILITY_MISSILE,
		FACILITY_CARGO,
		FACILITY_GUN,
		FACILITY_SCILAB,
		NUM_FACILITIES,

		BUILD_TIME = 5,	// in seconds
	};

	BaseChit( SpaceTree* tree, const grinliz::Vector2I& pos, int index, const ItemDefArr& _itemDefArr, bool firstBase );
	~BaseChit();

	virtual BaseChit* IsBaseChit()			{ return this; }
	virtual int DoTick(  U32 deltaTime );

	// 0 short range, 1 long range
	float MissileRange( const int type ) const;
	int Index() const { return index; }
	const char* Name() const;

	Storage* GetStorage() { return storage; }
	
	Unit* GetUnits() { return units; }
	int NumUnits();

	int NumScientists()					{ return nScientists; }
	void SetNumScientists( int m )		{ nScientists = m; }
	int* GetScientstPtr()				{ return &nScientists; }

	bool IsFacilityComplete( int i )	const	{ return facilityStatus[i] == 0; }
	bool IsFacilityInProgress( int i )			{ return facilityStatus[i] > 0; }
	void BuildFacility( int i )					
	{ 
		GLASSERT( facilityStatus[i] < 0 ); 
#ifdef IMMEDIATE_BUY
		facilityStatus[i] = 0;
#else
		facilityStatus[i] = BUILD_TIME*1000; 
#endif
	}

	bool CanUseSoldiers() const {
		return IsFacilityComplete( FACILITY_CARGO ) && IsFacilityComplete( FACILITY_LANDER );
	}
	bool CanUseScientists() const {
		return IsFacilityComplete( FACILITY_CARGO ) && IsFacilityComplete( FACILITY_SCILAB );
	}

	virtual void Save( FILE* fp, int depth );
	virtual void Load( const TiXmlElement* doc, Game* game );

private:
	void Init();
	int index;
	int nScientists;

	Storage* storage;
	int facilityStatus[NUM_FACILITIES];	// -1, does not exist. 0, complete. >0 in progress
	Unit units[MAX_TERRANS];
};


class CargoChit : public Chit
{
public:
	enum {
		TYPE_CARGO,
		TYPE_LANDER
	};
	CargoChit( SpaceTree* tree, int type, const grinliz::Vector2I& start, const grinliz::Vector2I& end );
	~CargoChit();

	virtual int DoTick( U32 deltaTime );
	virtual CargoChit* IsCargoChit() { return this; }
	int Type() const { return type; }
	void CheckDest( const ChitBag& chitBag );	// make sure we're going somewhere...

	virtual void Save( FILE* fp, int depth );
	virtual void Load( const TiXmlElement* doc );

	const grinliz::Vector2I& Dest() const { return dest; }
	const grinliz::Vector2I& Origin() const { return origin; }

private:
	void Init();

	int type;
	bool outbound;
	grinliz::Vector2I origin;
	grinliz::Vector2I dest;
};


class ChitBag
{
public:
	ChitBag();
	~ChitBag();
	void Clear();

	void Add( Chit* );
	void Remove( Chit* );	// called automatically for 'delete chit'
	void Clean();			// delete destroyed chits

	Chit* Begin() 	{ return sentinel.next; }
	Chit* End() 	{ return &sentinel; }

	Chit* GetChit( int id ) { 
		for( Chit* it=Begin(); it != End(); it=it->Next() ) {
			if ( it->ID() == id ) {
				return it;
			}
		}
		return 0;
	}

	int AllocBaseChitIndex();

	Chit*		GetChit( const grinliz::Vector2I& pos );
	BaseChit*	GetBaseChit( const char* name );
	BaseChit*	GetBaseChit( int i ) const { GLASSERT( i>=0 && i<MAX_BASES ); return baseChitArr[i]; }
	BaseChit*	GetBaseChitAt( const grinliz::Vector2I& pos );
	UFOChit*	GetLandedUFOChitAt( const grinliz::Vector2I& pos ) const;
	int			NumBaseChits();

	CargoChit*	GetCargoGoingTo( int type, const grinliz::Vector2I& to );
	CargoChit*	GetCargoComingFrom( int type, const grinliz::Vector2I& from );
	Chit*		GetParkedChitAt( const grinliz::Vector2I& pos ) const;

	void SetBattle( int ufoID, int landerID, int scenario )	{ this->battleUFOID = ufoID; this->battleLanderID = landerID; this->battleScenario = scenario; }
	UFOChit*	GetBattleUFO()					{ Chit* chit = GetChit( battleUFOID ); return (chit) ? chit->IsUFOChit() : 0; }
	CargoChit*	GetBattleLander()				{ Chit* chit = GetChit( battleLanderID ); return (chit) ? chit->IsCargoChit() : 0; }
	UFOChit*	GetUFOBase();
	int         GetBattleScenario() const		{ return battleScenario; }

	// Length is MAX_BASES. Alpha, Bravo, Charlie, Delta
	enum {
		MAX_BASES = 4
	};

	virtual void Save( FILE* fp, int depth );
	virtual void Load( const TiXmlElement* doc, SpaceTree* tree, const ItemDefArr& arr, Game* game );

private:
	int idPool;
	int battleUFOID;
	int battleLanderID;
	int battleScenario;
	Chit sentinel;
	BaseChit* baseChitArr[MAX_BASES];
};

#endif // GEO_CHITS_INCLUDED