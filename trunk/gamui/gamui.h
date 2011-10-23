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
#include <memory.h>
#include <string.h>

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
class UIItem;
class ToggleButton;
class TextLabel;


template < class T > 
class CDynArray
{
public:
	CDynArray()
	{
		vec = (T*)malloc( ALLOCATE*sizeof(T) );
		size = 0;
		cap = ALLOCATE;
	}
	~CDynArray() {
		free( vec );
	}

	T& operator[]( int i )				{ GAMUIASSERT( i>=0 && i<(int)size ); return vec[i]; }
	const T& operator[]( int i ) const	{ GAMUIASSERT( i>=0 && i<(int)size ); return vec[i]; }

	T* PushArr( int count ) {
		if ( size + count > cap ) {
			cap = (size+count) * 3 / 2;
			vec = (T*)realloc( vec, cap*sizeof(T) );
		}
		T* result = vec + size;
		size += count;
		return result;
	}

	int Size() const		{ return (int)size; }
	
	void Clear()			{ size = 0; }
	bool Empty() const		{ return size==0; }
	const T* Mem() const	{ return vec; }
	T* Mem()				{ return vec; }

private:
	enum { ALLOCATE=1000 };
	T* vec;
	unsigned size;
	unsigned cap;
};


/**
	The most basic unit of state. A set of vertices and indices are sent to the GPU with a given RenderAtom, which
	encapselates a state (renderState), texture (textureHandle), and coordinates. 
*/
struct RenderAtom 
{
	/// Creates a default that renders nothing.
	RenderAtom() : tx0( 0 ), ty0( 0 ), tx1( 0 ), ty1( 0 ), renderState( 0 ), textureHandle( 0 ), user( 0 ) {}
	
	RenderAtom( const void* _renderState, const void* _textureHandle, float _tx0, float _ty0, float _tx1, float _ty1 ) {
		Init( _renderState, _textureHandle, _tx0, _ty0, _tx1, _ty1 );
	}
	
	RenderAtom( const RenderAtom& rhs, const void* _renderState ) {
		Init( _renderState, rhs.textureHandle, rhs.tx0, rhs.ty0, rhs.tx1, rhs.ty1 );
	}

	void Init( const void* _renderState, const void* _textureHandle, float _tx0, float _ty0, float _tx1, float _ty1 ) {
		SetCoord( _tx0, _ty0, _tx1, _ty1 );
		renderState = (const void*)_renderState;
		textureHandle = (const void*) _textureHandle;
		user = 0;
	}

	bool Equal( const RenderAtom& atom ) const {
		return (    tx0 == atom.tx0
			     && ty0 == atom.ty0
				 && tx1 == atom.tx1
				 && ty1 == atom.ty1
				 && renderState == atom.renderState
				 && textureHandle == atom.textureHandle );
	}

	/*  These methods are crazy useful. Don't work with Android compiler, which doesn't seem to like template functions at all. Grr.
	/// Copy constructor that allows a different renderState. It's often handy to render the same texture in different states.
	template <class T > RenderAtom( const RenderAtom& rhs, const T _renderState ) {
		Init( _renderState, rhs.textureHandle, rhs.tx0, rhs.ty0, rhs.tx1, rhs.ty1, rhs.srcWidth, rhs.srcHeight );
	}

	/// Constructor with all parameters.
	template <class T0, class T1 > RenderAtom( const T0 _renderState, const T1 _textureHandle, float _tx0, float _ty0, float _tx1, float _ty1, float _srcWidth, float _srcHeight ) {
		GAMUIASSERT( sizeof( T0 ) <= sizeof( const void* ) );
		GAMUIASSERT( sizeof( T1 ) <= sizeof( const void* ) );
		Init( (const void*) _renderState, (const void*) _textureHandle, _tx0, _ty0, _tx1, _ty1, _srcWidth, _srcHeight );
	}

	/// Initialize - works just like the constructor.
	template <class T0, class T1 > void Init( const T0 _renderState, const T1 _textureHandle, float _tx0, float _ty0, float _tx1, float _ty1, float _srcWidth, float _srcHeight ) {
		GAMUIASSERT( sizeof( T0 ) <= sizeof( const void* ) );
		GAMUIASSERT( sizeof( T1 ) <= sizeof( const void* ) );
		SetCoord( _tx0, _ty0, _tx1, _ty1 );
		renderState = (const void*)_renderState;
		textureHandle = (const void*) _textureHandle;
		srcWidth = _srcWidth;
		srcHeight = _srcHeight;
	}
*/

