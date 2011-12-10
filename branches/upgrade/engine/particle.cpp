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
#include "../grinliz/glperformance.h"
#include "surface.h"
#include "camera.h"
#include "particleeffect.h"

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
	pointTexture = 0;
	quadTexture = 0;
	frame = 0;
}


ParticleSystem::~ParticleSystem()
{
	Clear();
}


void ParticleSystem::Clear()
{
	pointBuffer.Clear();
	quadBuffer.Clear();

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


void ParticleSystem::Cull( CDynArray<Particle>* buffer )
{
	if ( !buffer->Empty() ) {
		Particle* p = buffer->Mem();
		Particle* q = buffer->Mem();
		const Particle* end = buffer->Mem() + buffer->Size();

		while ( q < end ) {
			*p = *q;
			q++;
//			if ( p->age < p->lifetime )
//				++p;
			if ( p->color.a > 0.0f )
				++p;
		}
		int size = p - buffer->Mem();
		if ( size < buffer->Size() )
			buffer->Trim( size );
	}
}


void ParticleSystem::Process( CDynArray<Particle>* buffer, float time, unsigned msec )
{
	Particle* p = buffer->Mem();
	const Particle* end = buffer->Mem() + buffer->Size();
	while ( p < end ) {
		p->Process( time, msec );
		++p;
	}
}


void ParticleSystem::Update( U32 deltaTime, U32 currentTime )
{
	GRINLIZ_PERFTRACK

	// Process the effects (may change number of particles, etc.
	for( int i=0; i<effectArr.Size(); ++i ) {
		if ( !effectArr[i]->Done() ) {
			effectArr[i]->DoTick( currentTime, deltaTime );
		}
	}

	++frame;
	Cull( (frame&1) ? &pointBuffer : &quadBuffer );	// cull one buffer / frame


	// Process the particles.
	float sec = (float)deltaTime / 1000.0f;
	Process( &pointBuffer, sec, deltaTime );
	Process( &quadBuffer, sec, deltaTime );
}


void ParticleSystem::EmitBeam( const grinliz::Vector3F& p0, const grinliz::Vector3F& p1, const grinliz::Color4F& color )
{
	Beam* beam = beamBuffer.Push();
	beam->pos0 = p0;
	beam->pos1 = p1;
	beam->color = color;
}


void ParticleSystem::Emit(	int primitive,					// POINT or QUAD
							int type,						// FIRE, SMOKE
							int count,						// number of particles to create
							int config,						// PARTICLE_RAY, etc.
							const Color4F& color,
							const Color4F& colorVelocity,
							const grinliz::Vector3F& pos0,	// origin
							float posFuzz,
							const grinliz::Vector3F& vel,	// velocity
							float velFuzz,
							float halfWidth,
							float velHalfWidth )
{
	GLASSERT( primitive >= 0 && primitive < NUM_PRIMITIVES );
	GLASSERT( type >=0 && type < 16 );
	GLASSERT( config >= 0 && config <= PARTICLE_SPHERE );
//	GLASSERT( pos0.x >= 0 && pos0.x < (float)EL_MAP_SIZE );
//	GLASSERT( pos0.y >= 0 && pos0.y < (float)EL_MAP_SIZE );

	Vector3F posP, velP, normal;
	Color4F colP;

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

		posP.x = Clamp( pos0.x + posFuzz*(-0.5f + rand.Uniform() ), 0.0f, (float)EL_MAP_SIZE-0.1f );
		posP.y = pos0.y + posFuzz*(-0.5f + rand.Uniform() );
		posP.z = Clamp( pos0.z + posFuzz*(-0.5f + rand.Uniform() ), 0.0f, (float)EL_MAP_SIZE-0.1f );

		const float CFUZZ_MUL = 0.90f;
		const float CFUZZ_ADD = 0.20f;

		for( int j=0; j<3; ++j ) {
			colP.X(j) = color.X(j)*CFUZZ_MUL + rand.Uniform()*CFUZZ_ADD;
		}
		colP.a = color.a;	// don't fuzz alpha

		Color4F colorVelocityP = colorVelocity;
		if ( colorVelocityP.a > -0.1f )
			colorVelocityP.a = -0.f;	// must fade!

		Particle* p = (primitive==POINT) ? pointBuffer.Push() : quadBuffer.Push();

		p->pos = posP;
		p->halfWidth = halfWidth;
		p->velHalfWidth = velHalfWidth;
		p->color = colP;
		p->vel = velP;
		p->colorVel = colorVelocityP;
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
								float velFuzz )					
{
	Emit( POINT, 0, count, configuration, color, colorVelocity, pos, posFuzz, 
		  vel, velFuzz, 0.0f, 0.0f );
}


void ParticleSystem::EmitQuad(	int type,						
								const Color4F& color,			
								const Color4F& colorVelocity,	
								const grinliz::Vector3F& pos,	
								float posFuzz,					
								const grinliz::Vector3F& vel,	
								float velFuzz,					
								float halfWidth,
								float velHalfWidth )			
{
	Emit( QUAD, type, 1, 0, color, colorVelocity, pos, posFuzz, vel, velFuzz, 
		  halfWidth, velHalfWidth );
}


void ParticleSystem::EmitOnePoint(	const Color4F& color, 
									const Color4F& colorVelocity,
									const grinliz::Vector3F& pos )
{
	grinliz::Vector3F vel = { 0, 0, 0 };
	Emit( POINT, 0, 1, PARTICLE_RAY, color, colorVelocity, pos, 0, vel, 0, 0, 0 );
}


void ParticleSystem::EmitSmokeAndFlame( U32 delta, const Vector3F& _pos, bool flame )
{
	// flame, smoke, particle
	U32 count[3];
	const U32 interval[3] = { 500,		// flame
							  800,		// smoke
							  200 };	// sparkle

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
					pos,		0.4f,	
					velocity,	0.3f,
					0.5f,		0.0f );		
		}
	}
	
	// Smoke
	{
		Vector3F pos = _pos;
		//pos.y -= 0.5f;	ground plane doesn't have a depth write. Bummer.

		const Color4F color		= { 0.5f, 0.5f, 0.5f, 1.0f };
		const Color4F colorVec	= { -0.1f, -0.1f, -0.1f, -0.30f };
		Vector3F velocity		= { 0.0f, 0.8f, 0.0f };

		for( U32 i=0; i<count[1]; ++i ) {
			Emit(	ParticleSystem::QUAD,
					ParticleSystem::SMOKE + rand.Rand( NUM_SMOKE ),
					1,
					ParticleSystem::PARTICLE_RAY,
					color,		colorVec,
					pos,		0.4f,	
					velocity,	0.2f,
					0.5f,		0.0f );		
		}
	}
	
	// Sparkle
	if ( flame ) {
		Vector3F pos = _pos;

		const Color4F color		= { 1.0f, 0.3f, 0.1f, 1.0f };
		const Color4F colorVec	= { 0.0f, -0.1f, -0.1f, -0.18f };
		Vector3F velocity		= { 0.0f, 1.5f, 0.0f };

		for( U32 i=0; i<count[2]; ++i ) {
			Emit(	ParticleSystem::POINT,
					0,
					1,
					ParticleSystem::PARTICLE_RAY,
					color,		colorVec,
					pos,		0.05f,	
					velocity,	0.3f,
					0.5f,		0.0f );		
		}
	}
}


