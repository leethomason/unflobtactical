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
#include "../tinyxml/tinyxml.h"
#include "../shared/gamedbreader.h"
#include "../gamui/gamui.h"
#include "../engine/uirendering.h"

class UIImage;
class UIButtonBox;
class UIButtonGroup;


class TacticalIntroScene : public Scene
{
public:
	TacticalIntroScene( Game* _game );
	virtual ~TacticalIntroScene();

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	{
		clip3D->SetInvalid();
		clip2D->SetInvalid();	// full screen
		return RENDER_2D; 
	}
	virtual void DrawHUD();

	//	Squad:		4 8
	//	  exp:		Low Med Hi
	//  Alien:		8 16
	//    exp:		Low Med Hi
	//  Weather:	Day Night
	enum {

		SQUAD_4 = 0,
		SQUAD_8,
		TERRAN_LOW,
		TERRAN_MED,
		TERRAN_HIGH,
		ALIEN_8,
		ALIEN_16,
		ALIEN_LOW,
		ALIEN_MED,
		ALIEN_HIGH,
		TIME_DAY,
		TIME_NIGHT,

		LOC_FARM,

		SCEN_LANDING,
		SCEN_CRASH,

		TOGGLE_COUNT,
	};

	struct SceneInfo {
		const char*		base;			// FARM
		int				blockSizeX;		// 2, 3, 4
		int				blockSizeY;		// 2, 3, 4
		bool			needsLander;	// true/false
		int				ufoSize;		// 0: no, >0: blocksize
		bool			crash;
	};
	static void CalcInfo( int location, int scenario, int ufoSize, SceneInfo* info );

	void CreateMap( TiXmlNode* parent, 
					int seed,
					int location,			// LOC_FARM
					int scenario,			// SCEN_LANDING
					int ufoSize );			// 0: small, 1: big

private:
	void WriteXML( TiXmlNode* xml );
	void FindNodes( const char* set,
					int size,
					const char* type,
					const gamedb::Item* parent );

	void AppendMapSnippet(	int x, int y, int tileRotation,
							const char* set,
							int size,
							const char* type,
							const gamedb::Item* parent,
							TiXmlElement* mapElement );

	enum { MAX_ITEM_MATCH = 32 };
	const gamedb::Item* itemMatch[ MAX_ITEM_MATCH ];
	int nItemMatch;
	grinliz::Random random;

	BackgroundUI		backgroundUI;
	gamui::PushButton	continueButton, newButton, helpButton, goButton, seedButton;
	gamui::TextLabel	terranLabel, alienLabel, timeLabel, seedLabel, scenarioLabel;
	gamui::ToggleButton	toggles[TOGGLE_COUNT];
};


#endif // UFO_ATTACK_TACTICAL_INTRO_SCENE_INCLUDED