	/// Utility method to set the texture coordinates.
	void SetCoord( float _tx0, float _ty0, float _tx1, float _ty1 ) {
		tx0 = _tx0;
		ty0 = _ty0;
		tx1 = _tx1;
		ty1 = _ty1;
	}

	float tx0, ty0, tx1, ty1;		///< Texture coordinates to use within the atom.
	//float srcWidth, srcHeight;		///< The width and height of the sub-image (as defined by tx0, etc.) Used to know the "natural" size, and how to scale.

	const void* renderState;		///< Application defined render state.
	const void* textureHandle;		///< Application defined texture handle, noting that 0 (null) is assumed to be non-rendering.

	void* user;						///< ignored by gamui
};


/**
	Main class of the Gamui system. All Items and objects are attached to an instance of 
	Gamui, which does rendering, hit testing, and organization.

	Coordinate system assumed by Gamui.

	Screen coordinates:
	+--- x
	|
	|
	y

	Texture coordinates:
	ty
	|
	|
	+---tx

	Note that you can use any coordinate system you want, this is only what Gamui assumes. 
	Gamui can be used to draw map overlays for instance, if projected into a 3D plane.
*/
class Gamui
{
public:
	enum {
		LEVEL_BACKGROUND = 0,
		LEVEL_FOREGROUND = 1,
		LEVEL_DECO		 = 2,
		LEVEL_TEXT		 = 3
	};

	/// Description of a vertex used by Gamui.
	struct Vertex {
		enum {
			POS_OFFSET = 0,
			TEX_OFFSET = 8
		};
		float x;
		float y;
		float tx;
		float ty;

		void Set( float _x, float _y, float _tx, float _ty ) { x = _x; y = _y; tx = _tx; ty = _ty; }
	};

	/// Construct and Init later.
	Gamui();
	/// Constructor
	Gamui(	IGamuiRenderer* renderer,
			const RenderAtom& textEnabled, 
			const RenderAtom& textDisabled,
			IGamuiText* iText );
	~Gamui();

	/// Required if the default constructor used.
	void Init(	IGamuiRenderer* renderer,
				const RenderAtom& textEnabled, 
				const RenderAtom& textDisabled,
				IGamuiText* iText );

	// normally not called by user code.
	void Add( UIItem* item );
	// normally not called by user code.
	void Remove( UIItem* item );
	void OrderChanged() { m_orderChanged = true; m_modified = true; }
	void Modify()		{ m_modified = true; }

	/// Call to begin the rendering pass and commit all the UIItems to the display.
	void Render();

	RenderAtom* GetTextAtom() 				{ return &m_textAtomEnabled; }
	RenderAtom* GetDisabledTextAtom()		{ return &m_textAtomDisabled; }

	IGamuiText* GetTextInterface() const	{ return m_iText; }
	void SetTextHeight( float h )			{ m_textHeight = h; }
	float GetTextHeight() const				{ return m_textHeight; }

	/** Feed touch/mouse events to Gamui. You should use TapDown/TapUp as a pair, OR just use Tap. TapDown/Up
		is richer, but you need device support. (Mice certainly, fingers possibly.) 
		* TapDown return the item tapped on. This does not activate anything except capture.
		* TapUp returns if the item was activated (that is, the up and down item are the same.)
	*/
	void TapDown( float x, float y );		
	const UIItem* TapUp( float x, float y );					///< Used as a pair with TapDown
	void TapCancel();											///< Cancel a tap (generally in response to the OS)
	const UIItem* TapCaptured() const { return m_itemTapped; }	///< The item that received the TapDown, while in move.
	const UIItem* Tap( float x, float y );						///< Used to send events on systems that have a simple tap without up/down symantics.

	/** During (or after) a TapUp, this will return the starting object and ending object of the drag. One
		or both may be null, and it is commonly the same object.
	*/
	void GetDragPair( const UIItem** start, const UIItem** end )		{ *start = m_dragStart; *end = m_dragEnd; }

