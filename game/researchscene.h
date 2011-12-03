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

#ifndef UFO_ATTACK_RESEARCH_SCENE_INCLUDED
#define UFO_ATTACK_RESEARCH_SCENE_INCLUDED

#include "scene.h"
#include "../gamui/gamui.h"
#include "../grinliz/glstringutil.h"


class Research;
class Engine;
class Model;

class ResearchSceneData : public SceneData
{
public:
	Research* research;
};


class ResearchScene : public Scene
{
public:
	ResearchScene( Game* _game, ResearchSceneData* data );
	virtual ~ResearchScene();

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
		return RENDER_2D | RENDER_3D;
	}	
	virtual void Draw3D();

private:
	void SetDescription();
	void SetOptions();

	ResearchSceneData*	data;
	gamui::Image		background;
	gamui::Image		researchImage;

//	Engine* localEngine;
//	Model*  model;

	enum {
		MAX_OPTIONS = 3
	};

	gamui::TextLabel		title;
	gamui::TextBox			mainDescription;
	gamui::ToggleButton		optionButton[MAX_OPTIONS];
	gamui::TextLabel		optionName[MAX_OPTIONS];
	gamui::TextLabel		optionRequires[MAX_OPTIONS];
	gamui::PushButton		okayButton, helpButton;
	gamui::Image			image;
};


#endif // UFO_ATTACK_RESEARCH_SCENE_INCLUDED