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

#ifndef UFOATTACK_GAME_INCLUDED
#define UFOATTACK_GAME_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../engine/engine.h"
#include "../engine/surface.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"

class ParticleSystem;
class Scene;

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
	int Rotation() { return rotation; }

	void Tap( int count, int x, int y );
	void Drag( int action, int x, int y );
	void Zoom( int action, int distance );
	void CancelInput();
	
	ModelResource*	GetResource( const char* name );
	Texture*		GetTexture( const char* name );
	Surface*		GetLightMap( const char* name );
	void TransformScreen( int x0, int y0, int *x1, int *y1 );

	Engine engine;
	Surface surface;
	ParticleSystem* particleSystem;

#ifdef MAPMAKER
	void MouseMove( int x, int y );
	void RotateSelection();
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );
#endif

	void SaveMap( FILE* fp )	{ engine.GetMap()->Save( fp ); }
	void LoadMap( FILE* fp )	{ engine.GetMap()->Load( fp ); }
	void ClearMap()				{ engine.GetMap()->Clear(); }

	enum { NUM_LIGHT_MAPS = 1,

		   BATTLE_SCENE = 0,
		   NUM_SCENES,

		   MAX_SCENE_STACK = 4,
		 };

private:

	void LoadTextures();
	void FreeTextures();
	void LoadModels();
	void FreeModels();
	void LoadMapResources();
	void LoadMap( const char* name );

	int rotation;
	int nTexture;
	int nModelResource;
	U32 markFrameTime;
	U32 frameCountsSinceMark;
	float framesPerSecond;
	int trianglesPerSecond;
	int trianglesSinceMark;

	Scene* currentScene;
	Scene* scenes[NUM_SCENES];
	Scene* sceneStack[MAX_SCENE_STACK];
	int nSceneStack;

	U32 previousTime;
	bool isDragging;

	int rotTestStart;
	int rotTestCount;

	Texture			texture[MAX_TEXTURES];
	ModelResource	modelResource[EL_MAX_MODEL_RESOURCES];
	Surface			lightMaps[NUM_LIGHT_MAPS];

};

#endif
