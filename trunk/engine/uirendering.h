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

#ifndef UIRENDERING_INCLUDED
#define UIRENDERING_INCLUDED

#include "vertex.h"


class UIWidgets
{
public:
	UIWidgets();
	~UIWidgets()	{}

	enum {
		ICON_CAMERA_HOME,
		ICON_FOG_OF_WAR,
		ICON_COUNT
	};

	int QueryTap( int x, int y );	
	void Draw( int width, int height, int rotation );

private:
	grinliz::Vector2< S16 > pos[ICON_COUNT*4];
	grinliz::Vector2F tex[ICON_COUNT*4];
	U16 index[6*ICON_COUNT];
};


#endif // UIRENDERING_INCLUDED