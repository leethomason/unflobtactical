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
#include "../grinliz/glstringutil.h"


enum {
	ICON_GREEN_BUTTON	= 0,
	ICON_BLUE_BUTTON	= 1,
	ICON_RED_BUTTON		= 2,
	ICON_TRANS_RED		= 4,
	ICON_TRANS_BLUE		= 5,
	ICON_TRANS_YELLOW	= 6,
	ICON_TRANS_ORANGE	= 7,
	ICON_GREEN_WALK_MARK	= 12,
	ICON_YELLOW_WALK_MARK	= 13,
	ICON_ORANGE_WALK_MARK	= 14,
	ICON_BLUE_WALK_MARK		= 15,
	ICON_NONE			= 3,

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
	DECO_BOOM			= 15,

	DECO_METAL			= 16,
	DECO_TECH			= 17,
	DECO_FUEL			= 18,
	DECO_SWAP			= 19,
	DECO_HELP			= 20,

	DECO_NONE			= 31,
};


class UIWidget
{
public:
	UIWidget( const Screenport& port );
	virtual ~UIWidget()								{}

	// Coordinates in pixels. Origin in the lower left.
	void SetOrigin( int x, int y )					{ origin.Set( x, y ); }
	const grinliz::Vector2I& GetOrigin() const		{ return origin; }

	virtual void Draw() = 0;

protected:
	struct Vertex2D
	{
		grinliz::Vector2F	pos;
		grinliz::Vector2F	tex;
	};

	grinliz::Vector2I		origin;
	const Screenport&		screenport;
};

/*
class UIMolecule
{
public:
	UIMolecule();
	~UIMolecule();
};
*/

/*	Draw a status/indicator/level bar. 
	The bar draws in 3 colors:
	minValue-value0		'green'/okay
	value0-value1		'red'/problem
	value1-maxValue		'black'/no value
*/
class UIBar : public UIWidget
{
public:
	UIBar( const Screenport& port );
	~UIBar();

	void SetSize( int dx, int dy );
	void SetSteps( int steps );
	
	void SetRange( int minValue, int maxValue );

	void SetValue0( int value0 );
	int GetValue0() const			{ return value0; }
	void SetValue1( int value1 );
	int GetValue1() const			{ return value1; }

	virtual void Draw();

private:
	enum { MAX_STEPS = 10 };

	bool valid;
	int nSteps;
	int minValue, maxValue, value0, value1;
	grinliz::Vector2I size;

	Texture* texture;

	U16		 index[ MAX_STEPS*6 ];
	Vertex2D vertex[ MAX_STEPS*4 ];
};


class UIImage : public UIWidget
{
public:
	UIImage( const Screenport& port );
	virtual ~UIImage();

	void Init( Texture* texture, int w, int h );
	void SetCenter( int x, int y )								{ SetOrigin( x-Width()/2, y-Height()/2 ); }
	void SetYRotation( float yRot )								{ this->yRot = yRot; }
	void SetZRotation( float zRot )								{ this->zRot = zRot; }
	void SetTexCoord( float tx, float ty, float sx, float sy )	{
		texCoord.Set( tx, ty, tx+sx, ty+sy );
	}
	virtual void Draw();
	int Width() const	{ return w; }
	int Height() const	{ return h; }

protected:
	int w, h;
	float yRot, zRot;
	grinliz::Rectangle2F texCoord;
	Texture* texture;
};


// Base class for button layouts.
class UIButtons : public UIWidget
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

	void InitButtons( const int* icons, int nIcons );
	void SetButton( int index, int iconID );
	int GetButton( int index ) const;
	void SetDeco( int index, int decoID );

	void SetText( const char** text );
	void SetText( int index, const char* text );
	void SetText( int index, const char* text0, const char* text1 );
	
	enum {
		LAYOUT_CENTER,
		LAYOUT_LEFT,
		LAYOUT_RIGHT
	};
	void SetTextLayout( int layout )	{ this->textLayout = layout; cacheValid = false; }

	const char* GetText( int index, int which=0 );

	// Set the alpha of non-text
	void SetAlpha( float alpha )		{ this->alpha = alpha; cacheValid = false; }

	void SetEnabled( int index, bool enabled );
	void SetHighLight( int index, bool highLight );
	bool GetHighLight( int index )				{ GLASSERT( index >=0 && index < nIcons );
												  return icons[index].highLight;
												}

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

	void CalcBounds( int* x, int* y, int* w, int* h ) {
		grinliz::Rectangle2I b;
		CalcBounds( &b );

		if ( x )
			*x = b.min.x;
		if ( y )
			*y = b.min.y;
		if ( w )
			*w = b.Width();
		if ( h )
			*h = b.Height();
	}

	virtual void CalcButtonBounds( int index, grinliz::Rectangle2I* _bounds ) = 0;

	// returns the icon INDEX, or -1 if not clicked
	virtual int QueryTap( int x, int y ) = 0;
	virtual void Draw();

protected:
	void PositionText( int index );
	virtual void CalcButtons() = 0;


	struct Icon
	{
		int						id;
		int						decoID;
		grinliz::CStr< MAX_TEXT_LEN > text0;
		grinliz::CStr< MAX_TEXT_LEN > text1;
		bool					enabled;
		bool					highLight;
		grinliz::Vector2I		textPos0;
		grinliz::Vector2I		textPos1;
	};
	Texture* texture;
	Texture* decoTexture;
	int nIcons;

	bool cacheValid;
	int textLayout;

	float alpha;
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
	int						nIndexSelected;
	U16						indexSelected[6*MAX_ICONS];
};


// Implements related, but individually positioned buttons.
class UIButtonGroup : public UIButtons
{
public:
	UIButtonGroup( const Screenport& port );
	virtual ~UIButtonGroup()	{}

	virtual int QueryTap( int x, int y );	

	void SetPos( int index, int x, int y );
	void SetItemSize( int index, int dx, int dy );

	void SetBG( int index, int x, int y, int iconID, int decoID, const char* text, bool highLight );
	virtual void CalcButtonBounds( int index, grinliz::Rectangle2I* _bounds );

protected:
	virtual void CalcButtons();
	grinliz::Vector2I	bPos[MAX_ICONS];
	grinliz::Vector2I	bSize[MAX_ICONS];
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
	virtual void CalcButtonBounds( int index, grinliz::Rectangle2I* _bounds );

protected:
	virtual void CalcButtons();
	int INV( int i ) { return nIcons-i-1; }

	int columns;
};


class UIRendering
{
public:
//	static void DrawQuad(	const Screenport& screenport,
//							const grinliz::Rectangle2I& location, 
//							const grinliz::Rectangle2F& textureUV,
//							const Texture* texture );
	static void GetDecoUV( int deco, grinliz::Rectangle2F* uv );
};

#endif // UIRENDERING_INCLUDED