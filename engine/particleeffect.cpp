#include "particleeffect.h"
#include "particle.h"
using namespace grinliz;


BoltEffect::BoltEffect( ParticleSystem* system ) : ParticleEffect( system )
{
	Clear();
}


void BoltEffect::Clear()
{
	p0.Set( 0, 0, 0 );
	startTime = 0;
	speed = 0.01f;
	color.Set( 1, 1, 1, 1.0f );
	width = 0.2f;
	boltLength = 1.0f;
	done = true;
}


void BoltEffect::Init(	const grinliz::Vector3F& _p0,
						const grinliz::Vector3F& _p1,
						U32 _startTime )
{
	p0 = _p0;
	startTime = _startTime;
	
	normal = _p1 - _p0;
	distance = normal.Length();
	normal.Normalize();
	done = false;
}


U32 BoltEffect::CalcDuration()
{
	float t = distance / speed;	// time until impact
	return LRintf( t );
}


bool BoltEffect::Done()
{
	return done;
}


void BoltEffect::DoTick( U32 time, U32 deltaTime )
{
	if ( time > startTime ) {
		float delta = (float)(time-startTime);	
		float d0 = delta*speed-boltLength;
		float d1 = delta*speed;

		if ( d0 >= distance ) {
			done = true;
		}
		else {
			if ( d0 < 0.0f )
				d0 = 0.0f;
			if ( d1 > distance )
				d1 = distance;
			if ( d1 > d0 ) {
				Vector3F q0 = p0 + normal*d0;
				Vector3F q1 = p0 + normal*d1;
				Color4F cvel = { 0, 0, 0, 0 };

				particleSystem->EmitBeam( color, cvel, q0, q1, width, 0 );
			}
		}
	}
}


const char* BoltEffect::Name()
{
	return "BoltEffect";
}


ImpactEffect::ImpactEffect( ParticleSystem* system ) : ParticleEffect( system )
{
	Clear();
}


void ImpactEffect::Clear()
{
	normal.Set( 0, 0, 0 );
	radius = 2.0f;
	color.Set( 1, 1, 1, 1 );
	done = true;
	config = ParticleSystem::PARTICLE_HEMISPHERE;
}


void ImpactEffect::Init(	const grinliz::Vector3F& p0,
							U32 startTime )
{
	this->p0 = p0;
	this->normal = normal;
	this->color = color;
	this->radius = radius;
	this->startTime = startTime;
	this->done = false;
}


bool ImpactEffect::Done()
{
	return done;
}


void ImpactEffect::DoTick( U32 time, U32 deltaTime )
{
	if ( time >= startTime ) {
		Color4F cvel = { 0, 0, 0, 0 };

		const float VEL = 2.0f;
		Vector3F vel = { normal.x*VEL, normal.y*VEL, normal.z*VEL };
		float life = radius / VEL * 1000.0f;

		particleSystem->EmitPoint(  40, config, 
									color, cvel, 
									p0, 0, 
									vel, 0.1f, 
									(U32)life );
		done = true;
	}
}


U32 ImpactEffect::CalcDuration()
{
	return 0;
}


const char* ImpactEffect::Name() 
{
	return "ImpactEffect";
}


SmokeTrailEffect::SmokeTrailEffect( ParticleSystem* system ) : ParticleEffect( system )
{
	Clear();
}


void SmokeTrailEffect::Clear()
{
	p0.Set( 0, 0, 0 );
	startTime = 0;
	speed = 0.01f;
	color.Set( 1, 1, 1, 1 );
	done = true;
	normal.Set( 0, 0, 0 );
}


void SmokeTrailEffect::Init(	const grinliz::Vector3F& _p0,
								const grinliz::Vector3F& _p1,
								U32 _startTime )
{
	p0 = _p0;
	startTime = _startTime;
	currentTime = startTime;
	timePool = 0;

	done = false;
	normal = _p1 - _p0;
	distance = normal.Length();
	normal.Normalize();

	U32 duration = (U32)(distance/speed);
	finishTime = currentTime + duration;
}


bool SmokeTrailEffect::Done()
{
	return done;
}

void SmokeTrailEffect::DoTick( U32 time, U32 deltaTime )
{
	const U32 INC = 70;
	
	timePool += (time-currentTime);
	currentTime = time;
	
	while ( timePool >= INC ) {
		Vector3F p = p0 + normal * ((float)( time - startTime - timePool) * speed);
		Color4F cvel = { 0, 0, 0, -0.4f };
		Vector3F vel = { 0, 0, 0 };

		particleSystem->EmitQuad(	ParticleSystem::SMOKE,
									color,
									cvel,
									p, 0.1f,
									vel, 0, 0.2f, 0.2f,
									3000 );
		timePool -= INC;
	}
	if ( time >= finishTime ) {
		done = true;
	}
}

U32 SmokeTrailEffect::CalcDuration()
{
	return finishTime-startTime;
}


const char* SmokeTrailEffect::Name()
{
	return "SmokeTrailEffect";
}

/*
SustainedPyroEffect::SustainedPyroEffect( ParticleSystem* system ) : ParticleEffect( system )
{
	Clear();
}


void SustainedPyroEffect::Init( const grinliz::Vector3F& p )
{
	pos = p;
}


void SustainedPyroEffect::Clear()
{
	pos.Set( 0, 0, 0 );
	useFire = true;
	done = false;
}


void SustainedPyroEffect::DoTick( U32 time, U32 delta )
{
	if ( useFire ) 
		particleSystem->EmitFlame( delta, pos );
	else
		particleSystem->EmitSmoke( delta, pos );
}	


U32 SustainedPyroEffect::CalcDuration()
{
	return (U32)(-1);
}


const char* SustainedPyroEffect::Name()
{
	return "SustainedPyroEffect";
}


bool SustainedPyroEffect::Done()
{
	return done;
}
*/
