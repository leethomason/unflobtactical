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

#ifndef UFO_ATTACK_SAVE_SCENE_INCLUDED
#define UFO_ATTACK_SAVE_SCENE_INCLUDED

#include "scene.h"

class UIImage;
class UIButtonBox;
class UIButtonGroup;


class SaveLoadSceneData : public SceneData
{
public:
	SaveLoadSceneData( bool _canSave ) : canSave( _canSave ) {}
	bool canSave;
};


class SaveLoadScene : public Scene
{
public:
	SaveLoadScene( Game* _game, const SaveLoadSceneData* data );
	virtual ~SaveLoadScene();

	virtual void Resize();

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

private:
	void Confirm( bool takeAction );
	void EnableSlots();

	const SaveLoadSceneData* data;
	gamui::Image		background;

	enum { SAVE_SLOTS = 4 };
	gamui::PushButton	backButton;
	gamui::PushButton	okayButton;
	gamui::ToggleButton saveButton;
	gamui::ToggleButton loadButton;
	gamui::ToggleButton slotButton[SAVE_SLOTS];
	gamui::TextLabel	slotText[SAVE_SLOTS];
	gamui::TextLabel	slotTime[SAVE_SLOTS];
	gamui::TextLabel	confirmText;
};


#endif // UFO_ATTACK_SAVE_SCENE_INCLUDED