void ParticleSystem::Draw( const Vector3F* eyeDir, const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>* fogOfWar )
{
	GRINLIZ_PERFTRACK

	this->fogOfWar = fogOfWar;

	if ( !pointTexture ) {
		pointTexture = TextureManager::Instance()->GetTexture( "particleSparkle" );
		quadTexture = TextureManager::Instance()->GetTexture( "particleQuad" );
	}

	//GLOUTPUT(( "Particles point=%d quad=%d\n", pointBuffer.Size(), quadBuffer.Size() ));
	DrawPointParticles( eyeDir );
	DrawQuadParticles( eyeDir );
	DrawBeamParticles( eyeDir );
	beamBuffer.Clear();

	for( int i=0; i<effectArr.Size(); ++i ) {
		if ( !effectArr[i]->Done() ) {
			effectArr[i]->Draw( eyeDir );
		}
	}
}


void ParticleSystem::DrawBeamParticles( const Vector3F* eyeDir )
{
	if ( beamBuffer.Empty() ) {
		return;
	}

	//const static float cornerX[] = { -1, 1, 1, -1 };
	//const static float cornerY[] = { -1, -1, 1, 1 };

	// fixme: hardcoded texture coordinates
	static const Vector2F tex[4] = {
		{ 0.50f, 0.0f },
		{ 0.75f, 0.0f },
		{ 0.75f, 0.25f },
		{ 0.50f, 0.25f }
	};

	int nIndex = 0;
	int nVertex = 0;

	vertexBuffer.Clear();
	indexBuffer.Clear();

	U16* iBuf = indexBuffer.PushArr( 6*quadBuffer.Size() );
	QuadVertex* vBuf = vertexBuffer.PushArr( 4*quadBuffer.Size() );

	for( int i=0; i<beamBuffer.Size(); ++i ) 
	{
		const Beam& b = beamBuffer[i];

		// Set up the particle that everything else is derived from:
		iBuf[nIndex++] = nVertex+0;
		iBuf[nIndex++] = nVertex+1;
		iBuf[nIndex++] = nVertex+2;

		iBuf[nIndex++] = nVertex+0;
		iBuf[nIndex++] = nVertex+2;
		iBuf[nIndex++] = nVertex+3;

		QuadVertex* pV = &vBuf[nVertex];

		const float hw = 0.1f;	//quadBuffer[i].halfWidth;
		Vector3F n;
		CrossProduct( eyeDir[Camera::NORMAL], b.pos1-b.pos0, &n );
		if ( n.LengthSquared() > 0.0001f )
			n.Normalize();
		else
			n = eyeDir[Camera::RIGHT];

		pV[0].pos = b.pos0 - hw*n;
		pV[1].pos = b.pos0 + hw*n;
		pV[2].pos = b.pos1 + hw*n;
		pV[3].pos = b.pos1 - hw*n;

		for( int j=0; j<4; ++j ) {
			pV->tex = tex[j];
			pV->color = b.color;
			++pV;
		}
		nVertex += 4;
	}

	if ( nIndex ) {
		QuadParticleShader shader;
		shader.SetTexture0( quadTexture );

		GPUShader::Stream stream;
		stream.stride = sizeof( vertexBuffer[0] );
		stream.nPos = 3;
		stream.posOffset = 0;
		stream.nTexture0 = 2;
		stream.texture0Offset = 12;
		stream.nColor = 4;
		stream.colorOffset = 20;

		shader.SetStream( stream, vBuf, nIndex, iBuf );
		shader.Draw();
	}
}



