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
#include "particleeffect.h"

extern int trianglesRendered;	// FIXME: should go away once all draw calls are moved to the enigine
extern int drawCalls;			// ditto

using namespace grinliz;


/*static*/ void ParticleSystem::Create()
{
	GLASSERT( instance == 0 );
	instance = new ParticleSystem();
}


/*static*/ void ParticleSystem::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}


ParticleSystem* ParticleSystem::instance = 0;


ParticleSystem::ParticleSystem()
{
	for( int i=0; i<NUM_PRIMITIVES; ++i ) {
		nParticles[i] = 0;
	}
	nDecals = 0;
	pointTexture = 0;
	quadTexture = 0;
}


ParticleSystem::~ParticleSystem()
{
	Clear();
}


void ParticleSystem::Clear()
{
	for( int i=0; i<NUM_PRIMITIVES; ++i ) {
		nParticles[i] = 0;
	}
	nDecals = 0;
	for( int i=0; i<effectArr.Size(); ++i ) {
		delete effectArr[i];
	}
	effectArr.Clear();
}


void ParticleSystem::AddEffect( ParticleEffect* effect )
{
	effectArr.Push( effect );
}


ParticleEffect* ParticleSystem::EffectFactory( const char* name )
{
	for( int i=0; i<effectArr.Size(); ++i ) {
		if ( effectArr[i]->Done() && strcmp( effectArr[i]->Name(), name ) == 0 ) {
			ParticleEffect* effect = effectArr[i];
			effectArr.SwapRemove( i );
			return effect;
		}
	}

	// If we can't re-use, and there are a few entries, go ahead and
	// cull the old memory. '20' is the magic random number here.
	if ( effectArr.Size() > 20 ) {
		int dc=0;
		while ( dc < effectArr.Size() ) {
			if ( effectArr[dc]->Done() ) {
				delete effectArr[dc];
				effectArr.SwapRemove( dc );
			}
			else {
				++dc;
			}
		}
	}
	return 0;
}


void ParticleSystem::Update( U32 deltaTime, U32 currentTime )
{
	// Process the effects (may change number of particles, etc.
	for( int i=0; i<effectArr.Size(); ++i ) {
		if ( !effectArr[i]->Done() ) {
			effectArr[i]->DoTick( currentTime, deltaTime );
		}
	}

	// Process the particles.
	float sec = (float)deltaTime / 1000.0f;

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
			p->age += deltaTime;
			if ( p->lifetime == 0 ) {
				// special case: lifetime of 0 is always a one frame particle.
				p->age = 2;
				p->lifetime = 1;	// clean up next time.
			}
			else if ( p->age > p->lifetime ) {
				if ( i==POINT ) {
					// swap with end.
					grinliz::Swap( p, pEnd-1);
					--pEnd;
				}
				else {
					// for quads, keep the same order so there isn't visual popping as they resort.
					for( Particle* q = p; (q+1)<pEnd; ++q ) {
						*q = *(q+1);
					}
					--pEnd;
				}
				continue;
			}
			// apply effect of time
			p->pos += p->vel*sec;
			p->color += p->colorVel*sec;
			p->halfWidth += p->velHalfWidth*sec;
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
							const grinliz::Vector3F& pos0,	// origin
							const grinliz::Vector3F* pos1,	// origin
							float posFuzz,
							const grinliz::Vector3F& vel,	// velocity
							float velFuzz,
							float halfWidth,
							float velHalfWidth,
							U32 lifetime )
{
	GLASSERT( primitive >= 0 && primitive < NUM_PRIMITIVES );
	GLASSERT( type >=0 && type < 16 );
	GLASSERT( config >= 0 && config <= PARTICLE_SPHERE );

	const int MAX_PARTICLES[2] = { MAX_POINT_PARTICLES, MAX_QUAD_PARTICLES };
	Particle* particles[2] = { pointBuffer, quadBuffer };

	GLASSERT( count < MAX_PARTICLES[primitive] );
	//lifetime = grinliz::Clamp( lifetime, (U32)16, (U32)0xffff );
	
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
	if ( len > 0.0f ) {
		normal = vel;
		normal.Normalize();
	}
	else {
		normal.Set( 0, 1, 0 );
	}

	for( int i=0; i<count; ++i ) {
		switch (config) {
			case PARTICLE_RAY:
				velP = vel;
				velP.x += velFuzz*(-0.5f + rand.Uniform());
				velP.y += velFuzz*(-0.5f + rand.Uniform());
				velP.z += velFuzz*(-0.5f + rand.Uniform());
				break;

			case PARTICLE_HEMISPHERE:
				rand.NormalVector3D( &velP.x );
				if ( DotProduct( velP, normal ) < 0.0f ) {
					velP = -velP;
				}
				velP.x = velP.x*len + velFuzz*(-0.5f + rand.Uniform());
				velP.y = velP.y*len + velFuzz*(-0.5f + rand.Uniform());
				velP.z = velP.z*len + velFuzz*(-0.5f + rand.Uniform());
				break;

			case PARTICLE_SPHERE:
				rand.NormalVector3D( &velP.x );
				velP.x = velP.x*len + velFuzz*(-0.5f + rand.Uniform());
				velP.y = velP.y*len + velFuzz*(-0.5f + rand.Uniform());
				velP.z = velP.z*len + velFuzz*(-0.5f + rand.Uniform());
				break;

			default:
				GLASSERT( 0 );
				break;
		};

		posP.x = pos0.x + posFuzz*(-0.5f + rand.Uniform() );
		posP.y = pos0.y + posFuzz*(-0.5f + rand.Uniform() );
		posP.z = pos0.z + posFuzz*(-0.5f + rand.Uniform() );

		if ( lifetime > 0 ) {
			const int LFUZZ = 64;
			lifeP = (lifetime * ((256-LFUZZ) + (rand.Rand()&(LFUZZ-1)))) >> 8;
		}
		else {
			lifeP = 0;
		}

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
		if ( pos1 ) {
			GLASSERT( type == BEAM );
			p->pos1 = *pos1;
		}
		else {
			p->pos1.Set( 0, 0, 0 );
		}
		p->halfWidth = halfWidth;
		p->velHalfWidth = velHalfWidth;
		p->color = colP;
		p->vel = velP;
		p->colorVel = colorVelocity;
		p->age = 0;
		p->lifetime = lifeP;

		p->type = (U8)type;
	}
}


