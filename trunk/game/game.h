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
class TiXmlDocument;
class Stats;

const float ONE8 = 1.0f / 8.0f;
const float ONE16 = 1.0f / 16.0f;
const float TRANSLUCENT_WHITE	= ONE8*0.0f + ONE16;
const float TRANSLUCENT_GREEN	= ONE8*1.0f + ONE16;
const float TRANSLUCENT_BLUE	= ONE8*2.0f + ONE16;
const float TRANSLUCENT_RED		= ONE8*3.0f + ONE16;
const float TRANSLUCENT_YELLOW	= ONE8*4.0f + ONE16;
const float TRANSLUCENT_GREY	= ONE8*5.0f + ONE16;


/*
	Assets
	---------------------------------

	Database:
	Read only. (Constructed by the builder.)

	The database (uforesource.db) contains everything the game needs to 
	run. This is primarily to ease the installation on the phones. One big
	wad file is easier to manage and take care of. sqlite3 does a great job
	of pulling resources out, so no worries about anything getting lost or
	slow. All assets are compressed in the database so the size is quite good.

	Models		- binary
	Textures	- binary
	Map			- XML

	Maps are saved as XML. Databases are just too tedious for rich data like maps.


	Serialization (Saving and Loading)
	----------------------------------
	Everything at runtime is saved as XML. This includes:
	Map (copied from the database, but then changed as the game plays)
	Engine
	Units
	Camera
	etc.

	MapMaker
	-------------------------------
	The evil mapmaker hack...the mapmaker works directly on XML files in the
	resource-in directory (resin). Since the MapMaker is also the BattleScene
	in a slightly different flavor, this leads to some circular code and compilation.
*/

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
	
	const ItemDef*  GetItemDef( const char* name );
	ItemDef* const*	GetItemDefArr() const	{ return itemDefArr; }

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

	enum {	MAX_NUM_LIGHT_MAPS = 16,

			BATTLE_SCENE = 0,
			CHARACTER_SCENE,
			INTRO_SCENE,
			END_SCENE,
			NUM_SCENES,
		 };

	void PushScene( int sceneID, void* data );
	void PopScene();

	U32 CurrentTime() const	{ return currentTime; }
	U32 DeltaTime() const	{ return currentTime-previousTime; }

	std::string GameSavePath()			{ return savePath + "currentgame.xml"; }
	void Load( const TiXmlDocument& doc );
	void Save( TiXmlDocument* doc );

	const char* AccessTextResource( const char* name );

	// cheating: moves states between scenes.
	int				loadRequested;	// 0-continue, 1-new, 2-test, -1 default
	std::string		newGameXML;

private:
	Scene* CreateScene( int id, void* data );
	void PushPopScene();
	bool scenePopQueued;
	void ProcessLoadRequest();

	int		scenePushQueued;
	void*	scenePushQueuedData;
	bool	loadCompleted;

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
	void LoadImages();
	void LoadMapResources();
	void LoadItemResources();
	void DumpWeaponInfo( FILE* fp, float range, const Stats& stats, int count );

	void InitMapLight( int index, const MapLightInit* init );
	void InitMapItemDef( int startIndex, const MapItemInit* );

	int currentFrame;
	U32 markFrameTime;
	U32 frameCountsSinceMark;
	float framesPerSecond;
	int trianglesPerSecond;
	int trianglesSinceMark;

	ModelLoader* modelLoader;
	sqlite3* database;

	CStack<Scene*> sceneStack;

	U32 currentTime;
	U32 previousTime;
	bool isDragging;

	int rotTestStart;
	int rotTestCount;
	Screenport screenport;
	std::string savePath;
	CDynArray< char > textResBuf;
	
#ifdef MAPMAKER	
	bool showPathing;
#endif
	grinliz::ProfileData profile;

	ItemDef*			itemDefArr[EL_MAX_ITEM_DEFS];
};

#endif