void ParticleSystem::DrawPointParticles( const Vector3F* eyeDir )
{
	if ( pointBuffer.Empty() )
		return;

	if ( PointParticleShader::IsSupported() )
	{
		PointParticleShader shader;

		GPUShader::Stream stream;
		stream.stride = sizeof(pointBuffer[0]);
		stream.nPos = 3;
		stream.posOffset = 0;
		stream.nColor = 4;
		stream.colorOffset = 12;

		shader.SetStream( stream, pointBuffer.Mem(), 0, 0 );
		shader.DrawPoints( pointTexture, 4.f, 0, pointBuffer.Size() );
	}
	else {
		// Try to duplicate the above. Draw a bunch of little tetrahedrons so
		// we don't have to worry about screen space.
		vertexBuffer.Clear();
		indexBuffer.Clear();

		int index = 0;
		int vertex = 0;

		QuadVertex* vBuf = vertexBuffer.PushArr( 4*pointBuffer.Size() );
		U16* iBuf = indexBuffer.PushArr( 6*pointBuffer.Size() );

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


		for( int i=0; i<pointBuffer.Size(); ++i ) 
		{
			const Vector3F& pos = pointBuffer[i].pos;
			const Color4F& color = pointBuffer[i].color;

			// Set up the particle that everything else is derived from:
			iBuf[index++] = vertex+0;
			iBuf[index++] = vertex+1;
			iBuf[index++] = vertex+2;

			iBuf[index++] = vertex+0;
			iBuf[index++] = vertex+2;
			iBuf[index++] = vertex+3;

			QuadVertex* pV = vBuf + vertex;

			for( int j=0; j<4; ++j ) {
				pV->pos = base[j].pos + pos;
				pV->tex = tex[j];
				pV->color = color;
				++pV;
			}
			vertex += 4;
		}

		if ( index ) {
			QuadParticleShader shader;
			GPUShader::Stream stream;
			stream.stride = sizeof( vertexBuffer[0] );
			stream.nPos = 3;
			stream.posOffset = 0;
			stream.nTexture0 = 2;
			stream.texture0Offset = 12;
			stream.nColor = 4;
			stream.colorOffset = 20;

			shader.SetStream( stream, vBuf, index, iBuf );
			shader.SetTexture0( TextureManager::Instance()->GetTexture( "particleSparkle" ) );
			shader.Draw();
		}
	}
}


