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

#ifndef UFO_ATTACK_GEO_SCENE_INCLUDED
#define UFO_ATTACK_GEO_SCENE_INCLUDED

#include "scene.h"

#include "../engine/ufoutil.h"
#include "../engine/texture.h"
#include "../engine/surface.h"
#include "../engine/model.h"

class UIImage;
class UIButtonBox;
class UIButtonGroup;
class AreaWidget;
class Model;
class GeoMap;


// Must be a POD - gets copied around
class Chit
{
public:
	enum {
		SCOUT,
		FRIGATE,
		BATTLESHIP,
		LANDER,
		NUM_TYPES
	};

	void InitAlienShip( SpaceTree* tree, int type, const grinliz::Vector2F& start, const grinliz::Vector2F& dest );
	void InitAlienShip( SpaceTree* tree, int type, float startx, float starty, float destx, float desty ) {
		grinliz::Vector2F start = { startx, starty };
		grinliz::Vector2F end = { destx, desty };
		InitAlienShip( tree, type, start, end );
	}

	void Free( SpaceTree* tree );

	void SetScroll( float deltaMap, float x0, float x1 );
	void DoTick( U32 currentTime, U32 deltaTime );

private:
	int type;
	grinliz::Vector2F pos;	// in map units
	grinliz::Vector2F dest;	// in map units
	Model* model;			// note that the model pos can be very different from the pos, because of scrolling
};


class GeoScene : public Scene
{
public:
	GeoScene( Game* _game );
	virtual ~GeoScene();

	virtual void Activate();
	virtual void DeActivate();

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

	virtual void DoTick( U32 currentTime, U32 deltaTime );

private:
	enum {	
		NUM_AREAS = 6,
	};

	void SetMapLocation();
	GeoMap* geoMap;
	SpaceTree* tree;

	grinliz::Vector2F	dragStart;
	float				mapOffset;
	float				WIDTH;		// width of map, in UI units
	float				HEIGHT;		// height of map, in UI units

	AreaWidget*			areaWidget[NUM_AREAS];
	CDynArray< Chit >	chitArr;
};


#endif // UFO_ATTACK_GEO_SCENE_INCLUDED