	/** Utility function to layout a grid of items.
		@param item			An array of item pointers to arrange.
		@param nItem		Number of items in the array
		@param cx			Number of columns.
		@param cy			Number of rows.
		@param originX		Upper left of the layout.
		@param originY		Upper left of the layout.
		@param tableWidth	Width of the entire table (items will be positioned to stay within this size.)
		@param tableHeight	Height of the entire table (items will be positioned to stay within this size.)
	*/
	static void Layout(	UIItem** item, int nItem,
						int cx, int cy,
						float originX, float originY,
						float tableWidth, float tableHeight );

	static void Layout( UIItem** item, int nItem,
						int columns,
						float originX, float originY );

	void LayoutTextBlock(	const char* text,
							TextLabel* textLabels, int nTextLabels,
							float originX, float originY,
							float width );

private:
	static int SortItems( const void* a, const void* b );

	UIItem*							m_itemTapped;
	RenderAtom						m_textAtomEnabled;
	RenderAtom						m_textAtomDisabled;
	IGamuiRenderer*					m_iRenderer;
	IGamuiText*						m_iText;

	bool			m_orderChanged;
	bool			m_modified;
	UIItem**		m_itemArr;
	int				m_nItems;
	int				m_nItemsAllocated;
	const UIItem*	m_dragStart;
	const UIItem*	m_dragEnd;
	float			m_textHeight;

	struct State {
		uint16_t	vertexStart;
		uint16_t	nVertex;
		uint16_t	indexStart;
		uint16_t	nIndex;
		const void* renderState;
		const void* textureHandle;
	};

	CDynArray< State >				m_stateBuffer;
	CDynArray< uint16_t >			m_indexBuffer;
	CDynArray< Vertex >				m_vertexBuffer;
};


class IGamuiRenderer
{
public:
	virtual ~IGamuiRenderer()	{}
	virtual void BeginRender() = 0;
	virtual void EndRender() = 0;

	virtual void BeginRenderState( const void* renderState ) = 0;
	virtual void BeginTexture( const void* textureHandle ) = 0;
	virtual void Render( const void* renderState, const void* textureHandle, int nIndex, const uint16_t* index, int nVertex, const Gamui::Vertex* vertex ) = 0;
};


class IGamuiText
{
public:
	struct GlyphMetrics {
		float advance;
		float x, y, w, h;			// position, based at 0,0
		float tx0, ty0, tx1, ty1;	// texture coordinates of the glyph
	};

	virtual ~IGamuiText()	{}
	virtual void GamuiGlyph( int c0, int cPrev,	// character, prev character
							 float height,
							 gamui::IGamuiText::GlyphMetrics* metric ) = 0;
};


class UIItem
{
public:
	enum {
		DEFAULT_SIZE = 64
	};

	virtual void SetPos( float x, float y )		
	{ 
		if ( m_x != x || m_y != y ) {
			m_x = x; m_y = y; 
			Modify(); 
		}
	}
	void SetPos( const float* x )				{ SetPos( x[0], x[1] ); }
	void SetCenterPos( float x, float y )		{ SetPos( x - Width()*0.5f, y - Height()*0.5f ); }
	
	float X() const								{ return m_x; }
	float Y() const								{ return m_y; }
	virtual float Width() const = 0;
	virtual float Height() const = 0;

	int Level() const							{ return m_level; }

	virtual void SetEnabled( bool enabled )		{}		// does nothing for many items.
	bool Enabled() const						{ return m_enabled; }
	virtual void SetVisible( bool visible )		
	{ 
		if ( m_visible != visible ) { 
			m_visible = visible; 
			Modify();
		}
	}
	bool Visible() const						{ return m_visible; }
	
	void SetLevel( int level )					
	{
		if ( m_level != level ) {
			m_level = level; 
			OrderChanged();
			Modify();
		}
	}

	void SetRotationX( float degrees )			{ if ( m_rotationX != degrees ) { m_rotationX = degrees; Modify(); } }
	void SetRotationY( float degrees )			{ if ( m_rotationY != degrees ) { m_rotationY = degrees; Modify(); } }
	void SetRotationZ( float degrees )			{ if ( m_rotationZ != degrees ) { m_rotationZ = degrees; Modify(); } }
	float RotationX() const						{ return m_rotationX; }
	float RotationY() const						{ return m_rotationY; }
	float RotationZ() const						{ return m_rotationZ; }

	enum TapAction {
		TAP_DOWN,
		TAP_UP,
		TAP_CANCEL
	};
	