void ParticleSystem::EmitPoint(	int count,						
								int configuration,				
								const Color4F& color,			
								const Color4F& colorVelocity,	
								const grinliz::Vector3F& pos,	
								float posFuzz,					
								const grinliz::Vector3F& vel,	
								float velFuzz,					
								U32 lifetime )					
{
	Emit( POINT, 0, count, configuration, color, colorVelocity, pos, 0, posFuzz, 
		  vel, velFuzz, 0.0f, 0.0f, lifetime );
}


void ParticleSystem::EmitQuad(	int type,						
								const Color4F& color,			
								const Color4F& colorVelocity,	
								const grinliz::Vector3F& pos,	
								float posFuzz,					
								const grinliz::Vector3F& vel,	
								float velFuzz,					
								float halfWidth,
								float velHalfWidth,
								U32 lifetime )			
{
	Emit( QUAD, type, 1, 0, color, colorVelocity, pos, 0, posFuzz, vel, velFuzz, 
		  halfWidth, velHalfWidth, lifetime );
}


void ParticleSystem::EmitOnePoint(	const Color4F& color, 
									const Color4F& colorVelocity,
									const grinliz::Vector3F& pos,
									U32 lifetime )
{
	grinliz::Vector3F vel = { 0, 0, 0 };
	Emit( POINT, 0, 1, PARTICLE_RAY, color, colorVelocity, pos, 0, 0, vel, 0, 0, 0, lifetime );
}


void ParticleSystem::EmitBeam(	const Color4F& color,			// color of the particle
								const Color4F& colorVelocity,	// change in color / second
								const grinliz::Vector3F& p0,	// beginning
								const grinliz::Vector3F& p1,	// end
								float beamWidth,
								U32 lifetime )
{
	Vector3F vel = { 0, 0, 0 };
	Emit( QUAD, BEAM, 1, 0, color, colorVelocity, p0, &p1, 0, vel, 0, beamWidth*0.5f, 0, lifetime );
}


