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


class UIButtonBox
{
public:
	UIButtonBox( const Texture* texture );
	~UIButtonBox()	{}

	enum {
		ICON_PLAIN,
		ICON_CHARACTER,
		NUM_ICONS,

		MAX_ICONS = 16,
		MAX_TEXT_LEN = 12,
	};

	void SetButtons( const int* icons, const char** text, int nIcons );

	// returns the icon INDEX, or nIcons if not clicked
	int QueryTap( int x, int y );	
	void Draw( int width, int height, int rotation );

private:
	struct Icon
	{
		int						id;
		char					text[MAX_TEXT_LEN];
		grinliz::Vector2I		textPos;
	};
	const Texture* texture;
	int nIcons;

	Icon					icons[MAX_ICONS];
	grinliz::Vector2< S16 > pos[MAX_ICONS*4];
	grinliz::Vector2F		tex[MAX_ICONS*4];
	U16						index[6*MAX_ICONS];
};


#endif // UIRENDERING_INCLUDED