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

#ifndef UFO_ATTACK_HELP_SCENE_INCLUDED
#define UFO_ATTACK_HELP_SCENE_INCLUDED

#include "scene.h"

class UIImage;
class UIButtonBox;
class UIButtonGroup;


class HelpSceneData : public SceneData
{
public:
	HelpSceneData( const char* _name, bool _settings ) : name( _name ), settings( _settings ) {}
	const char* name;
	bool settings;
};

class HelpScene : public Scene
{
public:
	HelpScene( Game* _game, const HelpSceneData* helpName );
	virtual ~HelpScene();

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
	virtual void HandleHotKeyMask( int mask );

private:
	enum {	NUM_BUTTONS = 4,
			NUM_TEXT_LABELS = 20
	};

	void Layout();

	const char*			helpName;
	int					currentScreen;
	gamui::Image		background;
	gamui::Image		image;
	gamui::TextBox		textBox;
	gamui::PushButton	buttons[NUM_BUTTONS];
};


#endif // UFO_ATTACK_HELP_SCENE_INCLUDED