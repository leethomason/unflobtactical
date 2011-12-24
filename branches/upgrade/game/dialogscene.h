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

#ifndef UFO_ATTACK_DIALOG_SCENE_INCLUDED
#define UFO_ATTACK_DIALOG_SCENE_INCLUDED

#include "scene.h"
#include "../gamui/gamui.h"
#include "../grinliz/glstringutil.h"

class DialogSceneData : public SceneData
{
public:
	// Input:
	enum DialogType {
		DS_YESNO
	};
	DialogType type;
	grinliz::CStr<1000> text;
};

class DialogScene : public Scene
{
public:
	DialogScene( Game* _game, const DialogSceneData* data );
	virtual ~DialogScene()	{}

	virtual void Activate();

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );

	//virtual void HandleHotKeyMask( int mask )

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	
	{ 
		clip3D->Zero(); 
		clip2D->Zero(); 
		return RENDER_2D;
	}	

private:
	const DialogSceneData*	data;
	gamui::Image		background;
	gamui::TextBox		textBox;
	gamui::PushButton	button0;	// no, cancel, false
	gamui::PushButton	button1;	// yes, accept, true
};


#endif // UFO_ATTACK_DIALOG_SCENE_INCLUDED