	virtual bool CanHandleTap()											{ return false; }
	virtual bool HandleTap( TapAction action, float x, float y )		{ return false; }

	virtual const RenderAtom* GetRenderAtom() const = 0;
	virtual bool DoLayout() = 0;
	virtual void Queue( CDynArray< uint16_t > *index, CDynArray< Gamui::Vertex > *vertex ) = 0;

	virtual void Clear()	{ m_gamui = 0; }

private:
	UIItem( const UIItem& );			// private, not implemented.
	void operator=( const UIItem& );	// private, not implemented.

	float m_x;
	float m_y;
	int m_level;
	bool m_visible;
	float m_rotationX;
	float m_rotationY;
	float m_rotationZ;

protected:
	template <class T> T Min( T a, T b ) const		{ return a<b ? a : b; }
	template <class T> T Max( T a, T b ) const		{ return a>b ? a : b; }
	float Mean( float a, float b ) const			{ return (a+b)*0.5f; }
	static Gamui::Vertex* PushQuad( CDynArray< uint16_t > *index, CDynArray< Gamui::Vertex > *vertex );
	void Modify()		{ if ( m_gamui ) m_gamui->Modify(); }
	void OrderChanged()	{ if ( m_gamui ) m_gamui->OrderChanged(); }

	void ApplyRotation( int nVertex, Gamui::Vertex* vertex );

	UIItem( int level );
	virtual ~UIItem();

	Gamui* m_gamui;
	bool m_enabled;
};


class TextLabel : public UIItem
{
public:
	TextLabel();
	TextLabel( Gamui* );
	virtual ~TextLabel();

	void Init( Gamui* );

	virtual float Width() const;
	virtual float Height() const;

	void SetText( const char* t );
	void SetText( const char* start, const char* end );	///< Adds text from [start,end)

	const char* GetText() const;
	void ClearText();
	virtual void SetEnabled( bool enabled )		
	{ 
		if ( m_enabled != enabled ) {
			m_enabled = enabled; 
			Modify(); 
		}
	}

	virtual const RenderAtom* GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( CDynArray< uint16_t > *index, CDynArray< Gamui::Vertex > *vertex );

private:
	void CalcSize( float* width, float* height ) const;

	enum { ALLOCATED = 16 };
	char  m_buf[ALLOCATED];
	char* m_str;
	int	  m_allocated;

	mutable float m_width;
	mutable float m_height;
};


class TextBox : public UIItem
{
public:
	TextBox();
	~TextBox();

	void Init( Gamui* );

	void SetSize( float width, float height )	{ m_width = width; m_height = height; m_needsLayout = true; Modify(); }
	virtual float Width() const					{ return m_width; }
	virtual float Height() const				{ return m_height; }

	void SetText( const char* t )				{ m_storage.SetText( t ); m_needsLayout = true; Modify(); }

	const char* GetText() const					{ return m_storage.GetText(); }
	void ClearText()							{ m_storage.ClearText(); m_needsLayout = true; Modify(); }
	virtual void SetEnabled( bool enabled )		{ UIItem::SetEnabled( enabled ); m_needsLayout = true; Modify(); }
	virtual void SetVisible( bool visible )		{ UIItem::SetVisible( visible ); m_needsLayout = true; Modify(); }

	virtual const RenderAtom* GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( CDynArray< uint16_t > *index, CDynArray< Gamui::Vertex > *vertex );

private:
	bool		m_needsLayout;
	float		m_width;
	float		m_height;
	TextLabel	m_storage;
	TextLabel*	m_textLabelArr;
	int			m_lines;
};


class Image : public UIItem
{
public:
	Image();
	Image( Gamui*, const RenderAtom& atom, bool foreground );
	virtual ~Image();
	void Init( Gamui*, const RenderAtom& atom, bool foreground );

	void SetAtom( const RenderAtom& atom );
	void SetSlice( bool enable );

	void SetSize( float width, float height )							
	{ 
		if ( m_width != width || m_height != height ) {
			m_width = width; 
			m_height = height; 
			Modify(); 
		}
	}
	void SetForeground( bool foreground );

	virtual float Width() const											{ return m_width; }
	virtual float Height() const										{ return m_height; }

	virtual const RenderAtom* GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( CDynArray< uint16_t > *index, CDynArray< Gamui::Vertex > *vertex );

private:
	RenderAtom m_atom;
	float m_width;
	float m_height;
	bool m_slice;
};