void ParticleSystem::DrawQuadParticles( const Vector3F* eyeDir )
{
	if ( quadBuffer.Empty() ) {
		return;
	}

	const static float cornerX[] = { -1, 1, 1, -1 };
	const static float cornerY[] = { -1, -1, 1, 1 };

	static const Vector2F tex[4] = {
		{ 0.0f, 0.0f },
		{ 0.25f, 0.0f },
		{ 0.25f, 0.25f },
		{ 0.0f, 0.25f }
	};

	int nIndex = 0;
	int nVertex = 0;

	vertexBuffer.Clear();
	indexBuffer.Clear();

	U16* iBuf = indexBuffer.PushArr( 6*quadBuffer.Size() );
	QuadVertex* vBuf = vertexBuffer.PushArr( 4*quadBuffer.Size() );
	Rectangle2I bounds;
	bounds.Set( 0, 0, EL_MAP_SIZE-1, EL_MAP_SIZE-1 );

	for( int i=0; i<quadBuffer.Size(); ++i ) 
	{
		const Particle& p = quadBuffer[i];

		const int type	  = p.type;
		const float tx = 0.25f * (float)(type&0x03);
		const float ty = 0.25f * (float)(type>>2);
		
		//const float size = particleTypeArr[QUAD].size;
		const Vector3F& pos  = p.pos;
		Color4F color = p.color;

		static const float FADE_IN = 0.95f;
		static const float FADE_IN_INV = 1.0f / (1.0f-FADE_IN);

		if ( color.a > FADE_IN ) {
			color.a = (1.0f - color.a)*FADE_IN_INV;
		}

		if ( fogOfWar ) {
			Vector2I fowPos = { (int)pos.x, (int)pos.z };

			if (    !bounds.Contains( fowPos )
				 || !fogOfWar->IsSet( fowPos.x, fowPos.y ) )
			{
				continue;
			}
		}

		// Set up the particle that everything else is derived from:
		iBuf[nIndex++] = nVertex+0;
		iBuf[nIndex++] = nVertex+1;
		iBuf[nIndex++] = nVertex+2;

		iBuf[nIndex++] = nVertex+0;
		iBuf[nIndex++] = nVertex+2;
		iBuf[nIndex++] = nVertex+3;

		QuadVertex* pV = &vBuf[nVertex];

		const float hw = quadBuffer[i].halfWidth;

		for( int j=0; j<4; ++j ) {
			pV->pos =   pos + ( cornerX[j]*eyeDir[Camera::RIGHT] + cornerY[j]*eyeDir[Camera::UP] ) * hw;
			pV->tex.Set( tx+tex[j].x, ty+tex[j].y );
			pV->color = color;
			++pV;
		}
		nVertex += 4;
	}

	if ( nIndex ) {
		QuadParticleShader shader;
		shader.SetTexture0( quadTexture );

		GPUShader::Stream stream;
		stream.stride = sizeof( vertexBuffer[0] );
		stream.nPos = 3;
		stream.posOffset = 0;
		stream.nTexture0 = 2;
		stream.texture0Offset = 12;
		stream.nColor = 4;
		stream.colorOffset = 20;

		shader.SetStream( stream, vBuf, nIndex, iBuf );
		shader.Draw();
	}
}



