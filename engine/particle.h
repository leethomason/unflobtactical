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

	enum {
		MAX_POINT_PARTICLES = 400,
		MAX_QUAD_PARTICLES = 100,
		MAX_DECALS = 48
	};

	// Texture particles. Location in texture follows.
	enum {
		FIRE			= 0,
		NUM_FIRE		= 2,
		SMOKE			= 4,
		NUM_SMOKE		= 2,
		BEAM			= 2,
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

	/*
	enum {
		DECAL_BOTTOM = 0x01,
		DECAL_TOP = 0x02,
		DECAL_BOTH = 0x03
	};
	*/

	// Emit N point particles.
	void EmitPoint(	int count,						// number of particles to create
					int configuration,				// PARTICLE_RAY, etc.
					const Color4F& color,			// color of the particle
					const Color4F& colorVelocity,	// change in color / second
					const grinliz::Vector3F& pos,	// origin
					float posFuzz,					// fuzz in the position
					const grinliz::Vector3F& vel,	// velocity
					float velFuzz,					// fuzz in the velocity
					U32 lifetime );					// lifetime in milliseconds (0 is one frame)

	// Emit one quad particle.
	void EmitQuad(	int type,						// FIRE, SMOKE
					const Color4F& color,			// color of the particle
					const Color4F& colorVelocity,	// change in color / second
					const grinliz::Vector3F& pos,	// origin
					float posFuzz,					// fuzz in the position
					const grinliz::Vector3F& vel,	// velocity
					float velFuzz,					// fuzz in the velocity
					float halfWidth,				// 1/2 size of the particle
					float velHalfWidth,				// rate of change of the 1/2 size
					U32 lifetime );					// lifetime in milliseconds

	// Simple call to emit a point at a location.
	void EmitOnePoint(	const Color4F& color, 
						const Color4F& colorVelocity,
						const grinliz::Vector3F& pos,
						U32 lifetime );

	void EmitBeam(	const Color4F& color,			// color of the particle
					const Color4F& colorVelocity,	// change in color / second
					const grinliz::Vector3F& p0,	// beginning
					const grinliz::Vector3F& p1,	// end
					float beamWidth,
					U32 lifetime );

	// Emits a compound flame system for this frame of animation.
	void EmitFlame( U32 delta, const grinliz::Vector3F& pos )	{ EmitSmokeAndFlame( delta, pos, true ); }
	void EmitSmoke( U32 delta, const grinliz::Vector3F& pos )	{ EmitSmokeAndFlame( delta, pos, false ); }

	// Place a decal for a frame (decals don't have lifetimes.)
	//void EmitDecal( int id, int flags, const grinliz::Vector3F& pos, float alpha, float rotation );

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
		grinliz::Vector3F pos1;			// for rays
		float			  halfWidth;	// for rays and quads
		float			  velHalfWidth;	// for quads
		U16 age;						// milliseconds
		U16 lifetime;					// milliseconds
		U8	type;	
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
				const grinliz::Vector3F* pos1,	// dst, used for rays
				float posFuzz,					// fuzz in the position
				const grinliz::Vector3F& vel,	// velocity
				float velFuzz,					// fuzz in the velocity
				float halfWidth,				// half width of beams and quads
				float velHalfWidth,				// rate of change of the width
				U32 lifetime );					// lifetime in milliseconds

	void DrawPointParticles( const grinliz::Vector3F* eyeDir );
	void DrawQuadParticles( const grinliz::Vector3F* eyeDir );
	void EmitSmokeAndFlame( U32 delta, const grinliz::Vector3F& pos, bool flame );
	void RemoveOldParticles( int primitive );

	grinliz::Random rand;
	int nParticles[NUM_PRIMITIVES];
	Texture* quadTexture;
	Texture* pointTexture;

	CDynArray<ParticleEffect*>		effectArr;
	grinliz::BitArray<Map::SIZE, Map::SIZE, 1> fogOfWarFilter;

	Particle		pointBuffer[ MAX_POINT_PARTICLES ];
	Particle		quadBuffer[ MAX_QUAD_PARTICLES ];

	QuadVertex		vertexBuffer[ MAX_QUAD_PARTICLES*4];
	U16				quadIndexBuffer[ MAX_QUAD_PARTICLES*6 ];
};

#endif // UFOTACTICAL_PARTICLE_INCLUDED