class TiledImageBase : public UIItem
{
public:

	virtual ~TiledImageBase();
	void Init( Gamui* );

	void SetTile( int x, int y, const RenderAtom& atom );

	void SetSize( float width, float height )							{ m_width = width; m_height = height; Modify(); }
	void SetForeground( bool foreground );

	virtual float Width() const											{ return m_width; }
	virtual float Height() const										{ return m_height; }
	void Clear();

	virtual const RenderAtom* GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( CDynArray< uint16_t > *index, CDynArray< Gamui::Vertex > *vertex );

	virtual int CX() const = 0;
	virtual int CY() const = 0;

protected:
	TiledImageBase();
	TiledImageBase( Gamui* );

	virtual int8_t* Mem() = 0;

private:
	enum {
		MAX_ATOMS = 32
	};
	RenderAtom m_atom[MAX_ATOMS];
	float m_width;
	float m_height;
};

template< int _CX, int _CY > 
class TiledImage : public TiledImageBase
{
public:
	TiledImage() : TiledImageBase() {}
	TiledImage( Gamui* g ) : TiledImageBase( g )	{}

	virtual ~TiledImage() {}

	virtual int CX() const			{ return _CX; }
	virtual int CY() const			{ return _CY; }

protected:
	virtual int8_t* Mem()			{ return mem; }

private:
	int8_t mem[_CX*_CY];
};


struct ButtonLook
{
	ButtonLook()	{};
	ButtonLook( const RenderAtom& _atomUpEnabled,
				const RenderAtom& _atomUpDisabled,
				const RenderAtom& _atomDownEnabled,
				const RenderAtom& _atomDownDisabled ) 
		: atomUpEnabled( _atomUpEnabled ),
		  atomUpDisabled( _atomUpDisabled ),
		  atomDownEnabled( _atomDownEnabled ),
		  atomDownDisabled( _atomDownDisabled )
	{}
	void Init( const RenderAtom& _atomUpEnabled, const RenderAtom& _atomUpDisabled, const RenderAtom& _atomDownEnabled, const RenderAtom& _atomDownDisabled ) {
		atomUpEnabled = _atomUpEnabled;
		atomUpDisabled = _atomUpDisabled;
		atomDownEnabled = _atomDownEnabled;
		atomDownDisabled = _atomDownDisabled;
	}

	RenderAtom atomUpEnabled;
	RenderAtom atomUpDisabled;
	RenderAtom atomDownEnabled;
	RenderAtom atomDownDisabled;
};

class Button : public UIItem
{
public:

	virtual float Width() const;
	virtual float Height() const;

	virtual void SetPos( float x, float y );
	void SetSize( float width, float height );

	virtual void SetEnabled( bool enabled );

	bool Up() const		{ return m_up; }
	bool Down() const	{ return !m_up; }
	void SetDeco( const RenderAtom& atom, const RenderAtom& atomD )			{ m_atoms[DECO] = atom; m_atoms[DECO_D] = atomD; SetState(); Modify(); }
	void SetDecoRotationY( float degrees )									{ m_deco.SetRotationY( degrees ); }

	void SetText( const char* text );
	const char* GetText() const { return m_label[0].GetText(); }
	void SetText2( const char* text );
	const char* GetText2() const { return m_label[1].GetText(); }

	enum {
		CENTER, LEFT, RIGHT
	};
	void SetTextLayout( int alignment, float dx=0.0f, float dy=0.0f )		{ m_textLayout = alignment; m_textDX = dx; m_textDY = dy; Modify(); }
	void SetDecoLayout( int alignment, float dx=0.0f, float dy=0.0f )		{ m_decoLayout = alignment; m_decoDX = dx; m_decoDY = dy; Modify(); }

	virtual const RenderAtom* GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( CDynArray< uint16_t > *index, CDynArray< Gamui::Vertex > *vertex );


protected:

	void Init( Gamui* gamui, const ButtonLook& look ) {
		RenderAtom nullAtom;
		Init( gamui, look.atomUpEnabled, look.atomUpDisabled, look.atomDownEnabled, look.atomDownDisabled, nullAtom, nullAtom );
	}

	void Init(	Gamui*,
				const RenderAtom& atomUpEnabled,
				const RenderAtom& atomUpDisabled,
				const RenderAtom& atomDownEnabled,
				const RenderAtom& atomDownDisabled,
				const RenderAtom& decoEnabled, 
				const RenderAtom& decoDisabled );

