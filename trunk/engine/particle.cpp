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

#include "particle.h"
#include "../grinliz/glgeometry.h"
#include "platformgl.h"
#include "surface.h"
#include "camera.h"

using namespace grinliz;

ParticleSystem::ParticleSystem()
{
	for( int i=0; i<NUM_PRIMITIVES; ++i ) {
		nParticles[i] = 0;
		particleTypeArr[i].Init( 0, 1.0f );
	}
	nDecals = 0;
}


ParticleSystem::~ParticleSystem()
{
}


void ParticleSystem::InitPoint( const Texture* texture )
{
	particleTypeArr[POINT].Init( texture, 4.0f );
}


void ParticleSystem::InitQuad( const Texture* texture )
{
	particleTypeArr[QUAD].Init( texture, 1.0f );
}


void ParticleSystem::Clear()
{
	for( int i=0; i<NUM_PRIMITIVES; ++i ) {
		nParticles[i] = 0;
	}
	nDecals = 0;
}


void ParticleSystem::Update( U32 msec )
{
	float sec = (float)msec / 1000.0f;

	for( int i=0; i<NUM_PRIMITIVES; ++i ) {
		Particle* pStart	= 0;
		if ( i==POINT )
			pStart = pointBuffer;
		else if ( i==QUAD )
			pStart = quadBuffer;
		GLASSERT( pStart );
		Particle* p = pStart;
			
		Particle* pEnd	= pStart + nParticles[i];

		while( p < pEnd ) {
			p->age += msec;
			if ( p->age > p->lifetime ) {
				// swap with end.
				grinliz::Swap( p, pEnd-1);
				--pEnd;
				continue;
			}
			// apply effect of time
			p->pos += p->vel*sec;
			p->color += p->colorVel*sec;
			++p;
		}
		nParticles[i] = pEnd - pStart;
	}
}


void ParticleSystem::Emit(	int primitive,					// POINT or QUAD
							int type,						// FIRE, SMOKE
							int count,						// number of particles to create
							int config,						// PARTICLE_RAY, etc.
							const Color4F& color,
							const Color4F& colorVelocity,
							const grinliz::Vector3F& pos,	// origin
							float posFuzz,
							const grinliz::Vector3F& vel,	// velocity
							float velFuzz,
							U32 lifetime )
{
	GLASSERT( primitive >= 0 && primitive < NUM_PRIMITIVES );
	GLASSERT( type >=0 && type < NUM_TYPE );
	GLASSERT( config >= 0 && config <= PARTICLE_SPHERE );

	const int MAX_PARTICLES[2] = { MAX_POINT_PARTICLES, MAX_QUAD_PARTICLES };
	Particle* particles[2] = { pointBuffer, quadBuffer };

	GLASSERT( count < MAX_PARTICLES[primitive] );
	lifetime = grinliz::Clamp( lifetime, (U32)16, (U32)0xffff );
	
	int insert = nParticles[primitive];
	if ( nParticles[primitive] + count > MAX_PARTICLES[primitive] ) {
		insert = MAX_PARTICLES[primitive] - count;
		nParticles[primitive] = MAX_PARTICLES[primitive];
	}
	else {
		nParticles[primitive] += count;
	}


	Vector3F posP, velP, normal;
	Color4F colP;
	U16 lifeP;
	const float len = vel.Length();
	normal = vel;
	normal.Normalize();

	for( int i=0; i<count; ++i ) {
		switch (config) {
			case PARTICLE_RAY:
				velP = vel;
				velP.x += velFuzz*(-0.5f + rand.Uniform());
				velP.y += velFuzz*(-0.5f + rand.Uniform());
				velP.z += velFuzz*(-0.5f + rand.Uniform());
				break;

			case PARTICLE_HEMISPHERE:
				rand.NormalVector( &velP.x, 3 );
				if ( DotProduct( velP, normal ) < 0.0f ) {
					velP = -velP;
				}
				velP.x = velP.x*len + velFuzz*(-0.5f + rand.Uniform());
				velP.y = velP.y*len + velFuzz*(-0.5f + rand.Uniform());
				velP.z = velP.z*len + velFuzz*(-0.5f + rand.Uniform());
				break;

			case PARTICLE_SPHERE:
				rand.NormalVector( &velP.x, 3 );
				velP.x = velP.x*len + velFuzz*(-0.5f + rand.Uniform());
				velP.y = velP.y*len + velFuzz*(-0.5f + rand.Uniform());
				velP.z = velP.z*len + velFuzz*(-0.5f + rand.Uniform());
				break;

			default:
				GLASSERT( 0 );
				break;
		};

		posP.x = pos.x + posFuzz*(-0.5f + rand.Uniform() );
		posP.y = pos.y + posFuzz*(-0.5f + rand.Uniform() );
		posP.z = pos.z + posFuzz*(-0.5f + rand.Uniform() );

		const int LFUZZ = 64;
		lifeP = (lifetime * ((256-LFUZZ) + (rand.Rand()&(LFUZZ-1)))) >> 8;

		const float CFUZZ_MUL = 0.90f;
		const float CFUZZ_ADD = 0.20f;

		for( int j=0; j<3; ++j ) {
			colP.X(j) = color.X(j)*CFUZZ_MUL + rand.Uniform()*CFUZZ_ADD;
		}
		colP.w = color.w;	// don't fuzz alpha

		GLASSERT( insert < MAX_PARTICLES[primitive] );
		Particle* p = particles[primitive] + insert;
		++insert;

		p->pos = posP;
		p->color = colP;
		p->vel = velP;
		p->colorVel = colorVelocity;
		p->age = 0;
		p->lifetime = lifeP;
		p->type = (U8)type;
		p->subType = (U8)(rand.Rand()>>30);
	}
}



