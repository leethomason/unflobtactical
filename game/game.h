#ifndef UFOATTACK_GAME_INCLUDED
#define UFOATTACK_GAME_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../engine/engine.h"
#include "../engine/surface.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"

class Engine;
class Surface;


const float FOV = 45.0f;
const int MAX_TEXTURES = 32;
const int MAX_MODELS = 256;


class Game
{
private:
	EngineData engineData;

public:
	Game( int width, int height );
	~Game();

	void DoTick( U32 msec );
	void SetScreenRotation( int i );
	void Tap( int count, int x, int y );
	
	ModelResource* GetResource( const char* name );
	Texture* GetTexture( const char* name );
	void TransformScreen( int x0, int y0, int *x1, int *y1 );

	Engine engine;
	Surface surface;

#ifdef MAPMAKER
	void MouseMove( int x, int y );
	void RotateSelection();
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );
#endif

	void SaveMap( FILE* fp )	{ engine.GetMap()->Save( fp ); }
	void LoadMap( FILE* fp )	{ engine.GetMap()->Load( fp ); }
	void ClearMap()				{ engine.GetMap()->Clear(); }
private:
	void LoadTextures();
	void FreeTextures();
	void LoadModels();
	void FreeModels();
	void LoadMapResources();
	void LoadMap( const char* name );

	Texture texture[MAX_TEXTURES];
	ModelResource modelResource[MAX_MODELS];

	enum {
		NUM_ICONS=2
	};
	IconInfo iconInfo[NUM_ICONS];

	int rotation;
	int nTexture;
	int nModelResource;
	U32 markFrameTime;
	U32 frameCountsSinceMark;
	float framesPerSecond;

	Texture* iconTexture;

	enum { NUM_TEST_MODEL = 256 };
	Model* testModel[NUM_TEST_MODEL];
	Model* mapModel;
	int rotTestStart;
	int rotTestCount;

	// Mapmaker:
	Model* selection;
	int currentMapItem;
};

#endif
