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

#include "particleeffect.h"
#include "particle.h"
#include "camera.h"
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
	d0 = d1 = 0;
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
		d0 = delta*speed-boltLength;
		d1 = delta*speed;

		if ( d0 >= distance ) {
			done = true;
		}
		else {
			if ( d0 < 0.0f )
				d0 = 0.0f;
			if ( d1 > distance )
				d1 = distance;
		}
	}
}


void BoltEffect::Draw( const Vector3F* eyeDir )
{
	if (    d1 > d0
		 && !done ) 
	{
		Vector3F q0 = p0 + normal*d0;
		Vector3F q1 = p0 + normal*d1;

		Vector3F right;
		CrossProduct( eyeDir[Camera::NORMAL], q1-q0, &right );
		right.Normalize();

		float halfWidth = width*0.5f;

		// FIXME: hardcoded texture coordinates
		static const float tx = 0.50f;
		static const float ty = 0.0f;
		PTVertex pV[4];

		pV[0].pos = q0 - right*halfWidth;
		pV[0].tex.Set( tx+0.0f, ty+0.0f );
			
		pV[1].pos = q0 + right*halfWidth;
		pV[1].tex.Set( tx+0.25f, ty+0.0f );

		pV[2].pos = q1 + right*halfWidth;
		pV[2].tex.Set( tx+0.25f, ty+0.25f );

		pV[3].pos = q1 - right*halfWidth;
		pV[3].tex.Set( tx+0.0f, ty+0.25f );

		static const U16 index[6] = { 0, 1, 2, 0, 2, 3 };

		QuadParticleShader shader;
		shader.SetTexture0( TextureManager::Instance()->GetTexture( "particleQuad" ) );

		GPUShader::Stream stream;
		stream.stride = sizeof( pV[0] );
		stream.nPos = 3;
		stream.posOffset = 0;
		stream.nTexture0 = 2;
		stream.texture0Offset = 12;
		shader.SetColor( color );

		shader.SetStream( stream, pV, 6, index );
		shader.Draw();
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
		Color4F cvel = { 0, 0, 0, -0.50f };

		const float VEL = 2.0f;
		Vector3F vel = { normal.x*VEL, normal.y*VEL, normal.z*VEL };
		//float life = radius / VEL * 1000.0f;

		particleSystem->EmitPoint(  40, config, 
									color, cvel, 
									p0, 0, 
									vel, 0.1f );
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
	quadStart = ParticleSystem::SMOKE;
	quadCount = 2;
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

		particleSystem->EmitQuad(	quadStart + random.Rand( quadCount ),
									color,
									cvel,
									p, 0.1f,
									vel, 0, 0.2f, 0.2f );
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


RingEffect::RingEffect( ParticleSystem* system ) : ParticleEffect( system )
{
	Clear();
}


void RingEffect::Clear()
{
	p0.Set( 0, 0, 0 );
	radius = 1.0f;
	color.Set( 1, 1, 1, 1 );
	count = 1;
	done = true;

	quads.Clear();
	index.Clear();
}


void RingEffect::Init(	const grinliz::Vector3F& origin,
						float radius,
						float halfSize,
						int count )
{
	p0 = origin;
	this->radius = radius;
	this->count = count;

	done = false;

	for( int i=0; i<count; ++i ) {
		float theta = TWO_PI * (float)i / (float)count;
		float x = cosf( theta )*radius + origin.x;
		float z = sinf( theta )*radius + origin.z;

		int base = quads.Size();
		RingVertex* vertex = quads.Push();
		vertex->pos.Set( x-halfSize, p0.y, z-halfSize );
		vertex->tex.Set( 0, 0 );

		vertex = quads.Push();
		vertex->pos.Set( x+halfSize, p0.y, z-halfSize );
		vertex->tex.Set( 1, 0 );

		vertex = quads.Push();
		vertex->pos.Set( x+halfSize, p0.y, z+halfSize );
		vertex->tex.Set( 1, 1 );

		vertex = quads.Push();
		vertex->pos.Set( x-halfSize, p0.y, z+halfSize );
		vertex->tex.Set( 0, 1 );

		U16* idx = index.PushArr( 6 );
		idx[0] = base+0;
		idx[1] = base+2;
		idx[2] = base+1;
		idx[3] = base+0;
		idx[4] = base+3;
		idx[5] = base+2;
	}
}


bool RingEffect::Done()
{
	return done;
}

void RingEffect::DoTick( U32 time, U32 deltaTime )
{
}


const char* RingEffect::Name()
{
	return "RingEffect";
}


void RingEffect::Draw( const Vector3F* eyeDir )
{
	if ( !done && !quads.Empty() && !index.Empty() ) 
	{
		QuadParticleShader shader;
		GPUShader::Stream stream;

		stream.stride = sizeof( RingVertex );
		stream.nPos = 3;
		stream.posOffset = 0;
		stream.nTexture0 = 2;
		stream.texture0Offset = 12;

		shader.SetColor( color );
		shader.SetTexture0( TextureManager::Instance()->GetTexture( "particleSparkle" ) );

		shader.SetStream( stream, quads.Mem(), index.Size(), index.Mem() );
		shader.Draw();
	}
}
