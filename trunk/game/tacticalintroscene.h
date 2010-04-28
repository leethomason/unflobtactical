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
						const grinliz::Vector2I& screen,
						const grinliz::Ray& world );

	virtual void Drag(	int action,
						const grinliz::Vector2I& view )			{}

	virtual void Zoom( int action, int distance )				{}
	virtual void CancelInput()									{}

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	{
		clip3D->SetInvalid();
		clip2D->SetInvalid();	// full screen
		return RENDER_2D; 
	}
	virtual void DoTick( U32 currentTime, U32 deltaTime )		{}
	virtual void DrawHUD();

	//	Squad:		4 8
	//	  exp:		Low Med Hi
	//  Alien:		8 16
	//    exp:		Low Med Hi
	//  Weather:	Day Night
	enum {
		NEW_GAME = 0,
		CONTINUE_GAME,

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

		SEED,
		GO_NEW_GAME,
		OPTION_COUNT
	};

	struct SceneInfo {
		const char*		base;			// FARM
		int				blockSizeX;		// 2, 3, 4
		int				blockSizeY;		// 2, 3, 4
		bool			needsLander;	// true/false
		int				ufo;			// 0: no, >0: blocksize
	};
	static void CalcInfo( int location, int ufoSize, SceneInfo* info );

	void CreateMap( TiXmlNode* parent, 
					int seed,
					int location,			// LOC_FARM
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
	UIImage*		background;
	UIImage*		backgroundNew;
	UIButtonBox*	buttons;
	UIButtonGroup*	choices;
	bool			showNewChoices;
};


#endif // UFO_ATTACK_TACTICAL_INTRO_SCENE_INCLUDED