void ParticleSystem::EmitDecal( int id, int flags, const grinliz::Vector3F& pos, float alpha, float rotation )
{
	GLASSERT( id >= 0 && id < 16 );
	const Color4F color		= { 1.0f, 1.0f, 1.0f, 1.0f };

	if ( nDecals == MAX_DECALS ) {
		nDecals--;
	}
	decalBuffer[nDecals].pos = pos;
	decalBuffer[nDecals].color = color;
	decalBuffer[nDecals].color.w = alpha ;
	decalBuffer[nDecals].type = id;
	decalBuffer[nDecals].rotation = rotation;
	decalBuffer[nDecals].flags = flags;
	nDecals++;
}


void ParticleSystem::EmitSmokeAndFlame( U32 delta, const Vector3F& _pos, bool flame )
{
	// flame, smoke, particle
	U32 count[3];
	const U32 interval[3] = { 500, 800, 200 };

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
	if ( flame ) {
		Vector3F pos = _pos;
		pos.y += 0.3f;

		const Color4F color		= { 1.0f, 1.0f, 1.0f, 1.0f };
		const Color4F colorVec	= { 0.0f, -0.1f, -0.1f, -0.4f };
		Vector3F velocity		= { 0.0f, 1.0f, 0.0f };

		if ( (rand.Rand()>>28)==0 ) {
			velocity.y = 0.1f;
		}

		for( U32 i=0; i<count[0]; ++i ) {
			Emit(	ParticleSystem::QUAD,
					ParticleSystem::FIRE + rand.Rand( NUM_FIRE ),
					1,
					ParticleSystem::PARTICLE_RAY,
					color,		colorVec,
					pos, 0,		0.4f,	
					velocity,	0.3f,
					0.5f,		0.0f,
					4000 );		
		}
	}
	
	// Smoke
	{
		Vector3F pos = _pos;
		//pos.y -= 1.0f;

		const Color4F color		= { 0.5f, 0.5f, 0.5f, 1.0f };
		const Color4F colorVec	= { -0.1f, -0.1f, -0.1f, -0.2f };
		Vector3F velocity		= { 0.0f, 0.8f, 0.0f };

		for( U32 i=0; i<count[1]; ++i ) {
			Emit(	ParticleSystem::QUAD,
					ParticleSystem::SMOKE + rand.Rand( NUM_SMOKE ),
					1,
					ParticleSystem::PARTICLE_RAY,
					color,		colorVec,
					pos, 0,		0.6f,	
					velocity,	0.2f,
					0.5f,		0.0f,
					7000 );		
		}
	}
	
	// Sparkle
	if ( flame ) {
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
					pos, 0,		0.05f,	
					velocity,	0.3f,
					0.5f,		0.0f,
					2000 );		
		}
	}
}


void ParticleSystem::Draw( const Vector3F* eyeDir, const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>* fogOfWar )
{
	if ( fogOfWar ) {
		fogOfWarFilter.ClearAll();
		for( int j=0; j<Map::SIZE; ++j ){
			for( int i=0; i<Map::SIZE; ++i ) {
				if ( fogOfWar->IsSet( i, j, 0 ) ) {
					// space the test out by one unit.
					fogOfWarFilter.Set( i, j, 0 );
					fogOfWarFilter.Set( i+1, j, 0 );
					fogOfWarFilter.Set( i-1, j, 0 );
					fogOfWarFilter.Set( i, j+1, 0 );
					fogOfWarFilter.Set( i, j-1, 0 );
				}
			}
		}
	}
	else {
		fogOfWarFilter.SetAll();
	}

	if ( !pointTexture ) {
		pointTexture = TextureManager::Instance()->GetTexture( "particleSparkle" );
		quadTexture = TextureManager::Instance()->GetTexture( "particleQuad" );
	}

	glEnable( GL_BLEND );
	glDepthMask( GL_FALSE );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	DrawPointParticles( eyeDir );

	DrawDecalParticles( DECAL_BOTTOM );
	glDisable( GL_DEPTH_TEST );
	DrawDecalParticles( DECAL_TOP );
	glEnable( GL_DEPTH_TEST );
	nDecals = 0;

	DrawQuadParticles( eyeDir );

	glDepthMask( GL_TRUE );
	glDisable( GL_BLEND );
}


void ParticleSystem::DrawPointParticles( const Vector3F* eyeDir )
{
	if ( nParticles[POINT] == 0 )
		return;

	CHECK_GL_ERROR;
	glEnableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );
	
	glEnable( GL_TEXTURE_2D );

	bool usePointSprite = true;
