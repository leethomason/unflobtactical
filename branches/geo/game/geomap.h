#ifndef GEOMAP_INCLUDED
#define GEOMAP_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../engine/surface.h"

#include "../engine/map.h"

class SpaceTree;
class Texture;
class Model;

// Note this is the lightweight iMap, NOT Map
class GeoMap :	public IMap,
				public ITextureCreator
{
public:
	GeoMap( SpaceTree* tree );
	~GeoMap();

	// IMap
	virtual Texture* LightFogMapTexture()		{ return dayNightTex; }
	virtual void LightFogMapParam( float* w, float* h, float* dx, float* dy );

	// ITextureCreator
	virtual void CreateTexture( Texture* t );

	// GeoMap functionality
	void DoTick( U32 currentTime, U32 deltaTime );


	enum {
		MAP_X = 20,
		MAP_Y = 10
	};

private:
	enum {
		DAYNIGHT_TEX_SIZE = 32
	};

	float				scrolling;
	float				dayNightOffset;

	SpaceTree*			tree;
	Texture*			dayNightTex;
	Surface				dayNightSurface;
	Model*				geoModel[2];
};

#endif // GEOMAP_INCLUDED