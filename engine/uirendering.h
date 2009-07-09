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
#include "screenport.h"


class UIButtonBox
{
public:
	UIButtonBox( const Texture* texture, const Screenport& port );
	~UIButtonBox()	{}

	enum {
		ICON_PLAIN			= 0,
		ICON_CHARACTER		= 1,
		ICON_AIM			= 4,
		ICON_SNAP			= 5,
		ICON_AUTO			= 6,

		MAX_ICONS = 20,
		MAX_TEXT_LEN = 12,
	};

	// Coordinates in pixels.
	void SetOrigin( int x, int y )				{	origin.Set( x, y ); }
	void SetColumns( int columns  )				{	if ( this->columns != columns ) {
														this->columns = columns; 
														cacheValid = false; 
													}
												}

	void SetButtonSize( int dx, int dy )		{	if ( size.x != dx || size.y != dy ) {
														size.x = dx; size.y = dy; 
														cacheValid = false; 
													}
												}
	void SetPadding( int dx, int dy )			{	if ( pad.x != dx || pad.y != dy ) {
														pad.x = dx; pad.y = dy; 
														cacheValid = false;		
													}
												}

	void SetButtons( const int* icons, int nIcons );
	void SetText( const char** text );
	void SetText( int index, const char* text );

	// Set the alpha of non-text
	void SetAlpha( float alpha )		{ this->alpha = alpha; cacheValid = false; }

	void CalcDimensions( int *x, int *y, int *w, int *h );
	void SetEnabled( int index, bool enabled );

	// returns the icon INDEX, or -1 if not clicked
	int QueryTap( int x, int y );	
	void Draw();

private:
	void CalcButtons();
	void PositionText( int index );
	int INV( int i ) { return nIcons-i-1; }

	struct Icon
	{
		int						id;
		char					text[MAX_TEXT_LEN];
		bool					enabled;
		grinliz::Vector2I		textPos;
	};
	const Texture* texture;
	int nIcons;

	bool cacheValid;
	Screenport screenport;
	int columns;
	float alpha;
	grinliz::Vector2I		origin;
	grinliz::Vector2I		size;
	grinliz::Vector2I		pad;

	Icon					icons[MAX_ICONS];
	grinliz::Vector2< S16 > pos[MAX_ICONS*4];
	grinliz::Vector2F		tex[MAX_ICONS*4];
	grinliz::Vector4F		color[MAX_ICONS*4];
	U16						index[6*MAX_ICONS];
};


#endif // UIRENDERING_INCLUDED