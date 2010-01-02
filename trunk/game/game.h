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
struct sqlite3;

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
	Game( const Screenport& screenport, const char* savePath );
	~Game();

	void DoTick( U32 msec );

	void Tap( int count, int x, int y );
	void Drag( int action, int x, int y );
	void Zoom( int action, int distance );
	void Rotate( int action, float degreesFromStart );
	void CancelInput();
	
	Surface*		GetLightMap( const char* name );
	const ItemDef*  GetItemDef( const char* name );
	ItemDef**		GetItemDefArr()	{ return itemDefArr; }

	Engine engine;
	Surface surface;

	// debugging / testing / mapmaker
	void MouseMove( int x, int y );
#ifdef MAPMAKER
	void RotateSelection( int delta );
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );
	void ShowPathing( bool show )	{ showPathing = show; }
	bool IsShowingPathing()			{ return showPathing; }
#endif

	//void SaveMap( const char* name );
	//void LoadMap( const char* name );
	//void ClearMap()				{ engine.GetMap()->Clear(); }

	enum { MAX_NUM_LIGHT_MAPS = 16,

		   BATTLE_SCENE = 0,
		   CHARACTER_SCENE,
		   NUM_SCENES,

		   MAX_SCENE_STACK = 4,
		   MAX_STREAMS = MAX_SCENE_STACK*2,
		 };


	UFOStream* OpenStream( const char* name, bool createIfDoesNotExist = true );
	UFOStream* FindStream( const char* name );
	void SaveStreamToDisk( UFOStream* s );
	void SaveAllStreamsToDisk();

	void PushScene( int sceneID );
	void PopScene();

	U32 CurrentTime() const	{ return currentTime; }

private:
	void CreateScene( int id );
	void PushPopScene();
	bool scenePopQueued;
	int scenePushQueued;

	struct MapLightInit
	{
		const char* name;
		int objectX;	// offset from object origin to light map origin.
		int objectY;
		int x;	// image position
		int y;
		int cx;	// light map size
		int cy;
		bool upperLeft;
	};

	struct MapItemInit 
	{
		const char* Name() const { return model; }

		const char* model;
		const char* modelOpen;
		const char* modelDestroyed;
		int cx;
		int cy;
		int hp;
		float flammable;		// 0 - 1
		/*
			+----X
			|			4
			|		8		2
			Z			1

		*/
		const char* pather;
		const char* visibility;
		int lightDef;
	};

	void LoadTextures();
	void LoadModels();
	void LoadModel( const char* name );
	void LoadLightMaps();
	void LoadMapResources();
	void LoadItemResources();

	void InitMapLight( int index, const MapLightInit* init );
	void InitMapItemDef( int startIndex, const MapItemInit* );

	UFOStream* rootStream;

	int currentFrame;
	U32 markFrameTime;
	U32 frameCountsSinceMark;
	float framesPerSecond;
	int trianglesPerSecond;
	int trianglesSinceMark;
	ModelLoader* modelLoader;
	sqlite3* database;

	Scene* currentScene;
	CStack<int> sceneStack;

	U32 currentTime;
	U32 previousTime;
	bool isDragging;

	int rotTestStart;
	int rotTestCount;
	Screenport screenport;
	std::string savePath;
	
#ifdef MAPMAKER	
	bool showPathing;
#endif
	grinliz::ProfileData profile;

	ItemDef*			itemDefArr[EL_MAX_ITEM_DEFS];
	int					nLightMaps;
	Surface				lightMaps[MAX_NUM_LIGHT_MAPS];
};

#endif