void ParticleSystem::EmitDecal( int id, int flags, const grinliz::Vector3F& pos, float alpha, float rotation )
{
	GLASSERT( id >= 0 && id < NUM_DECAL );
	const Color4F color		= { 1.0f, 1.0f, 1.0f, 1.0f };

	if ( nDecals == MAX_DECALS ) {
		nDecals--;
	}
	decalBuffer[nDecals].pos = pos;
	decalBuffer[nDecals].color = color;
	decalBuffer[nDecals].color.w = alpha ;
	decalBuffer[nDecals].subType = id;
	decalBuffer[nDecals].rotation = rotation;
	decalBuffer[nDecals].flags = flags;
	nDecals++;
}


void ParticleSystem::EmitFlame( U32 delta, const Vector3F& _pos )
{
	// flame, smoke, particle
	U32 count[3];
	const U32 interval[3] = { 500, 600, 200 };

	// If the delta is 200, there is a 200/250 chance of it being in this delta
	for( int i=0; i<3; ++i ) {
		count[i] = delta / interval[i];
		U32 remain = delta - count[i]*interval[i];

		U32 v = rand.Rand() % interval[i];
		if ( v < remain ) {
			++count[i];
		}
	}

	
	// Flame
	{
		Vector3F pos = _pos;
		pos.y += 0.3f;

		const Color4F color		= { 1.0f, 1.0f, 1.0f, 1.0f };
		const Color4F colorVec	= { 0.0f, 0.0f, 0.0f, -0.4f };
		Vector3F velocity		= { 0.0f, 1.0f, 0.0f };

		if ( (rand.Rand()>>28)==0 ) {
			velocity.y = 0.1f;
		}

		for( U32 i=0; i<count[0]; ++i ) {
			Emit(	ParticleSystem::QUAD,
					ParticleSystem::FIRE,
					1,
					ParticleSystem::PARTICLE_RAY,
					color,		colorVec,
					pos,		0.4f,	
					velocity,	0.3f,
					2000 );		
		}
	}
	
	// Smoke
	{
		Vector3F pos = _pos;
		pos.y += 1.0f;

		const Color4F color		= { 0.5f, 0.5f, 0.5f, 1.0f };
		const Color4F colorVec	= { -0.1f, -0.1f, -0.1f, 0.0f };
		Vector3F velocity		= { 0.0f, 0.8f, 0.0f };

		for( U32 i=0; i<count[1]; ++i ) {
			Emit(	ParticleSystem::QUAD,
					ParticleSystem::SMOKE,
					1,
					ParticleSystem::PARTICLE_RAY,
					color,		colorVec,
					pos,		0.6f,	
					velocity,	0.2f,
					5000 );		
		}
	}
	
	// Sparkle
	{
		Vector3F pos = _pos;

		const Color4F color		= { 1.0f, 0.3f, 0.1f, 1.0f };
		const Color4F colorVec	= { 0.0f, -0.1f, -0.1f, -0.1f };
		Vector3F velocity		= { 0.0f, 1.5f, 0.0f };

		for( U32 i=0; i<count[2]; ++i ) {
			Emit(	ParticleSystem::POINT,
					0,
					1,
					ParticleSystem::PARTICLE_RAY,
					color,		colorVec,
					pos,		0.05f,	
					velocity,	0.3f,
					2000 );		
		}
	}
}