	Button();
	virtual ~Button()	{}

	bool m_up;
	void SetState();


private:

	void PositionChildren();

	enum {
		UP,
		UP_D,
		DOWN,
		DOWN_D,
		DECO,
		DECO_D,
		COUNT
	};
	RenderAtom m_atoms[COUNT];
	
	Image		m_face;
	Image		m_deco;

	bool		m_usingText1;
	int			m_textLayout;
	int			m_decoLayout;
	float		m_textDX;
	float		m_textDY;
	float		m_decoDX;
	float		m_decoDY;	
	TextLabel	m_label[2];
};


class PushButton : public Button
{
public:
	PushButton() : Button()	{}
	PushButton( Gamui* gamui,
				const RenderAtom& atomUpEnabled,
				const RenderAtom& atomUpDisabled,
				const RenderAtom& atomDownEnabled,
				const RenderAtom& atomDownDisabled,
				const RenderAtom& decoEnabled, 
				const RenderAtom& decoDisabled) : Button()	
	{
		Init( gamui, atomUpEnabled, atomUpDisabled, atomDownEnabled, atomDownDisabled, decoEnabled, decoDisabled );
	}

	PushButton( Gamui* gamui, const ButtonLook& look ) : Button()
	{
		RenderAtom nullAtom;
		Init( gamui, look.atomUpEnabled, look.atomUpDisabled, look.atomDownEnabled, look.atomDownDisabled, nullAtom, nullAtom );
	}


	void Init( Gamui* gamui, const ButtonLook& look ) {
		RenderAtom nullAtom;
		Init( gamui, look.atomUpEnabled, look.atomUpDisabled, look.atomDownEnabled, look.atomDownDisabled, nullAtom, nullAtom );
	}

	void Init(	Gamui* gamui,
				const RenderAtom& atomUpEnabled,
				const RenderAtom& atomUpDisabled,
				const RenderAtom& atomDownEnabled,
				const RenderAtom& atomDownDisabled,
				const RenderAtom& decoEnabled, 
				const RenderAtom& decoDisabled )
	{
		Button::Init( gamui, atomUpEnabled, atomUpDisabled, atomDownEnabled, atomDownDisabled, decoEnabled, decoDisabled );
	}

	virtual ~PushButton()	{}

	virtual bool CanHandleTap()											{ return true; }
	virtual bool HandleTap( TapAction action, float x, float y );
};


class ToggleButton : public Button
{
public:
	ToggleButton() : Button(), m_next( 0 ), m_prev( 0 ), m_wasUp( true )		{}
	ToggleButton(	Gamui* gamui,
					const RenderAtom& atomUpEnabled,
					const RenderAtom& atomUpDisabled,
					const RenderAtom& atomDownEnabled,
					const RenderAtom& atomDownDisabled,
					const RenderAtom& decoEnabled, 
					const RenderAtom& decoDisabled) : Button(), m_next( 0 ), m_prev( 0 ), m_wasUp( true )
	{
		Button::Init( gamui, atomUpEnabled, atomUpDisabled, atomDownEnabled, atomDownDisabled, decoEnabled, decoDisabled );
		m_prev = m_next = this;
	}

	ToggleButton( Gamui* gamui, const ButtonLook& look ) : Button(), m_next( 0 ), m_prev( 0 ), m_wasUp( true )
	{
		RenderAtom nullAtom;
		Button::Init( gamui, look.atomUpEnabled, look.atomUpDisabled, look.atomDownEnabled, look.atomDownDisabled, nullAtom, nullAtom );
		m_prev = m_next = this;
	}

	void Init( Gamui* gamui, const ButtonLook& look ) {
		RenderAtom nullAtom;
		Button::Init( gamui, look.atomUpEnabled, look.atomUpDisabled, look.atomDownEnabled, look.atomDownDisabled, nullAtom, nullAtom );
		m_prev = m_next = this;
	}

	void Init(	Gamui* gamui,
				const RenderAtom& atomUpEnabled,
				const RenderAtom& atomUpDisabled,
				const RenderAtom& atomDownEnabled,
				const RenderAtom& atomDownDisabled,
				const RenderAtom& decoEnabled, 
				const RenderAtom& decoDisabled )
	{
		Button::Init( gamui, atomUpEnabled, atomUpDisabled, atomDownEnabled, atomDownDisabled, decoEnabled, decoDisabled );
		m_prev = m_next = this;
	}

