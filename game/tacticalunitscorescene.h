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

#ifndef UFO_ATTACK_TACTICAL_UNIT_SCORE_SCENE_INCLUDED
#define UFO_ATTACK_TACTICAL_UNIT_SCORE_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "gamelimits.h"

class UIImage;
class UIButtonBox;
class UIButtonGroup;
class UITextTable;

class TacticalUnitScoreScene : public Scene
{
public:
	TacticalUnitScoreScene( Game* _game );
	virtual ~TacticalUnitScoreScene();

	virtual void Activate();
	virtual void Resize();

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	
	{ 
		clip3D->Zero(); 
		clip2D->Zero(); 
		return RENDER_2D;
	}	
	virtual void DrawHUD();

private:
	enum { MAX_ROWS = 8 };
	enum { MAX_AWARDS = 40 };

	BackgroundUI			backgroundUI;
	NameRankUI				nameRank[MAX_ROWS];
	gamui::TextLabel		status[MAX_ROWS];
	gamui::PushButton		button;
	gamui::Image			award[MAX_AWARDS];

	//TacticalEndSceneData* data;
	int nAwards;
};


#endif // UFO_ATTACK_TACTICAL_UNIT_SCORE_SCENE_INCLUDED
