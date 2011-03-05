#ifndef UFO_ATTACK_RESEARCH_SCENE_INCLUDED
#define UFO_ATTACK_RESEARCH_SCENE_INCLUDED

#include "scene.h"
#include "../gamui/gamui.h"
#include "../grinliz/glstringutil.h"


class Research;

class ResearchSceneData : public SceneData
{
public:
	Research* research;
};


class ResearchScene : public Scene
{
public:
	ResearchScene( Game* _game, ResearchSceneData* data );
	virtual ~ResearchScene()	{}

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

private:
	void SetDescription();
	void SetOptions();

	ResearchSceneData*	data;
	BackgroundUI		backgroundUI;

	enum {
		MAX_OPTIONS = 3
	};

	gamui::TextBox			mainDescription;
	gamui::ToggleButton		optionButton[MAX_OPTIONS];
	gamui::TextLabel		optionName[MAX_OPTIONS];
	gamui::TextLabel		optionRequires[MAX_OPTIONS];
	gamui::PushButton		okayButton;
};


#endif // UFO_ATTACK_RESEARCH_SCENE_INCLUDED