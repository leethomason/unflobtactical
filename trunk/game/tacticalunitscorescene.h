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

#ifndef UFO_ATTACK_TACTICAL_UNIT_SCORE_SCENE_INCLUDED
#define UFO_ATTACK_TACTICAL_UNIT_SCORE_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "gamelimits.h"

class UIImage;
class UIButtonBox;
class UIButtonGroup;
class UITextTable;
struct TacticalEndSceneData;

class TacticalUnitScoreScene : public Scene
{
public:
	TacticalUnitScoreScene( Game* _game, const TacticalEndSceneData* data );
	virtual ~TacticalUnitScoreScene();

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2I& screen,
						const grinliz::Ray& world );

	virtual void Drag(	int action,
						const grinliz::Vector2I& view )			{}

	virtual void Zoom( int action, int distance )				{}
	virtual void CancelInput()									{}

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	
	{ 
		clip3D->SetInvalid(); 
		clip2D->SetInvalid(); 
		return RENDER_2D;
	}	
	virtual void DoTick( U32 currentTime, U32 deltaTime )		{}
	virtual void DrawHUD();

private:
	UIImage*		background;
	UITextTable*	textTable;
	UIButtonBox*	buttonBox;
	const TacticalEndSceneData* data;
	int nAwards;
	enum { MAX_AWARDS = 40 };
	UIImage*		awards[MAX_AWARDS];
};


#endif // UFO_ATTACK_TACTICAL_UNIT_SCORE_SCENE_INCLUDED