void ParticleSystem::Draw( const Vector3F* eyeDir )
{
	glEnable( GL_BLEND );
	glDepthMask( GL_FALSE );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	DrawPointParticles();

	DrawDecalParticles( DECAL_BOTTOM );
	glDisable( GL_DEPTH_TEST );
	DrawDecalParticles( DECAL_TOP );
	glEnable( GL_DEPTH_TEST );
	nDecals = 0;

	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	DrawQuadParticles( eyeDir );

	glDepthMask( GL_TRUE );
	glDisable( GL_BLEND );
}


void ParticleSystem::DrawPointParticles()
{
	CHECK_GL_ERROR;
	glEnableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );
	
	glEnable( GL_TEXTURE_2D );

#ifdef USING_GL	
	glEnable(GL_POINT_SPRITE);
	glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
#else
	glEnable(GL_POINT_SPRITE_OES);
	glTexEnvx(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
#endif
	CHECK_GL_ERROR;
	if ( nParticles[POINT] > 0 ) {
		glBindTexture( GL_TEXTURE_2D, particleTypeArr[POINT].texture->glID );
		//float maxSize = 40.0f;
		//glPointParameterfv( GL_POINT_SIZE_MAX, &maxSize );
		CHECK_GL_ERROR;

		glPointSize( particleTypeArr[POINT].size );
		CHECK_GL_ERROR;
		
		U8* vPtr = (U8*)pointBuffer + 0;
		U8* cPtr = (U8*)pointBuffer + 12;

		glVertexPointer( 3, GL_FLOAT, sizeof(Particle), vPtr );
		glColorPointer( 4, GL_FLOAT, sizeof(Particle), cPtr );
		CHECK_GL_ERROR;
		glDrawArrays( GL_POINTS, 0, nParticles[POINT] );
		CHECK_GL_ERROR;
	}

#ifdef USING_GL
	glDisable( GL_POINT_SPRITE );
#else
	glDisable( GL_POINT_SPRITE_OES );
#endif
	CHECK_GL_ERROR;
	
	// Restore standard state.
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	CHECK_GL_ERROR;
}


