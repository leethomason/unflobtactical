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

#ifndef UFOATTACK_SCENE_INCLUDED
#define UFOATTACK_SCENE_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glrectangle.h"

class Game;
class Engine;
class TiXmlElement;

#pragma warning ( push )
#pragma warning ( disable : 4100 )	// un-referenced formal parameter

class Scene
{
public:
	Scene( Game* _game ) : game( _game )						{}
	virtual ~Scene()											{}

	virtual void Activate()										{}
	virtual void DeActivate()									{}

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2I& screen,
						const grinliz::Ray& world )				{}

	virtual void Drag(	int action,
						const grinliz::Vector2I& view )			{}
	virtual void Zoom( int action, int distance )				{}
	virtual void Rotate( int action, float degrees )			{}
	virtual void CancelInput()									{}

	virtual void Save( TiXmlElement* doc )						{}
	virtual void Load( const TiXmlElement* doc )				{}

	// Rendering
	enum {
		RENDER_2D = 0x01,
		RENDER_3D = 0x02
	};
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	{ return 0; }
	// Perspective rendering.
	virtual void DoTick( U32 currentTime, U32 deltaTime )		{}
	// 2D overlay rendering.
	virtual void DrawHUD()										{}

protected:
	Engine* GetEngine();

	Game* game;
};

#pragma warning ( pop )

#endif // UFOATTACK_SCENE_INCLUDED