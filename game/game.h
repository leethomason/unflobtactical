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
#include "../grinliz/glperformance.h"
#include "../engine/ufoutil.h"

class ParticleSystem;
class Scene;
class ItemDef;

const float FOV = 45.0f;
const int MAX_TEXTURES = 32;
const int MAX_MODELS = 256;

const float ONE8 = 1.0f / 8.0f;
const float ONE16 = 1.0f / 16.0f;
const float TRANSLUCENT_WHITE	= ONE8*0.0f + ONE16;
const float TRANSLUCENT_GREEN	= ONE8*1.0f + ONE16;
const float TRANSLUCENT_BLUE	= ONE8*2.0f + ONE16;
const float TRANSLUCENT_RED		= ONE8*3.0f + ONE16;
const float TRANSLUCENT_YELLOW	= ONE8*4.0f + ONE16;
const float TRANSLUCENT_GREY	= ONE8*5.0f + ONE16;



class Game
{
private:
	EngineData engineData;

public:
	Game( const Screenport& screenport );
	~Game();

	void DoTick( U32 msec );

	void Tap( int count, int x, int y );
	void Drag( int action, int x, int y );
	void Zoom( int action, int distance );
	void Rotate( int action, float degreesFromStart );
	void CancelInput();
	
	ModelResource*	GetResource( const char* name );
	Texture*		GetTexture( const char* name );
	Surface*		GetLightMap( const char* name );
	const ItemDef*  GetItemDef( const char* name );
	const CDynArray<ItemDef*>& GetItemDefArray() { return itemDefArr; }

	Engine engine;
	Surface surface;
	ParticleSystem* particleSystem;

	// debugging / testing / mapmaker
	void MouseMove( int x, int y );
#ifdef MAPMAKER
	void RotateSelection( int delta );
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );
	void ShowPathing( bool show )	{ showPathing = show; }
	bool IsShowingPathing()			{ return showPathing; }
#endif

	void SaveMap( FILE* fp )	{ engine.GetMap()->Save( fp ); }
	void LoadMap( FILE* fp )	{ engine.GetMap()->Load( fp ); }
	void ClearMap()				{ engine.GetMap()->Clear(); }

	enum { NUM_LIGHT_MAPS = 1,

		   BATTLE_SCENE = 0,
		   CHARACTER_SCENE,
		   NUM_SCENES,

		   MAX_SCENE_STACK = 4,
		   MAX_STREAMS = MAX_SCENE_STACK*2,
		 };


	UFOStream* OpenStream( const char* name, bool createIfDoesNotExist = true );

	void PushScene( int sceneID );
	void PopScene();

private:
	void CreateScene( int id );
	void PushPopScene();
	bool scenePopQueued;
	int scenePushQueued;

	struct MapItemInit 
	{
		const char* Name() const { return model; }

		const char* model;
		const char* modelOpen;
		const char* modelDestroyed;
		int cx;
		int cy;
		int hp;
		int materialFlags;
		const char* pather0;
		const char* pather1;
	};

	struct MemStream {
		char name[EL_FILE_STRING_LEN];
		UFOStream* stream;
	};
	MemStream memStream[MAX_STREAMS];

	void LoadTextures();
	void FreeTextures();
	void LoadModels();
	void LoadModel( const char* name );
	void FreeModels();
	void LoadLightMaps();
	void LoadMapResources();
	void LoadItemResources();
	void LoadMap( const char* name );

	void InitMapItemDef( int startIndex, const MapItemInit* );

	int nTexture;
	int nModelResource;
	int currentFrame;
	U32 markFrameTime;
	U32 frameCountsSinceMark;
	float framesPerSecond;
	int trianglesPerSecond;
	int trianglesSinceMark;
	ModelLoader* modelLoader;

	Scene* currentScene;
	CStack<int> sceneStack;

	U32 previousTime;
	bool isDragging;

	int rotTestStart;
	int rotTestCount;
	Screenport screenport;

#ifdef MAPMAKER	
	bool showPathing;
#endif
	grinliz::ProfileData profile;

	CDynArray<ItemDef*>	itemDefArr;
	Texture				texture[MAX_TEXTURES];
	ModelResource		modelResource[EL_MAX_MODEL_RESOURCES];
	Surface				lightMaps[NUM_LIGHT_MAPS];
};

#endif