void ParticleSystem::DrawQuadParticles( const Vector3F* eyeDir )
{
	if ( nParticles[QUAD] == 0 ) {
		return;
	}

	QuadVertex base[4];

	base[0].pos = -eyeDir[Camera::RIGHT]*0.5f - eyeDir[Camera::UP]*0.5f;
	base[1].pos =  eyeDir[Camera::RIGHT]*0.5f - eyeDir[Camera::UP]*0.5f;
	base[2].pos =  eyeDir[Camera::RIGHT]*0.5f + eyeDir[Camera::UP]*0.5f;
	base[3].pos = -eyeDir[Camera::RIGHT]*0.5f + eyeDir[Camera::UP]*0.5f;

	const Vector2F tex[4] = {
		{ 0.0f, 0.0f },
		{ 0.25f, 0.0f },
		{ 0.25f, 0.25f },
		{ 0.0f, 0.25f }
	};

	int index = 0;
	int vertex = 0;
	GLASSERT( nParticles[QUAD] <= MAX_QUAD_PARTICLES );

	for( int i=0; i<nParticles[QUAD]; ++i ) 
	{
		const int type	  = quadBuffer[i].type;
		const int subType = quadBuffer[i].subType;

		const float tx = 0.25f * (float)subType;
		const float ty = 0.25f * (float)type;
		
		//const float size = particleTypeArr[QUAD].size;
		const Vector3F& pos  = quadBuffer[i].pos;
		const Color4F& color = quadBuffer[i].color;

		// Set up the particle that everything else is derived from:
		quadIndexBuffer[index++] = vertex+0;
		quadIndexBuffer[index++] = vertex+1;
		quadIndexBuffer[index++] = vertex+2;

		quadIndexBuffer[index++] = vertex+0;
		quadIndexBuffer[index++] = vertex+2;
		quadIndexBuffer[index++] = vertex+3;

		QuadVertex* pV = &vertexBuffer[vertex];

		for( int j=0; j<4; ++j ) {
			pV->pos = base[j].pos + pos;
			pV->tex.Set( tx+tex[j].x, ty+tex[j].y );
			pV->color = color;
			++pV;
		}

		vertex += 4;
	}

	CHECK_GL_ERROR;
	glEnableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, particleTypeArr[QUAD].texture->glID );

	CHECK_GL_ERROR;
	U8* vPtr = (U8*)vertexBuffer + 0;
	U8* tPtr = (U8*)vertexBuffer + 12;
	U8* cPtr = (U8*)vertexBuffer + 20;

	CHECK_GL_ERROR;
	glVertexPointer(   3, GL_FLOAT, sizeof(QuadVertex), vPtr );
	glTexCoordPointer( 2, GL_FLOAT, sizeof(QuadVertex), tPtr );
	glColorPointer(    4, GL_FLOAT, sizeof(QuadVertex), cPtr );

	glDrawElements( GL_TRIANGLES, nParticles[QUAD]*6, GL_UNSIGNED_SHORT, quadIndexBuffer );
	CHECK_GL_ERROR;

	// Restore standard state.
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	CHECK_GL_ERROR;
}

	
void ParticleSystem::DrawDecalParticles( int flag )
{
	if ( nDecals == 0 ) {
		return;
	}

	const float EPS = 0.05f;
	const float DSIZE = 0.7f;
	QuadVertex base[4];
	base[0].pos.Set( -DSIZE, EPS,  DSIZE );
	base[1].pos.Set(  DSIZE, EPS,  DSIZE );
	base[2].pos.Set(  DSIZE, EPS, -DSIZE );
	base[3].pos.Set( -DSIZE, EPS, -DSIZE );

	const Vector2F tex[4] = {
		{ 0.0f,  0.25f },
		{ 0.25f, 0.25f },
		{ 0.25f, 0.0f },
		{ 0.0f,  0.0f }
	};

	int index = 0;
	int vertex = 0;
	int count = 0;

	for( int i=0; i<nDecals; ++i ) 
	{
		if ( decalBuffer[i].flags & flag ) {
			const int type	  = SELECTION;
			const int subType = decalBuffer[i].subType;

			const float tx = 0.25f * (float)subType;
			const float ty = 0.25f * (float)type;
			
			const Vector3F& pos  = decalBuffer[i].pos;
			const Color4F& color = decalBuffer[i].color;

			// Set up the particle that everything else is derived from:
			quadIndexBuffer[index++] = vertex+0;
			quadIndexBuffer[index++] = vertex+1;
			quadIndexBuffer[index++] = vertex+2;

			quadIndexBuffer[index++] = vertex+0;
			quadIndexBuffer[index++] = vertex+2;
			quadIndexBuffer[index++] = vertex+3;

			QuadVertex* pV = &vertexBuffer[vertex];

			for( int j=0; j<4; ++j ) {
				pV->pos = base[j].pos + pos;
				pV->tex.Set( tx+tex[j].x, ty+tex[j].y );
				pV->color = color;
				++pV;
			}
			if ( decalBuffer[i].rotation != 0.0f ) {
				Matrix4 m;
				Vector3F prime;
				m.SetYRotation( decalBuffer[i].rotation );
				pV -= 4;
				for( int j=0; j<4; ++j ) {
					prime = m * base[j].pos;
					pV->pos = prime + pos;
					++pV;
				}
			}
			vertex += 4;
			++count;
		}
	}

	if ( count ) {
		CHECK_GL_ERROR;
		glEnableClientState( GL_VERTEX_ARRAY );
		glDisableClientState( GL_NORMAL_ARRAY );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glEnableClientState( GL_COLOR_ARRAY );

		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, particleTypeArr[QUAD].texture->glID );

		CHECK_GL_ERROR;
		U8* vPtr = (U8*)vertexBuffer + 0;
		U8* tPtr = (U8*)vertexBuffer + 12;
		U8* cPtr = (U8*)vertexBuffer + 20;

		CHECK_GL_ERROR;
		glVertexPointer(   3, GL_FLOAT, sizeof(QuadVertex), vPtr );
		glTexCoordPointer( 2, GL_FLOAT, sizeof(QuadVertex), tPtr );
		glColorPointer(    4, GL_FLOAT, sizeof(QuadVertex), cPtr );

		glDrawElements( GL_TRIANGLES, count*6, GL_UNSIGNED_SHORT, quadIndexBuffer );
		CHECK_GL_ERROR;

		// Restore standard state.
		glEnableClientState( GL_VERTEX_ARRAY );
		glEnableClientState( GL_NORMAL_ARRAY );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glDisableClientState( GL_COLOR_ARRAY );
		CHECK_GL_ERROR;
	}
}
