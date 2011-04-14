#ifndef GEO_END_SCENE_INCLUDED
#define GEO_END_SCENE_INCLUDED

#include "scene.h"


class GeoEndSceneData : public SceneData
{
public:
	bool victory;
};


class GeoEndScene : public Scene
{
public:
	GeoEndScene( Game* _game, const GeoEndSceneData* data );
	virtual ~GeoEndScene();

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
	BackgroundUI		backgroundUI;
	gamui::PushButton	okayButton;

	const GeoEndSceneData* data;
};



#endif //  GEO_END_SCENE_INCLUDED