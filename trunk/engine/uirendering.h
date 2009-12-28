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
#include "../grinliz/glrectangle.h"


enum {
	ICON_GREEN_BUTTON	= 0,
	ICON_BLUE_BUTTON	= 1,
	ICON_RED_BUTTON		= 2,
	ICON_TRANS_RED		= 4,
	ICON_TRANS_BLUE		= 5,
	ICON_GREEN_BUTTON_DOWN	= 8,
	ICON_BLUE_BUTTON_DOWN	= 9,
	ICON_RED_BUTTON_DOWN	= 10,
	ICON_NONE			= 15,

	DECO_CHARACTER		= 0,
	DECO_PISTOL			= 1,
	DECO_RIFLE			= 2,
	DECO_SHELLS			= 3,
	DECO_ROCKET			= 4,
	DECO_CELL			= 5,
	DECO_RAYGUN			= 6,
	DECO_UNLOAD			= 7,

	DECO_AIMED			= 8,
	DECO_AUTO			= 9,
	DECO_SNAP			= 10,
	DECO_MEDKIT			= 11,
	DECO_KNIFE			= 12,
	DECO_ALIEN			= 13,
	DECO_ARMOR			= 14,

	DECO_METAL			= 16,
	DECO_TECH			= 17,
	DECO_FUEL			= 18,

	DECO_NONE			= 31,
};

// Base class for button layouts.
class UIButtons
{
public:

	enum {
		ICON_DX				= 4,
		ICON_DY				= 4,
		DECO_DX				= 8,
		DECO_DY				= 4,
		MAX_ICONS			= 20,
		MAX_TEXT_LEN		= 12,
	};


	UIButtons( const Screenport& port );
	virtual ~UIButtons()	{}

	// Coordinates in pixels. Origin in the lower left.
	void SetOrigin( int x, int y )				{	origin.Set( x, y ); }
	const grinliz::Vector2I& GetOrigin() const	{	return origin; }

	void InitButtons( const int* icons, int nIcons );
	void SetButton( int index, int iconID );
	void SetDeco( int index, int decoID );

	void SetText( const char** text );
	void SetText( int index, const char* text );
	void SetText( int index, const char* text0, const char* text1 );

	const char* GetText( int index );

	// Set the alpha of non-text
	void SetAlpha( float alpha )		{ this->alpha = alpha; cacheValid = false; }

	void SetEnabled( int index, bool enabled );
	void SetHighLight( int index, bool highLight );

	void SetButtonSize( int dx, int dy )		{	if ( size.x != dx || size.y != dy ) {
														size.x = dx; size.y = dy; 
														cacheValid = false; 
													}
												}
	const grinliz::Vector2I& GetButtonSize() const	{	return size; }

	void SetPadding( int dx, int dy )			{	if ( pad.x != dx || pad.y != dy ) {
														pad.x = dx; pad.y = dy; 
														cacheValid = false;		
													}
												}
	const grinliz::Vector2I& GetPadding() const	{	return pad; }

	void CalcBounds( grinliz::Rectangle2I* _bounds ) {
		CalcButtons();
		_bounds->min.x = bounds.min.x + origin.x;
		_bounds->min.y = bounds.min.y + origin.y;
		_bounds->max.x = bounds.max.x + origin.x;
		_bounds->max.y = bounds.max.y + origin.y;
	}
	

	// returns the icon INDEX, or -1 if not clicked
	virtual int QueryTap( int x, int y ) = 0;
	void Draw();

protected:
	void PositionText( int index );
	virtual void CalcButtons() = 0;


	struct Icon
	{
		int						id;
		int						decoID;
		char					text0[MAX_TEXT_LEN];
		char					text1[MAX_TEXT_LEN];
		bool					enabled;
		bool					highLight;
		grinliz::Vector2I		textPos0;
		grinliz::Vector2I		textPos1;
	};
	const Texture* texture;
	const Texture* decoTexture;
	int nIcons;

	bool cacheValid;
	Screenport screenport;

	float alpha;
	grinliz::Vector2I		origin;
	grinliz::Vector2I		size;
	grinliz::Vector2I		pad;
	grinliz::Rectangle2I	bounds;

	Icon					icons[MAX_ICONS];
	grinliz::Vector2< S16 > pos[MAX_ICONS*4];
	grinliz::Vector2F		tex[MAX_ICONS*4];
	grinliz::Vector2F		texDeco[MAX_ICONS*4];
	grinliz::Vector4< U8 >	color[MAX_ICONS*4];
	grinliz::Vector4< U8 >	colorDeco[MAX_ICONS*4];

	U16						index[6*MAX_ICONS];
};


// Implements related, but individually positioned buttons.
class UIButtonGroup : public UIButtons
{
public:
	UIButtonGroup( const Screenport& port );
	virtual ~UIButtonGroup()	{}

	virtual int QueryTap( int x, int y );	

	void SetPos( int index, int x, int y );

protected:
	virtual void CalcButtons();
	grinliz::Vector2I	bPos[MAX_ICONS];
};


// implements buttons in a row/column grid
// ordering (ex: 2 col, 8 icons)
//  6 7
//  4 5
//  2 3
//	0 1
//
class UIButtonBox : public UIButtons
{
public:
	UIButtonBox( const Screenport& port );
	virtual ~UIButtonBox()	{}

	// Set the columns - rows will be added as needed.
	void SetColumns( int columns  )				{	if ( this->columns != columns ) {
														this->columns = columns; 
														cacheValid = false; 
													}
												}

	void CalcDimensions( int *x, int *y, int *w, int *h );

	int TopIndex( int x, int y ) {
		int dx = columns;
		int dy = nIcons / columns;
		return (dy-1-y)*dx+x;
	}
	int TopIndex( int i ) {
		int x = i%columns;
		int y = i/columns;
		return TopIndex( x, y );
	}

	virtual int QueryTap( int x, int y );	

protected:
	virtual void CalcButtons();
	int INV( int i ) { return nIcons-i-1; }

	int columns;
};


class UIRendering
{
public:
	static void DrawQuad(	const Screenport& screenport,
							const grinliz::Rectangle2I& location, 
							const grinliz::Rectangle2F& textureUV,
							const Texture* texture );
	static void GetDecoUV( int deco, grinliz::Rectangle2F* uv );
};

#endif // UIRENDERING_INCLUDED