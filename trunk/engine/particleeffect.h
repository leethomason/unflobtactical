#ifndef PARTICLE_EFFECT_INCLUDED
#define PARTICLE_EFFECT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "vertex.h"

class ParticleSystem;

class ParticleEffect
{
public:
	ParticleEffect( ParticleSystem* system ) 
		: particleSystem( system ) 			
	{}
	virtual ~ParticleEffect()	{}

	virtual bool Done() = 0;
	virtual void DoTick( U32 time, U32 deltaTime ) = 0;
	virtual U32 CalcDuration() = 0;		//< can return 0 if no time, or U32(-1) for ongoing (controlled by creator)
	virtual const char* Name() = 0;
	virtual void Clear() = 0;

protected:
	ParticleSystem* particleSystem;
};

class BoltEffect : public ParticleEffect
{
public:
	BoltEffect( ParticleSystem* system );
	virtual ~BoltEffect()	{}

	virtual void Clear();
	void Init(	const grinliz::Vector3F& p0,
				const grinliz::Vector3F& p1,
				U32 startTime );


	void SetSpeed( float s )			{ speed = s; }
	void SetColor( const Color4F& c )	{ color = c; }
	void SetWidth( float w )			{ width = w; }
	void SetLength( float len )			{ boltLength = len; }

	virtual bool Done();
	virtual void DoTick( U32 time, U32 deltaTime );
	virtual U32 CalcDuration();			// duration until the next thing should happen.
	virtual const char* Name();

private:
	grinliz::Vector3F p0;
	U32 startTime;
	float speed;
	Color4F color;
	float width;
	float boltLength;	// length of the bolt
	bool done;

	float distance;		// distance from the p0 to p1
	grinliz::Vector3F normal;
};


class ImpactEffect : public ParticleEffect
{
public:
	ImpactEffect( ParticleSystem* system );
	virtual ~ImpactEffect()	{}

	virtual void Clear();
	void Init(	const grinliz::Vector3F& p0,
				U32 startTime );

	void SetColor( const Color4F& c )	{ color = c; }
	void SetRadius( float r )			{ radius = r; }
	void SetNormal( const grinliz::Vector3F& n ) { normal = n; }
	// PARTICLE_HEMISPHERE is default
	void SetConfig( int c )				{ config = c; }

	virtual bool Done();
	virtual void DoTick( U32 time, U32 deltaTime );
	virtual U32 CalcDuration();
	virtual const char* Name();

private:
	grinliz::Vector3F normal;
	float radius;
	Color4F color;
	int config;
	bool done;

	grinliz::Vector3F p0;
	U32 startTime;
};


class SmokeTrailEffect : public ParticleEffect
{
public:
	SmokeTrailEffect( ParticleSystem* system );
	virtual ~SmokeTrailEffect()	{}

	virtual void Clear();
	void Init(	const grinliz::Vector3F& p0,
				const grinliz::Vector3F& p1,
				U32 startTime );

	void SetSpeed( float s )			{ speed = s; }
	void SetColor( const Color4F& c )	{ color = c; }

	virtual bool Done();
	virtual void DoTick( U32 time, U32 deltaTime );
	virtual U32 CalcDuration();
	virtual const char* Name();

private:
	grinliz::Vector3F p0;
	float speed;
	Color4F color;
	bool done;
	grinliz::Vector3F normal;

	float distance;		// distance from the p0 to p1

	U32 startTime;
	U32 currentTime;
	U32 timePool;
	U32 finishTime;
};


/* Problematic class: bad to keep pointer to a system that can delete the object at any time.
class SustainedPyroEffect : public ParticleEffect
{
public:
	SustainedPyroEffect( ParticleSystem* system );
	virtual ~SustainedPyroEffect()	{}

	void Init(	const grinliz::Vector3F& p0 );

	void SetFire( bool enable )	{ useFire = enable; }
	bool IsFire()				{ return useFire; }
	void SetDone()				{ done = true; }
	const grinliz::Vector3F& Pos()	{ return pos; }

	virtual void Clear();
	virtual bool Done();
	virtual void DoTick( U32 time, U32 deltaTime );
	virtual U32 CalcDuration();
	virtual const char* Name();

	U32 duration;	// user value

private:
	grinliz::Vector3F pos;
	bool useFire;
	bool done;
};
*/

#endif // PARTICLE	_EFFECT_INCLUDED