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

#ifndef UFO_ATTACK_TACTICAL_END_SCENE_INCLUDED
#define UFO_ATTACK_TACTICAL_END_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "gamelimits.h"

class UIImage;
class UIButtonBox;
class UIButtonGroup;
class UITextTable;
class Research;


/*
class TacticalEndSceneData : public SceneData
{
public:

	enum {
		VICTORY		= 1,
		DEFEAT		= 2,
		TIE			= 3,
	};

	Unit* soldiers;
	const Unit* aliens;
	const Unit* civs;
	
	int result;
	bool dayTime;
	int scenario;

	Storage* storage;
};	
*/


class TacticalEndScene : public Scene
{
public:
	TacticalEndScene( Game* _game );
	virtual ~TacticalEndScene();

	virtual void Activate();

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
	virtual void DrawHUD();

private:
	enum { TEXT_ROW = 6, TEXT_COL = 3, ITEM_NUM = 16 };
	BackgroundUI		backgroundUI;
	gamui::TextLabel	victory;

	gamui::TextLabel	textTable[TEXT_ROW*TEXT_COL];
	gamui::TextLabel	totalScoreLabel, totalScoreValue;
	gamui::TextLabel	items[ITEM_NUM];

	gamui::PushButton	okayButton;

//	const TacticalEndSceneData* data;
};


#endif // UFO_ATTACK_TACTICAL_END_SCENE_INCLUDED