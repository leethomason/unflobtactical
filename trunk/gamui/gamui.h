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
	std::vector< int16_t >			m_indexBuffer;
	std::vector< Gamui::Vertex >	m_vertexBuffer;

	std::vector< GamItem* > m_itemArr;
};


class IGamuiRenderer
{
public:
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
	virtual void GamuiGlyph( int c, GlyphMetrics* metric ) = 0;
//	virtual void GamuiGlyphAdvance( int c, int* advance ) = 0;
};


class GamItem
{
public:
	GamItem();
	virtual ~GamItem()					{}

	void SetPos( float x, float y )		{ m_x = x; m_y = y; }
	void SetPos( const float* x )		{ m_x = x[0]; m_y = x[1]; }
	float X() const						{ return m_x; }
	float Y() const						{ return m_y; }

	int Level() const					{ return m_level; }
	void SetLevel( int level );

	virtual const RenderAtom* GetCurrentAtom() const = 0;
	
	void Attach( Gamui* );
	virtual void Queue( std::vector< int16_t >* index, std::vector< Gamui::Vertex >* vertex ) = 0;

private:
	float m_x;
	float m_y;

protected:
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
	virtual void Queue( std::vector< int16_t >* index, std::vector< Gamui::Vertex >* vertex );

private:

	enum { ALLOCATE = 16 };
	union {
		char buf[ALLOCATE];
		std::string* str;
	};
};



};


#endif // GAMUI_INCLUDED