	virtual ~ToggleButton()		{ RemoveFromToggleGroup(); }

	virtual bool CanHandleTap()											{ return true; }
	virtual bool HandleTap(	TapAction action, float x, float y );

	void AddToToggleGroup( ToggleButton* );
	void RemoveFromToggleGroup();
	bool InToggleGroup();

	void SetUp()								{ m_up = true; SetState(); ProcessToggleGroup(); }
	void SetDown()								{ m_up = false; SetState(); ProcessToggleGroup(); }

	virtual void Clear();

private:
	void ProcessToggleGroup();
	void PriSetUp()								{ m_up = true; SetState(); }
	void PriSetDown()							{ m_up = false; SetState(); }

	ToggleButton* m_next;
	ToggleButton* m_prev;
	bool m_wasUp;
};


class DigitalBar : public UIItem
{
public:
	DigitalBar();
	DigitalBar( Gamui* gamui,
				int nTicks,

				const RenderAtom& atom0,
				const RenderAtom& atom1,
				const RenderAtom& atom2 ) 
		: UIItem( Gamui::LEVEL_FOREGROUND ), SPACING( 0.1f )
	{
		Init( gamui, nTicks, atom0, atom1, atom2 );
	}
	virtual ~DigitalBar()		{}

	void Init(	Gamui* gamui, 
				int nTicks,
				const RenderAtom& atom0,
				const RenderAtom& atom1,
				const RenderAtom& atom2 );

	void SetRange( float t0, float t1 );
	void SetRange0( float t0 )				{ SetRange( t0, m_t1 ); }
	void SetRange1( float t1 )				{ SetRange( m_t0, t1 ); }
	float GetRange0() const					{ return m_t0; }
	float GetRange1() const					{ return m_t1; }

	virtual float Width() const;
	virtual float Height() const;
	virtual void SetVisible( bool visible );
	void SetSize( float w, float h );

	virtual const RenderAtom* GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( CDynArray< uint16_t > *index, CDynArray< Gamui::Vertex > *vertex );

private:
	enum { MAX_TICKS = 10 };
	const float	SPACING;
	int			m_nTicks;
	float		m_t0, m_t1;
	RenderAtom	m_atom[3];
	float		m_width, m_height;
	Image		m_image[MAX_TICKS];
};


class Matrix
{
  public:
	Matrix()		{	x[0] = x[5] = x[10] = x[15] = 1.0f;
						x[1] = x[2] = x[3] = x[4] = x[6] = x[7] = x[8] = x[9] = x[11] = x[12] = x[13] = x[14] = 0.0f; 
					}

	void SetTranslation( float _x, float _y, float _z )		{	x[12] = _x;	x[13] = _y;	x[14] = _z;	}

	void SetXRotation( float thetaDegree );
	void SetYRotation( float thetaDegree );
	void SetZRotation( float thetaDegree );

	float x[16];

	inline friend Matrix operator*( const Matrix& a, const Matrix& b )
	{	
		Matrix result;
		MultMatrix( a, b, &result );
		return result;
	}


	inline friend void MultMatrix( const Matrix& a, const Matrix& b, Matrix* c )
	{
	// This does not support the target being one of the sources.
		GAMUIASSERT( c != &a && c != &b && &a != &b );

		// The counters are rows and columns of 'c'
		for( int i=0; i<4; ++i ) 
		{
			for( int j=0; j<4; ++j ) 
			{
				// for c:
				//	j increments the row
				//	i increments the column
				*( c->x + i +4*j )	=   a.x[i+0]  * b.x[j*4+0] 
									  + a.x[i+4]  * b.x[j*4+1] 
									  + a.x[i+8]  * b.x[j*4+2] 
									  + a.x[i+12] * b.x[j*4+3];
			}
		}
	}

	inline friend void MultMatrix( const Matrix& m, const float* v, int components, float* out )
	{
		float w = 1.0f;
		for( int i=0; i<components; ++i )
		{
			*( out + i )	=		m.x[i+0]  * v[0] 
								  + m.x[i+4]  * v[1]
								  + m.x[i+8]  * v[2]
								  + m.x[i+12] * w;
		}
	}
};


};


#endif // GAMUI_INCLUDED
