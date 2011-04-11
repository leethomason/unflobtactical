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

#ifndef UFO_BUILD_BASE_SCENE_INCLUDED
#define UFO_BUILD_BASE_SCENE_INCLUDED

#include "scene.h"
#include "gamelimits.h"
#include "../gamui/gamui.h"

class BaseChit;


class BuildBaseSceneData : public SceneData
{
public:
	BaseChit* baseChit;
	int *cash;
};


class BuildBaseScene : public Scene
{
public:
	BuildBaseScene( Game* _game, BuildBaseSceneData* data );
	virtual ~BuildBaseScene()	{}

	virtual void Activate()		{}

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

	enum {
		NUM_FACILITIES = 5	// needs to match BaseChit
	};

private:
	void UpdateButtons();

	BuildBaseSceneData*	data;
	gamui::PushButton	backButton;

	gamui::Image		mapImage;
	gamui::PushButton	buyButton[NUM_FACILITIES];
	gamui::TextLabel	progressLabel[NUM_FACILITIES];
	gamui::PushButton	helpButton;

	gamui::Image		cashImage;
	gamui::TextLabel	cashLabel;
};


#endif // UFO_BUILD_BASE_SCENE_INCLUDED