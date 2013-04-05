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

#ifndef UFO_ATTACK_TACTICAL_INTRO_SCENE_INCLUDED
#define UFO_ATTACK_TACTICAL_INTRO_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "gamelimits.h"
#include "../tinyxml2/tinyxml2.h"
#include "../shared/gamedbreader.h"
#include "../gamui/gamui.h"
#include "../engine/uirendering.h"
#include "battlescenedata.h"

class UIImage;
class UIButtonBox;
class UIButtonGroup;


struct NewSceneOptionsReturn
{
	unsigned scenario	: 8;
	unsigned crash		: 1;
	unsigned nTerrans	: 4;
	unsigned terranRank : 4;
	unsigned alienRank  : 4;
	unsigned dayTime	: 1;
};

class TacticalIntroScene : public Scene
{
public:
	TacticalIntroScene( Game* _game );
	virtual ~TacticalIntroScene();

	virtual void Activate();

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );
	virtual void SceneResult( int sceneID, int result );

	virtual void JoyDPad( int dir );
	virtual void JoyButton( int id, bool down );
	virtual void DoTick( U32 currentTime, U32 deltaTime );

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	{
		clip3D->SetInvalid();
		clip2D->SetInvalid();	// full screen
		return RENDER_2D; 
	}
	virtual void DrawHUD();
	virtual void Resize();

	//	Squad:		4 8
	//	  exp:		Low Med Hi
	//  Alien:		8 16
	//    exp:		Low Med Hi
	//  Weather:	Day Night

	static bool IsScoutScenario( int s ) {
		GLASSERT( s >= FARM_SCOUT && s <= TERRAN_BASE );
		return (s >= FARM_SCOUT && s < FARM_DESTROYER);
	}
	static bool IsFrigateScenario( int s ) {
		GLASSERT( s >= FARM_SCOUT && s <= TERRAN_BASE );
		return (s >= FARM_DESTROYER && s <= DSRT_DESTROYER);
	}
	static bool IsUFOScenario( int s ) {
		GLASSERT( s >= FARM_SCOUT && s <= TERRAN_BASE );
		return    ( s >= FARM_SCOUT && s < CITY )
			   || ( s == BATTLESHIP );
	}
	static int CivsInScenario( int scenario ) {
		GLASSERT( scenario != TERRAN_BASE );
		int nCiv = 0;
		
		switch ( scenario ) {
		case CITY:
			nCiv = MAX_CIVS;
			break;

		case FARM_SCOUT:
		case FARM_DESTROYER:
			nCiv = MAX_CIVS * 2 / 3;
			break;

		case FRST_SCOUT:
		case FRST_DESTROYER:
			nCiv = MAX_CIVS / 2;
			break;

		default:
			break;
		}
		GLASSERT( nCiv >= 0 && nCiv <= MAX_CIVS );
		return nCiv;
	}

	struct SceneInfo {
		SceneInfo( int _scenario, bool _crash, int _nCivs ) : scenario( _scenario ), crash( _crash ), nCivs( _nCivs ) {
			if ( !SupportsCrash() )
				crash = false;
		}
		int		scenario;		// FARM_SCOUT -> TERRAN_BASE
		bool	crash;
		int		nCivs;

		grinliz::Vector2I Size() const;

		bool TileAlgorithm() const			{ return (scenario >= FARM_SCOUT && scenario <= DSRT_DESTROYER); }

		bool SupportsCivs() const			{ return (scenario >= FARM_SCOUT && scenario <= DSRT_DESTROYER) || scenario == CITY /*|| scenario == TERRAN_BASE*/; }
		bool SupportsCrash() const			{ return (scenario >= FARM_SCOUT && scenario <= DSRT_DESTROYER); };

		int UFOSize() const					{ return (scenario >= FARM_DESTROYER && scenario <= DSRT_DESTROYER) ? 2 : 1; }
		const char* UFO() const;
		const char* Base() const;
	};


	static int RandomRank( grinliz::Random* random, float rank, int nRanks );

	static void GenerateTerranTeam( Unit* units,				// target units to write
									int count,
									float averageLevel,
									const ItemDefArr&,			// if null, will have no items
									int seed=0 );

	static void GenerateAlienTeamUpper( int scenario,	
										bool crash,
										float rank,
										Unit* units,
										const ItemDefArr&,
										int seed=0 );

	static void GenerateAlienTeam(	Unit* units,				// target units to write
									const int alienCount[],		// aliens per type
									float averageLevel,
									const ItemDefArr&,
									int seed=0 );

	static void GenerateCivTeam(	Unit* units,				// target units to write
									int count,
									const ItemDefArr&,
									int seed=0 );

	static void CreateMap(	tinyxml2::XMLPrinter*, 
							int seed,
							const SceneInfo& info,
							const gamedb::Reader* database );

	static void WriteXML( FILE* fp, const BattleSceneData* data, const ItemDefArr&, const gamedb::Reader* database  );

	
private:
	enum { MAX_ITEM_MATCH = 32 };
	static void FindNodes(	const char* set,
							int size,
							const char* type,
							const gamedb::Item* parent,
							const gamedb::Item** itemMatch,
							int *nMatch );

	static void AppendMapSnippet(	int x, int y, int tileRotation,
									const char* set,
									int size,
									bool crash,
									const char* type,
									const gamedb::Reader* database,
									const gamedb::Item* parent,
									tinyxml2::XMLElement* mapElement,
									int seed );

	grinliz::Random random;

	BackgroundUI		backgroundUI;
	gamui::PushButton	continueButton, helpButton, goButton, settingButton;
	gamui::PushButton	loadButton;
	gamui::PushButton	newTactical, newGeo, newCampaign, newGame;
	gamui::TextLabel	newGameWarning;
	gamui::ToggleButton audioButton;
};


#endif // UFO_ATTACK_TACTICAL_INTRO_SCENE_INCLUDED