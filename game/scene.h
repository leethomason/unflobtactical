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
#include "../gamui/gamui.h"
#include "../engine/uirendering.h"

class Game;
class Engine;
class TiXmlElement;
class Unit;

#pragma warning ( push )
#pragma warning ( disable : 4100 )	// un-referenced formal parameter

class Scene
{
public:
	Scene( Game* _game );
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
	virtual void HandleHotKeyMask( int mask )					{}

	// Rendering
	enum {
		RENDER_2D = 0x01,
		RENDER_3D = 0x02,
		RENDER_2D_FLIPPED = 0x04,
	};

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	
	{ 
		clip3D->SetInvalid(); 
		clip2D->SetInvalid(); 
		return 0; 
	}

	void RenderGamui2D()	{ gamui2D.Render(); }
	void RenderGamui3D()	{ gamui3D.Render(); }

	// Perspective rendering.
	virtual void DoTick( U32 currentTime, U32 deltaTime )		{}
	// 2D overlay rendering.
	virtual void DrawHUD()										{}

	// Put in debugging output into the 3D stream.
	virtual void Debug3D()										{}


protected:
	Engine* GetEngine();

	Game*			game;
	UIRenderer		uiRenderer;
	gamui::Gamui	gamui2D, gamui3D;
};


// Utility class for standard display of the Unit name / rank / weapon
struct NameRankUI {
	gamui::Image		rank;
	gamui::TextLabel	name;

	void Init( gamui::Gamui* );
	void Set( float x, float y, const Unit* unit, bool displayWeapon );
	void SetVisible( bool visible ) {
		rank.SetVisible( visible );
		name.SetVisible( visible );
	}
};

#pragma warning ( pop )

#endif // UFOATTACK_SCENE_INCLUDED