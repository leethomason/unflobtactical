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

#ifndef UFO_FAST_BATTLE_SCENE_INCLUDED
#define UFO_FAST_BATTLE_SCENE_INCLUDED

#include "scene.h"
#include "../gamui/gamui.h"
#include "../grinliz/glstringutil.h"
#include "unit.h"


class FastBattleSceneData : public SceneData
{
public:
	int	  seed;
	int   scenario;			// FARM_SCOUT, etc.
	bool  crash;			// is a crashed alien
	Unit* soldierUnits;		// pointer to MAX_TERRAN units
	bool  dayTime;			// fixme: set
	float  alienRank;		// fixme: set consistent with game difficulty

	Storage* storage;
};


class FastBattleScene : public Scene
{
public:
	FastBattleScene( Game* _game, FastBattleSceneData* data );
	virtual ~FastBattleScene()	{}

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	
	{ 
		clip3D->SetInvalid(); 
		clip2D->SetInvalid(); 
		return RENDER_2D;
	}	

	virtual void SceneResult( int sceneID, int result );


private:
	int RunSim( Unit* soldier, Unit* aliens, bool dayTime );

	enum {
		TL_SCENARIO,
		TL_CRASH,
		TL_DAYTIME,
		TL_ALIEN_RANK,
		TL_RESULT,
		NUM_TL,

		NUM_BATTLE = 24
	};
	FastBattleSceneData*	data;
	BackgroundUI			backgroundUI;
	gamui::TextLabel		scenarioText[NUM_TL];
	gamui::TextLabel		battle[NUM_BATTLE];
	gamui::PushButton		button;

	int						battleResult;
	grinliz::Random			random;

	Storage					foundStorage;
	Unit aliens[MAX_ALIENS];
	Unit civs[MAX_CIVS];
};


#endif // UFO_FAST_BATTLE_SCENE_INCLUDED
