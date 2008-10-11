#ifndef UFOATTACK_GAME_INCLUDED
#define UFOATTACK_GAME_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../engine/engine.h"
#include "../engine/surface.h"

class Engine;
class Surface;


const int WIDTH  = 480;
const int HEIGHT = 320;
const float FOV = 45.0f;
const float ASPECT = (float)(WIDTH) / (float)(HEIGHT);
const int MAX_TEXTURES = 32;

struct Texture
{
	char name[16];
	U32	 glID;

	void Set( const char* name, U32 glID );
};

class Game
{
private:
	EngineData engineData;

public:
	Game();
	~Game();

	void DoTick( U32 msec );

	int NumTextures()	{ return nTexture; }
	Texture texture[MAX_TEXTURES];
	Engine engine;
	Surface surface;

private:
	void LoadTextures();
	void FreeTextures();

	int nTexture;
	U32 markFrameTime;
	U32 frameCountsSinceMark;
	float framesPerSecond;
};

#endif
