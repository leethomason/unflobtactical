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

#ifndef UFOATTACK_NEW_TACTICAL_OPTIONS
#define UFOATTACK_NEW_TACTICAL_OPTIONS

#include "scene.h"
#include "../gamui/gamui.h"


class NewTacticalOptionsData;


class NewTacticalOptions : public Scene
{
public:
	NewTacticalOptions( Game*, NewTacticalOptionsData* );
	virtual ~NewTacticalOptions()	{}

	// UI
	virtual void Tap(	int count, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	{
		clip3D->Zero();
		clip2D->Zero();	// full screen
		return RENDER_2D; 
	}

	virtual void Resize();

private:
	enum {
		SQUAD_4 = 0,
		SQUAD_6,
		SQUAD_8,
		TERRAN_LOW,
		TERRAN_MED,
		TERRAN_HIGH,
		ALIEN_LOW,
		ALIEN_MED,
		ALIEN_HIGH,
		TIME_DAY,
		TIME_NIGHT,
		// scenarios: 11 -> 22
		UFO_CRASH = 23,	

		TOGGLE_COUNT = 24,
		MAX_ROWS = 3
	};

	BackgroundUI		backgroundUI;
	gamui::TextLabel	terranLabel, 
						alienLabel, 
						timeLabel, 
						scenarioLabel, 
						rowLabel[MAX_ROWS];
	gamui::ToggleButton	toggles[TOGGLE_COUNT];
	gamui::PushButton	goButton;
};

#endif // UFOATTACK_NEW_TACTICAL_OPTIONS