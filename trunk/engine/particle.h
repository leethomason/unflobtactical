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
#include "vertex.h"

class Texture;

class ParticleSystem
{
public:


	enum {
		MAX_POINT_PARTICLES = 400,
		MAX_QUAD_PARTICLES = 100,
		MAX_DECALS = 48
	};

	ParticleSystem();
	~ParticleSystem();

	void InitPoint( const Texture* texture );
	void InitQuad( const Texture* texture );

	// texture particles
	enum {
		FIRE,			
		SMOKE,
		NUM_TYPE,

		SELECTION = 2,
	};

	enum {
		DECAL_DIR,
		DECAL_SELECTED,
		DECAL_PATH,
		NUM_DECAL
	};

	enum {
		POINT,
		QUAD,
		NUM_PRIMITIVES
	};

	enum {
		PARTICLE_RAY,
		PARTICLE_HEMISPHERE,
		PARTICLE_SPHERE
	};

	void Emit(	int primitive,					// POINT or QUAD
				int type,						// FIRE, SMOKE
				int count,						// number of particles to create
				int configuration,				// PARTICLE_RAY, etc.
				const Color4F& color,
				const Color4F& colorVelocity,
				const grinliz::Vector3F& pos,	// origin
				float posFuzz,
				const grinliz::Vector3F& vel,	// velocity
				float velFuzz,
				U32 lifetime );					// lifetime in milliseconds

	void EmitFlame( U32 delta, const grinliz::Vector3F& pos );
	void EmitDecal( int id, const grinliz::Vector3F& pos, float alpha, float rotation );

	void Update( U32 timePassed );
	void Draw( const grinliz::Vector3F* eyeDir );

	void Clear();

private:

	struct ParticleType 
	{
		const Texture* texture;
		float size;
		
		void Init( const Texture* texture, float size ) {
			this->texture = texture;
			this->size = size;
		}
	};

	struct Particle
	{
		// streamed to GL
		grinliz::Vector3F	pos;		
		Color4F				color;

		grinliz::Vector3F vel;			// units / second
		grinliz::Vector4F colorVel;		// units / second
		U16 age;						// milliseconds
		U16 lifetime;					// milliseconds
		U8	type;	
		U8	subType;
	};

	struct Decal
	{
		grinliz::Vector3F	pos;		
		Color4F				color;
		float				rotation;
		int					subType;
	};

	struct QuadVertex
	{
		grinliz::Vector3F	pos;
		grinliz::Vector2F	tex;
		Color4F				color;
	};

	void DrawPointParticles();
	void DrawQuadParticles( const grinliz::Vector3F* eyeDir );
	void DrawDecalParticles();

	grinliz::Random rand;
	int nParticles[NUM_PRIMITIVES];
	int nDecals;

	ParticleType	particleTypeArr[ NUM_PRIMITIVES ];
	Particle		pointBuffer[ MAX_POINT_PARTICLES ];
	Particle		quadBuffer[ MAX_QUAD_PARTICLES ];
	Decal			decalBuffer[ MAX_DECALS ];

	QuadVertex		vertexBuffer[ MAX_QUAD_PARTICLES*4];
	U16				quadIndexBuffer[ MAX_QUAD_PARTICLES*6 ];
};

#endif // UFOTACTICAL_PARTICLE_INCLUDED
