#ifndef UFOATTACK_SETTINGS_SCENE_INCLUDED
#define UFOATTACK_SETTINGS_SCENE_INCLUDED

#include "scene.h"
#include "../grinliz/glstringutil.h"

class SettingScene : public Scene
{
public:
	SettingScene( Game* _game );
	virtual ~SettingScene();

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

private:
	grinliz::GLString CalcMod( int delta );

	gamui::Image		background;
	gamui::PushButton	doneButton;

	gamui::TextBox		modText;
	gamui::PushButton	modDown;
	gamui::TextLabel	modCurrent;
	gamui::PushButton	modUp;

	gamui::TextBox		moveText;
	gamui::ToggleButton	moveButton[2];

	gamui::TextBox		dotText;
	gamui::ToggleButton	dotButton[2];

	gamui::TextBox		debugText;
	gamui::ToggleButton	debugButton[4];

	gamui::ToggleButton	audioButton;
};


#endif // UFOATTACK_SETTINGS_SCENE_INCLUDED
