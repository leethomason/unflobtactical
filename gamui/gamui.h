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

#ifndef GAMUI_INCLUDED
#define GAMUI_INCLUDED

#include <stdint.h>

#include <string>
#include <vector>


#if defined( _DEBUG ) || defined( DEBUG )
#	if defined( _MSC_VER )
#		define GAMUIASSERT( x )		if ( !(x)) { _asm { int 3 } }
#	else
#		include <assert.h>
#		define GAMUIASSERT assert
#	endif
#else
#	define GAMUIASSERT( x ) {}
#endif


namespace gamui
{

class Gamui;
class IGamuiRenderer;
class IGamuiText;
class GamItem;

struct RenderAtom 
{
	// sorting fields
	const void* renderState;
	const void* textureHandle;

	// non-sorting
	float tx0, ty0, tx1, ty1;
	void SetCoord( float _tx0, float _ty0, float _tx1, float _ty1 ) { tx0 = _tx0; ty0 = _ty0; tx1 = _tx1; ty1 = _ty1; }

	// ignored
	void* user;
};


class Gamui
{
public:
	enum {
		BACKGROUND_LEVEL = 0x10,
		ITEM_LEVEL		 = 0x20,
		TEXT_LEVEL		 = 0x30
	};

	struct Vertex {
		float x;
		float y;
		float tx;
		float ty;

		void Set( float _x, float _y, float _tx, float _ty ) { x = _x; y = _y; tx = _tx; ty = _ty; }
	};

	Gamui();
	~Gamui();

	void Add( GamItem* item );
	void Remove( GamItem* item );

	void Render( IGamuiRenderer* renderer );

	void InitText(	const RenderAtom* atom, 
					int pixelHeight,
					IGamuiText* iText );

	const RenderAtom* GetTextAtom() const	{ return m_textAtom; }
	IGamuiText* GetTextInterface() const	{ return m_iText; }
	int GetTextHeight() const				{ return m_textHeight; }

private:
	static bool SortItems( const GamItem* a, const GamItem* b );

	const RenderAtom*				m_textAtom;
	IGamuiText*						m_iText;
	int								m_textHeight;
	enum { INDEX_SIZE = 6000,
		   VERTEX_SIZE = 4000 };
	int16_t							m_indexBuffer[INDEX_SIZE];
	Vertex							m_vertexBuffer[VERTEX_SIZE];

	std::vector< GamItem* > m_itemArr;
};


class IGamuiRenderer
{
public:
	virtual ~IGamuiRenderer()	{}
	virtual void BeginRender() = 0;
	virtual void EndRender() = 0;

	virtual void BeginRenderState( const void* renderState ) = 0;
	virtual void BeginTexture( const void* textureHandle ) = 0;
	virtual void Render( const void* renderState, const void* textureHandle, int nIndex, const int16_t* index, int nVertex, const Gamui::Vertex* vertex ) = 0;
};


class IGamuiText
{
public:
	struct GlyphMetrics {
		int advance;				// distance in pixels from this glyph to the next.
		int width;					// width of this glyph
		float tx0, ty0, tx1, ty1;	// texture coordinates of the glyph );
	};

	virtual ~IGamuiText()	{}
	virtual void GamuiGlyph( int c, GlyphMetrics* metric ) = 0;
};


/*
	Subclasses:
		x TextLabel
		- Bar
		- Image
		- Text table
		- Button
			- Toggle
			- Radio
*/
class GamItem
{
public:
	void SetPos( float x, float y )		{ m_x = x; m_y = y; }
	void SetPos( const float* x )		{ m_x = x[0]; m_y = x[1]; }
	float X() const						{ return m_x; }
	float Y() const						{ return m_y; }
	int Level() const					{ return m_level; }

	virtual const RenderAtom* GetCurrentAtom() const = 0;
	
	void Attach( Gamui* );
	virtual void Requires( int* indexNeeded, int* vertexNeeded ) = 0;
	virtual void Queue( int *nIndex, int16_t* index, int *nVertex, Gamui::Vertex* vertex ) = 0;

private:
	float m_x;
	float m_y;

protected:
	GamItem( int level );
	virtual ~GamItem()					{}

	Gamui* m_gamui;
	int m_level;
};


class TextLabel : public GamItem
{
public:
	TextLabel();
	virtual ~TextLabel();

	void SetText( const char* t );
	const char* GetText();
	void ClearText();

	virtual const RenderAtom* GetCurrentAtom() const;

	virtual void Requires( int* indexNeeded, int* vertexNeeded );
	virtual void Queue( int *nIndex, int16_t* index, int *nVertex, Gamui::Vertex* vertex );

private:

	enum { ALLOCATE = 16 };
	union {
		char buf[ALLOCATE];
		std::string* str;
	};
};


class Image : public GamItem
{
public:
	Image();
	virtual ~Image();

	void Init( const RenderAtom* atom );
	void SetSlice( bool enable, int srcWidth, int srcHeight, int x0, int y0, int x1, int y1 );

private:
	const RenderAtom* m_atom;
};



};


#endif // GAMUI_INCLUDED