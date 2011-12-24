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
		clip3D->Zero(); 
		clip2D->Zero(); 
		return RENDER_2D;
	}	

private:
	BackgroundUI		backgroundUI;
	gamui::PushButton	okayButton;

	const GeoEndSceneData* data;
};



#endif //  GEO_END_SCENE_INCLUDED