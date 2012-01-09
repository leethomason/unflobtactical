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

#ifndef PARTICLE_EFFECT_INCLUDED
#define PARTICLE_EFFECT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glcolor.h"
#include "vertex.h"
#include "ufoutil.h"

class ParticleSystem;


// Never hold on to ParticleEffect pointers! They are fire and forget. If 
// you need to make sure an effect ends, use the ID(), and RemoveByID();
class ParticleEffect
{
public:
	ParticleEffect( ParticleSystem* system ) 
		: particleSystem( system ) 			
	{}
	virtual ~ParticleEffect()	{}

	virtual bool Done() = 0;
	virtual void DoTick( U32 time, U32 deltaTime ) = 0;
	virtual void Draw( const grinliz::Vector3F* eyeDir ) {}	// mostly particles are created in DoTick; however, if the Effect actually draws, do it here.
	virtual U32 CalcDuration() = 0;						//< can return 0 if no time, or U32(-1) for ongoing (controlled by creator)
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
	void SetColor( const grinliz::Color4F& c )	{ color = c; }
	void SetWidth( float w )			{ width = w; }
	void SetLength( float len )			{ boltLength = len; }

	virtual bool Done();
	virtual void DoTick( U32 time, U32 deltaTime );
	virtual U32 CalcDuration();			// duration until the next thing should happen.
	virtual const char* Name();
	virtual void Draw( const grinliz::Vector3F* eyeDir );

private:
	grinliz::Vector3F p0;		// origin location
	float d0, d1;				// current distance

	U32 startTime;
	float speed;
	grinliz::Color4F color;
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

	void SetColor( const grinliz::Color4F& c )	{ color = c; }
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
	grinliz::Color4F color;
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

	void SetSpeed( float s )				{ speed = s; }
	void SetColor( const grinliz::Color4F& c )		{ color = c; }
	void SetQuad( int start, int count )	{ quadStart = start; quadCount = count; }

	virtual bool Done();
	virtual void DoTick( U32 time, U32 deltaTime );
	virtual U32 CalcDuration();
	virtual const char* Name();

private:
	grinliz::Vector3F p0;
	float speed;
	grinliz::Color4F color;
	bool done;
	grinliz::Vector3F normal;
	int quadStart, quadCount;
	grinliz::Random random;

	float distance;		// distance from the p0 to p1

	U32 startTime;
	U32 currentTime;
	U32 timePool;
	U32 finishTime;
};


class RingEffect : public ParticleEffect
{
public:
	RingEffect( ParticleSystem* system );
	virtual ~RingEffect()	{}

	virtual void Clear();
	void Init(	const grinliz::Vector3F& origin,
				float radius,
				float halfsize,
				int count );

	void SetColor( const grinliz::Color4F& c )		{ color = c; }

	virtual bool Done();
	virtual void DoTick( U32 time, U32 deltaTime );
	virtual U32 CalcDuration()	{ return 0; }
	virtual void Draw( const grinliz::Vector3F* eyeDir );
	virtual const char* Name();

private:
	grinliz::Vector3F p0;
	float radius;
	grinliz::Color4F color;
	int count;
	bool done;

	struct RingVertex
	{
		grinliz::Vector3F	pos;
		grinliz::Vector2F	tex;
	};

	CDynArray< RingVertex > quads;
	CDynArray< U16 > index;
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