#ifdef _MSC_VER
	usePointSprite = (GLEW_ARB_point_sprite) ? true : false;
#endif
	
	if (usePointSprite)
		{
		#ifdef USING_GL	
			glEnable(GL_POINT_SPRITE);
			glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
		#else
			glEnable(GL_POINT_SPRITE_OES);
			glTexEnvx(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
		#endif
		CHECK_GL_ERROR;

		glBindTexture( GL_TEXTURE_2D, pointTexture->glID );
		CHECK_GL_ERROR;

		glPointSize( 4.0f );
		CHECK_GL_ERROR;
		
		U8* vPtr = (U8*)pointBuffer + 0;
		U8* cPtr = (U8*)pointBuffer + 12;

		glVertexPointer( 3, GL_FLOAT, sizeof(Particle), vPtr );
		glColorPointer( 4, GL_FLOAT, sizeof(Particle), cPtr );
		CHECK_GL_ERROR;
		// Points are not filtered on fogOfWarFilter. 
		glDrawArrays( GL_POINTS, 0, nParticles[POINT] );
		CHECK_GL_ERROR;
		trianglesRendered += nParticles[POINT];		// 1 or 2?? go with 1
		drawCalls++;

		#ifdef USING_GL
			glDisable( GL_POINT_SPRITE );
		#else
			glDisable( GL_POINT_SPRITE_OES );
		#endif
		CHECK_GL_ERROR;
	}
	else {
		QuadVertex base[4];
		float SIZE = 0.07f;
		base[0].pos = -eyeDir[Camera::RIGHT]*SIZE - eyeDir[Camera::UP]*SIZE;
		base[1].pos =  eyeDir[Camera::RIGHT]*SIZE - eyeDir[Camera::UP]*SIZE;
		base[2].pos =  eyeDir[Camera::RIGHT]*SIZE + eyeDir[Camera::UP]*SIZE;
		base[3].pos = -eyeDir[Camera::RIGHT]*SIZE + eyeDir[Camera::UP]*SIZE;

		static const Vector2F tex[4] = {
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f }
		};

		int index = 0;
		int vertex = 0;
		int skip = nParticles[POINT] / MAX_QUAD_PARTICLES + 1;	// the quad is used to buffer, so we may have
																// fewer particles.
		for( int i=0; i<nParticles[POINT]; i+=skip ) 
		{
			const Vector3F& pos  = pointBuffer[i].pos;
			Vector2I fowPos = { (int)pos.x, (int)pos.z };
			if ( !fogOfWarFilter.IsSet( fowPos.x, fowPos.y ) )
				continue;
			const Color4F& color = pointBuffer[i].color;

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
				pV->tex = tex[j];
				pV->color = color;
				++pV;
			}
			vertex += 4;
		}

		CHECK_GL_ERROR;
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );

		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, pointTexture->glID );

		CHECK_GL_ERROR;
		U8* vPtr = (U8*)vertexBuffer + 0;
		U8* tPtr = (U8*)vertexBuffer + 12;
		U8* cPtr = (U8*)vertexBuffer + 20;

		CHECK_GL_ERROR;
		glVertexPointer(   3, GL_FLOAT, sizeof(QuadVertex), vPtr );
		glTexCoordPointer( 2, GL_FLOAT, sizeof(QuadVertex), tPtr );
		glColorPointer(    4, GL_FLOAT, sizeof(QuadVertex), cPtr );

		// because of the skip, the #elements can be less than nParticles*6
		if ( index ) {
			glDrawElements( GL_TRIANGLES, index, GL_UNSIGNED_SHORT, quadIndexBuffer );
			trianglesRendered += index/3;
			drawCalls++;
		}
		CHECK_GL_ERROR;
	}

	
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

	//QuadVertex base[4];
	//base[0].pos = -eyeDir[Camera::RIGHT]*0.5f - eyeDir[Camera::UP]*0.5f;
	//base[1].pos =  eyeDir[Camera::RIGHT]*0.5f - eyeDir[Camera::UP]*0.5f;
	//base[2].pos =  eyeDir[Camera::RIGHT]*0.5f + eyeDir[Camera::UP]*0.5f;
	//base[3].pos = -eyeDir[Camera::RIGHT]*0.5f + eyeDir[Camera::UP]*0.5f;

	const static float cornerX[] = { -1, 1, 1, -1 };
	const static float cornerY[] = { -1, -1, 1, 1 };

	static const Vector2F tex[4] = {
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
		const float tx = 0.25f * (float)(type&0x03);
		const float ty = 0.25f * (float)(type>>2);
		
		//const float size = particleTypeArr[QUAD].size;
		const Vector3F& pos  = quadBuffer[i].pos;
		const Color4F& color = quadBuffer[i].color;

		Vector2I fowPos = { (int)pos.x, (int)pos.z };
		if ( !fogOfWarFilter.IsSet( fowPos.x, fowPos.y ) )
			continue;

		// Set up the particle that everything else is derived from:
		quadIndexBuffer[index++] = vertex+0;
		quadIndexBuffer[index++] = vertex+1;
		quadIndexBuffer[index++] = vertex+2;

		quadIndexBuffer[index++] = vertex+0;
		quadIndexBuffer[index++] = vertex+2;
		quadIndexBuffer[index++] = vertex+3;

		QuadVertex* pV = &vertexBuffer[vertex];

		if ( type == BEAM ) {
			const Vector3F pos1 = quadBuffer[i].pos1;
			Vector3F right;
			CrossProduct( eyeDir[Camera::NORMAL], pos1-pos, &right );
			right.Normalize();
			float halfWidth = quadBuffer[i].halfWidth;

			pV[0].pos = pos - right*halfWidth;
			pV[0].tex.Set( tx+0.0f, ty+0.0f );
			pV[0].color = color;
			
			pV[1].pos = pos + right*halfWidth;
			pV[1].tex.Set( tx+0.25f, ty+0.0f );
			pV[1].color = color;

			pV[2].pos = pos1 + right*halfWidth;
			pV[2].tex.Set( tx+0.25f, ty+0.25f );
			pV[2].color = color;

			pV[3].pos = pos1 - right*halfWidth;
			pV[3].tex.Set( tx+0.0f, ty+0.25f );
			pV[3].color = color;
		}
		else {
			const float hw = quadBuffer[i].halfWidth;

			for( int j=0; j<4; ++j ) {
				//pV->pos = base[j].pos + pos;
				pV->pos =   pos
						  + cornerX[j]*eyeDir[Camera::RIGHT]*hw 
					      + cornerY[j]*eyeDir[Camera::UP]*hw;
				pV->tex.Set( tx+tex[j].x, ty+tex[j].y );
				pV->color = color;
				++pV;
			}
		}
		vertex += 4;
	}

	CHECK_GL_ERROR;
	glEnableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, quadTexture->glID );

	CHECK_GL_ERROR;
	U8* vPtr = (U8*)vertexBuffer + 0;
	U8* tPtr = (U8*)vertexBuffer + 12;
	U8* cPtr = (U8*)vertexBuffer + 20;

	CHECK_GL_ERROR;
	glVertexPointer(   3, GL_FLOAT, sizeof(QuadVertex), vPtr );
	glTexCoordPointer( 2, GL_FLOAT, sizeof(QuadVertex), tPtr );
	glColorPointer(    4, GL_FLOAT, sizeof(QuadVertex), cPtr );

	if ( index ) {
		glDrawElements( GL_TRIANGLES, index, GL_UNSIGNED_SHORT, quadIndexBuffer );
		CHECK_GL_ERROR;
		trianglesRendered += nParticles[QUAD]*2;
		drawCalls++;
	}
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
			const float tx = 0.25f * (float)(decalBuffer[i].type&0x03);
			const float ty = 0.25f * (float)(decalBuffer[i].type>>2);
			
			const Vector3F& pos  = decalBuffer[i].pos;
			Vector2I fowPos = { (int)pos.x, (int)pos.z };
			if ( !fogOfWarFilter.IsSet( fowPos.x, fowPos.y ) )
				continue;

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
		glBindTexture( GL_TEXTURE_2D, quadTexture->glID );

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
		trianglesRendered += count*2;
		drawCalls++;

		// Restore standard state.
		glEnableClientState( GL_VERTEX_ARRAY );
		glEnableClientState( GL_NORMAL_ARRAY );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glDisableClientState( GL_COLOR_ARRAY );
		CHECK_GL_ERROR;
	}
}
