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
#include "../engine/texture.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"
#include "../grinliz/glperformance.h"
#include "../engine/ufoutil.h"
#include "../tinyxml/tinyxml.h"
#include "../shared/gamedbreader.h"
#include "../gamui/gamui.h"

class ParticleSystem;
class Scene;
class ItemDef;
class TiXmlDocument;
class Stats;
class Unit;

static const float ONE8 = 1.0f / 8.0f;
static const float ONE16 = 1.0f / 16.0f;
static const float TRANSLUCENT_WHITE	= ONE8*0.0f + ONE16;
static const float TRANSLUCENT_GREEN	= ONE8*1.0f + ONE16;
static const float TRANSLUCENT_BLUE	= ONE8*2.0f + ONE16;
static const float TRANSLUCENT_RED		= ONE8*3.0f + ONE16;
static const float TRANSLUCENT_YELLOW	= ONE8*4.0f + ONE16;
static const float TRANSLUCENT_GREY	= ONE8*5.0f + ONE16;


struct TileSetDesc {
	const char* set;	// "FARM"
	int size;			// 16, 32, 64
	const char* type;	// "TILE"
	int variation;		// 0-99
};


/*
	Assets
	---------------------------------

	Database:
	Read only. (Constructed by the builder.)

	The database (uforesource.db) contains everything the game needs to 
	run. This is primarily to ease the installation on the phones. One big
	wad file is easier to manage and take care of. All assets are compressed 
	in the database so the size is quite good.

	Models		- binary
	Textures	- binary
	Map			- XML

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

class Game : public ITextureCreator 
{
private:
	EngineData engineData;

public:
	Game( int width, int height, int rotation, const char* savepath );
	Game( int width, int height, int rotation, const char* path, const TileSetDesc& tileSetDesc );
	~Game();

	void DeviceLoss();
	void Resize( int width, int height, int rotation );
	void DoTick( U32 msec );

	void Tap( int action, int x, int y );
	void Zoom( int action, int distance );
	void Rotate( int action, float degreesFromStart );
	void CancelInput();
	
	const ItemDef*  GetItemDef( const char* name );
	ItemDef* const*	GetItemDefArr() const	{ return itemDefArr; }

	// debugging / testing / mapmaker
	void MouseMove( int x, int y );
	void HandleHotKeyMask( int mask );

	void RotateSelection( int delta );
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );

	// MapMaker methods
	void ShowPathing( bool show )	{ mapmaker_showPathing = show; }
	bool IsShowingPathing()			{ return mapmaker_showPathing; }

	enum {	MAX_NUM_LIGHT_MAPS = 16,

			BATTLE_SCENE = 0,
			CHARACTER_SCENE,
			INTRO_SCENE,
			END_SCENE,
			UNIT_SCORE_SCENE,
			HELP_SCENE,
			NUM_SCENES,
		 };

	void QueueReset()		{ resetGame = true; }
	void PushScene( int sceneID, void* data );
	void PopScene();

	U32 CurrentTime() const	{ return currentTime; }
	U32 DeltaTime() const	{ return currentTime-previousTime; }

	const grinliz::GLString GameSavePath()	{	grinliz::GLString str( savePath );
												str += "currentgame.xml";
												return str;
											}

	void Load( const TiXmlDocument& doc );
	void Save();
	void Save( TiXmlDocument* doc );

	const gamedb::Reader* GetDatabase()	{ return database; }

	enum {
		ATOM_TEXT, ATOM_TEXT_D,
		ATOM_TACTICAL_BACKGROUND,
		ATOM_TACTICAL_BACKGROUND_TEXT,
		ATOM_GREEN_BUTTON_UP, ATOM_GREEN_BUTTON_UP_D, ATOM_GREEN_BUTTON_DOWN, ATOM_GREEN_BUTTON_DOWN_D,
		ATOM_BLUE_BUTTON_UP, ATOM_BLUE_BUTTON_UP_D, ATOM_BLUE_BUTTON_DOWN, ATOM_BLUE_BUTTON_DOWN_D,
		ATOM_RED_BUTTON_UP, ATOM_RED_BUTTON_UP_D, ATOM_RED_BUTTON_DOWN, ATOM_RED_BUTTON_DOWN_D,
		ATOM_COUNT
	};
	const gamui::RenderAtom& GetRenderAtom( int id );
	enum {
		GREEN_BUTTON,
		BLUE_BUTTON,
		RED_BUTTON,
		LOOK_COUNT
	};
	const gamui::ButtonLook& GetButtonLook( int id );

	// For creating some required textures:
	virtual void CreateTexture( Texture* t );

	// cheating: moves states between scenes.
	int				loadRequested;	// 0-continue, 1-new, 2-test, -1 default
	TiXmlDocument	newGameXML;

private:
	Screenport screenport;
public:
	Engine* engine;
private:
	Surface surface;

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
//		bool upperLeft;
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

	void Init();
	void LoadTextures();
	void LoadModels();
	void LoadModel( const char* name );
	void LoadMapResources();
	void LoadItemResources();
	void LoadAtoms();

	void DumpWeaponInfo( FILE* fp, float range, const Stats& stats, int count );

	void InitMapLight( int index, const MapLightInit* init );
	void InitMapItemDef( int startIndex, const MapItemInit* );

	int currentFrame;
	U32 markFrameTime;
	U32 frameCountsSinceMark;
	float framesPerSecond;
	int trianglesPerSecond;
	int trianglesSinceMark;
	bool debugTextOn;
	bool resetGame;

	ModelLoader* modelLoader;
	gamedb::Reader* database;

	CStack<Scene*> sceneStack;

	U32 currentTime;
	U32 previousTime;
	bool isDragging;

	int rotTestStart;
	int rotTestCount;
	grinliz::GLString savePath;
	CDynArray< char > resourceBuf;

	gamui::RenderAtom renderAtoms[ATOM_COUNT];
	gamui::ButtonLook buttonLooks[LOOK_COUNT];
	
	bool		mapmaker_showPathing;
	std::string mapmaker_xmlFile;
	grinliz::ProfileData profile;

	ItemDef*			itemDefArr[EL_MAX_ITEM_DEFS];
};




#endif
