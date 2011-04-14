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

#ifndef AREA_WIDGET_INCLUDED
#define AREA_WIDGET_INCLUDED

#include "../gamui/gamui.h"

class Game;


class AreaWidget
{
public:
	AreaWidget( Game* game,
				gamui::Gamui* container,
				const char* name );

	void SetOrigin( float x, float y );
	void SetInfluence( float x );
	void SetTraits( int i );

private:
	gamui::TextLabel	name;
	gamui::DigitalBar	bar;
	gamui::Image		trait[2];
};


#endif // AREA_WIDGET_INCLUDED
