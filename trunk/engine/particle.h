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

#ifndef UFOTACTICAL_PARTICLE_INCLUDED
#define UFOTACTICAL_PARTICLE_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glbitarray.h"
#include "vertex.h"
#include "ufoutil.h"
#include "map.h"

class Texture;
class ParticleEffect;

/*	Class to render all sorts of particle effects.
*/
class ParticleSystem
{
public:
	static ParticleSystem* Instance()	{ GLASSERT( instance ); return instance; }

	static void Create();
	static void Destroy();

	// Texture particles. Location in texture follows.
	enum {
		FIRE			= 0,
		NUM_FIRE		= 2,
		SMOKE			= 4,
		NUM_SMOKE		= 2,

		BEAM			= 2,
		CIRCLE			= 11,
	};

	enum {
		POINT,
		QUAD,
		NUM_PRIMITIVES
	};

	enum {
		PARTICLE_RAY,
		PARTICLE_HEMISPHERE,
		PARTICLE_SPHERE,
	};


	// Emit N point particles.
	void EmitPoint(	int count,						// number of particles to create
					int configuration,				// PARTICLE_RAY, etc.
					const Color4F& color,			// color of the particle
					const Color4F& colorVelocity,	// change in color / second
					const grinliz::Vector3F& pos,	// origin
					float posFuzz,					// fuzz in the position
					const grinliz::Vector3F& vel,	// velocity
					float velFuzz );					// fuzz in the velocity

	// Emit one quad particle.
	void EmitQuad(	int type,						// FIRE, SMOKE
					const Color4F& color,			// color of the particle
					const Color4F& colorVelocity,	// change in color / second
					const grinliz::Vector3F& pos,	// origin
					float posFuzz,					// fuzz in the position
					const grinliz::Vector3F& vel,	// velocity
					float velFuzz,					// fuzz in the velocity
					float halfWidth,				// 1/2 size of the particle
					float velHalfWidth );				// rate of change of the 1/2 size

	// Simple call to emit a point at a location.
	void EmitOnePoint(	const Color4F& color, 
						const Color4F& colorVelocity,
						const grinliz::Vector3F& pos );

	// Emits a compound flame system for this frame of animation.
	void EmitFlame( U32 delta, const grinliz::Vector3F& pos )	{ EmitSmokeAndFlame( delta, pos, true ); }
	void EmitSmoke( U32 delta, const grinliz::Vector3F& pos )	{ EmitSmokeAndFlame( delta, pos, false ); }

	void Update( U32 deltaTime, U32 currentTime );
	void Draw( const grinliz::Vector3F* eyeDir, const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>* fogOfWar );
	void Clear();

	void AddEffect( ParticleEffect* effect );
	// Not a "real" factory - can return 0. But re-uses when possible.
	ParticleEffect* EffectFactory( const char* name );

private:
	ParticleSystem();
	~ParticleSystem();

	static ParticleSystem* instance;

	struct Particle
	{
		// streamed to GL
		grinliz::Vector3F	pos;		
		Color4F				color;

		// extra data
		grinliz::Vector3F vel;			// units / second
		grinliz::Vector4F colorVel;		// units / second
		float			  halfWidth;	// for rays and quads
		float			  velHalfWidth;	// for quads
		U8				  type;	

		void Process( float sec, unsigned msec ) {
			pos			+= vel*sec;
			color		+= colorVel*sec;
			halfWidth	+= velHalfWidth*sec;
		}
	};

	struct QuadVertex
	{
		grinliz::Vector3F	pos;
		grinliz::Vector2F	tex;
		Color4F				color;
	};

	// General do-all for emmitting all kinds of particles (except decals.)
	void Emit(	int primitive,					// POINT or QUAD
				int type,						// FIRE, SMOKE, BEAM (location in the texture)
				int count,						// number of particles to create
				int configuration,				// PARTICLE_RAY, QUAD_BEAM, etc.
				const Color4F& color,			// color of the particle
				const Color4F& colorVelocity,	// change in color / second
				const grinliz::Vector3F& pos0,	// origin
				float posFuzz,					// fuzz in the position
				const grinliz::Vector3F& vel,	// velocity
				float velFuzz,					// fuzz in the velocity
				float halfWidth,				// half width of beams and quads
				float velHalfWidth );			// rate of change of the width

	void DrawPointParticles( const grinliz::Vector3F* eyeDir );
	void DrawQuadParticles( const grinliz::Vector3F* eyeDir );
	void EmitSmokeAndFlame( U32 delta, const grinliz::Vector3F& pos, bool flame );
	void RemoveOldParticles( int primitive );
	int NumParticles( int type ) { return type == POINT ? pointBuffer.Size() : quadBuffer.Size(); };
	
	void Cull( CDynArray<Particle>* );
	void Process( CDynArray<Particle>*, float time, unsigned msec );

	grinliz::Random rand;

	Texture* quadTexture;
	Texture* pointTexture;
	int frame;

	CDynArray<ParticleEffect*>							effectArr;
	const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>*	fogOfWar;
	CDynArray<Particle>									pointBuffer;
	CDynArray<Particle>									quadBuffer;

	// When we don't have point sprites:
	CDynArray<QuadVertex>								vertexBuffer;
	CDynArray<U16>										indexBuffer;
};

#endif // UFOTACTICAL_PARTICLE_INCLUDED
