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

#include "gamui.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

using namespace gamui;
using namespace std;

static const float PI = 3.1415926535897932384626433832795f;

void Matrix::SetXRotation( float theta )
{
	float cosTheta = cosf( theta*PI/180.0f );
	float sinTheta = sinf( theta*PI/180.0f );

	x[5] = cosTheta;
	x[6] = sinTheta;

	x[9] = -sinTheta;
	x[10] = cosTheta;
}


void Matrix::SetYRotation( float theta )
{
	float cosTheta = cosf( theta*PI/180.0f );
	float sinTheta = sinf( theta*PI/180.0f );

	// COLUMN 1
	x[0] = cosTheta;
	x[1] = 0.0f;
	x[2] = -sinTheta;
	
	// COLUMN 2
	x[4] = 0.0f;
	x[5] = 1.0f;
	x[6] = 0.0f;

	// COLUMN 3
	x[8] = sinTheta;
	x[9] = 0;
	x[10] = cosTheta;
}

void Matrix::SetZRotation( float theta )
{
	float cosTheta = cosf( theta*PI/180.0f );
	float sinTheta = sinf( theta*PI/180.0f );

	// COLUMN 1
	x[0] = cosTheta;
	x[1] = sinTheta;
	x[2] = 0.0f;
	
	// COLUMN 2
	x[4] = -sinTheta;
	x[5] = cosTheta;
	x[6] = 0.0f;

	// COLUMN 3
	x[8] = 0.0f;
	x[9] = 0.0f;
	x[10] = 1.0f;
}



UIItem::UIItem( int p_level ) 
	: m_x( 0 ),
	  m_y( 0 ),
	  m_level( p_level ),
	  m_visible( true ),
	  m_rotationX( 0 ),
	  m_rotationY( 0 ),
	  m_rotationZ( 0 ),
	  m_gamui( 0 ),
	  m_enabled( true )
{}


UIItem::~UIItem()
{
	if ( m_gamui )
		m_gamui->Remove( this );
}


void UIItem::PushQuad( int *nIndex, int16_t* index, int base, int a, int b, int c, int d, int e, int f )
{
	index[(*nIndex)++] = base+a;
	index[(*nIndex)++] = base+b;
	index[(*nIndex)++] = base+c;
	index[(*nIndex)++] = base+d;
	index[(*nIndex)++] = base+e;
	index[(*nIndex)++] = base+f;
}


void UIItem::ApplyRotation( int nVertex, Gamui::Vertex* vertex )
{
	if ( m_rotationX != 0.0f || m_rotationY != 0.0f || m_rotationZ != 0.0f ) {
		Matrix m, mx, my, mz, t0, t1, temp;

		t0.SetTranslation( -(X()+Width()*0.5f), -(Y()+Height()*0.5f), 0 );
		t1.SetTranslation( (X()+Width()*0.5f), (Y()+Height()*0.5f), 0 );
		mx.SetXRotation( m_rotationX );
		my.SetYRotation( m_rotationY );
		mz.SetZRotation( m_rotationZ );

		m = t1 * mz * my * mx * t0;

		for( int i=0; i<nVertex; ++i ) {
			float in[3] = { vertex[i].x, vertex[i].y, 0 };
			MultMatrix( m, in, 2, &vertex[i].x );
		}
	}
}


TextLabel::TextLabel() : UIItem( Gamui::LEVEL_TEXT ),
	m_width( -1 ),
	m_height( -1 )
{
	m_str = m_buf;
	m_str[0] = 0;
	m_allocated = ALLOCATED;
}


TextLabel::~TextLabel()
{
	if ( m_gamui ) 
		m_gamui->Remove( this );
	if ( m_str != m_buf )
		delete [] m_str;
}


void TextLabel::Init( Gamui* gamui )
{
	m_gamui = gamui;
	m_gamui->Add( this );
}


void TextLabel::ClearText()
{
	m_str[0] = 0;
	m_width = m_height = -1;
}


const RenderAtom* TextLabel::GetRenderAtom() const
{
	GAMUIASSERT( m_gamui );
	GAMUIASSERT( m_gamui->GetTextAtom() );

	return Enabled() ? m_gamui->GetTextAtom() : m_gamui->GetDisabledTextAtom();
}


const char* TextLabel::GetText() const
{
	return m_str;
}


void TextLabel::SetText( const char* t )
{
	SetText( t, t+strlen(t) );
}


void TextLabel::SetText( const char* start, const char* end )
{
	m_width = m_height = -1;
	int len = end - start;
	int allocatedNeeded = len+1;

	if ( m_allocated < allocatedNeeded ) {
		m_allocated = (allocatedNeeded+ALLOCATED)*3/2;
		if ( m_str != m_buf )
			delete m_str;
		m_str = new char[m_allocated];
	}
	memcpy( m_str, start, len );
	m_str[len] = 0;
}


void TextLabel::Requires( int* indexNeeded, int* vertexNeeded ) 
{
	int len = strlen( m_str );
	*indexNeeded  = len*6;
	*vertexNeeded = len*4;
}


void TextLabel::CalcSize( float* width, float* height ) const
{
	*width = 0;
	*height = 0;

	if ( !m_gamui )
		return;
	IGamuiText* iText = m_gamui->GetTextInterface();
	if ( !iText )
		return;

	const char* p = m_str;

	IGamuiText::GlyphMetrics metrics;
	float x = 0;
	float x1 = 0;

	while ( p && *p ) {
		iText->GamuiGlyph( *p, &metrics );
		++p;
		x1 = x + metrics.width;
		x += metrics.advance;
		*height = metrics.height;
	}
	*width = x1;
}


void TextLabel::Queue( int *nIndex, int16_t* index, int *nVertex, Gamui::Vertex* vertex )
{
	if ( !m_gamui )
		return;
	IGamuiText* iText = m_gamui->GetTextInterface();

	const char* p = m_str;
	float x=X();
	float y=Y();

	IGamuiText::GlyphMetrics metrics;

	while ( p && *p ) {
		iText->GamuiGlyph( *p, &metrics );

		PushQuad( nIndex, index, *nVertex, 0, 1, 2, 0, 2, 3 );

		vertex[(*nVertex)++].Set( x, y, metrics.tx0, metrics.ty0 );
		vertex[(*nVertex)++].Set( x, (y+metrics.height), metrics.tx0, metrics.ty1 );
		vertex[(*nVertex)++].Set( x + metrics.width, y+metrics.height, metrics.tx1, metrics.ty1 );
		vertex[(*nVertex)++].Set( x + metrics.width, y, metrics.tx1, metrics.ty0 );

		++p;
		x += metrics.advance;
	}
}


float TextLabel::Width() const
{
	if ( !m_gamui )
		return 0;

	if ( m_width < 0 ) {
		CalcSize( &m_width, &m_height );
	}
	return m_width;
}


float TextLabel::Height() const
{
	if ( !m_gamui )
		return 0;

	if ( m_height < 0 ) {
		CalcSize( &m_width, &m_height );
	}
	return m_height;
}


TextBox::TextBox() : UIItem( Gamui::LEVEL_TEXT ), m_needsLayout( true ), m_width( 0 ), m_height( 0 ), m_textLabelArr( 0 ), m_lines( 0 )
{}


TextBox::~TextBox() 
{
	delete [] m_textLabelArr;
}


void TextBox::Init( Gamui* g ) 
{
	m_gamui = g;
	m_gamui->Add( this );
	m_storage.Init( g );
	m_storage.SetVisible( false );
	m_needsLayout = true;
}


const RenderAtom* TextBox::GetRenderAtom() const
{
	return 0;
}


void TextBox::Requires( int* indexNeeded, int* vertexNeeded )
{
	if ( m_needsLayout ) {
		float h = m_storage.Height();
		int lines =(int)(Height()/h);

		if ( lines != m_lines ) {
			m_lines = lines;
			delete [] m_textLabelArr;
			m_textLabelArr = new TextLabel[m_lines];
			for( int i=0; i<m_lines; ++i ) {
				m_textLabelArr[i].Init( m_gamui );
			}
		}
		for( int i=0; i<m_lines; ++i ) {
			m_textLabelArr[i].SetVisible( Visible() );
			m_textLabelArr[i].SetEnabled( Enabled() );
		}
		m_gamui->LayoutTextBlock( m_storage.GetText(), m_textLabelArr, m_lines, X(), Y(), Width() );

		m_needsLayout = false;
	}

	*indexNeeded = 0;
	*vertexNeeded = 0;
}


void TextBox::Queue( int *nIndex, int16_t* index, int *nVertex, Gamui::Vertex* vertex )
{
	// does nothing - children draw
}


Image::Image() : UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( 0 ),
	  m_height( 0 ),
	  m_slice( false )
{
}


Image::Image( Gamui* gamui, const RenderAtom& atom ): UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( 0 ),
	  m_height( 0 ),
	  m_slice( false )
{
	Init( gamui, atom );
}

Image::~Image()
{
}


void Image::Init( Gamui* gamui, const RenderAtom& atom )
{
	m_atom = atom;
	m_width = atom.srcWidth;
	m_height = atom.srcHeight;

	m_gamui = gamui;
	gamui->Add( this );
}


void Image::SetAtom( const RenderAtom& atom )
{
	m_atom = atom;
}


void Image::SetSlice( bool enable )
{
	m_slice = enable;
}


void Image::SetForeground( bool foreground )
{
	this->SetLevel( foreground ? Gamui::LEVEL_FOREGROUND : Gamui::LEVEL_BACKGROUND );
}


void Image::Requires( int* indexNeeded, int* vertexNeeded )
{
	if ( m_slice ) {
		*indexNeeded = 6*9;
		*vertexNeeded = 4*9;
	}
	else {
		*indexNeeded = 6;
		*vertexNeeded = 4;
	}
}


TiledImageBase::TiledImageBase() : UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( 0 ),
	  m_height( 0 )
{
}


TiledImageBase::TiledImageBase( Gamui* gamui ): UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( 0 ),
	  m_height( 0 )
{
	Init( gamui );
}


TiledImageBase::~TiledImageBase()
{
}


void TiledImageBase::Init( Gamui* gamui )
{
	m_width = 1;
	m_height = 1;

	m_gamui = gamui;
	gamui->Add( this );
}


void TiledImageBase::SetTile( int x, int y, const RenderAtom& atom )
{
	GAMUIASSERT( x<CX() );
	GAMUIASSERT( y<CY() );

	int index = 0;

	if ( atom.textureHandle == 0 ) {
		// Can always add a null atom.
		index = 0;
	}
	else if ( m_atom[1].textureHandle == 0 ) {
		// First thing added.
		index = 1;
		m_atom[1] = atom;
	}
	else {
		GAMUIASSERT( atom.renderState == m_atom[1].renderState );
		GAMUIASSERT( atom.textureHandle == m_atom[1].textureHandle );
		for ( index=1; index<MAX_ATOMS && m_atom[index].textureHandle; ++index ) {
			if (    m_atom[index].tx0 == atom.tx0
				 && m_atom[index].ty0 == atom.ty0
				 && m_atom[index].tx1 == atom.tx1
				 && m_atom[index].ty1 == atom.ty1 )
			{
				break;
			}
		}
		m_atom[index] = atom;
	}
	*(Mem()+y*CX()+x) = index;
}


void TiledImageBase::SetForeground( bool foreground )
{
	this->SetLevel( foreground ? Gamui::LEVEL_FOREGROUND : Gamui::LEVEL_BACKGROUND );
}


const RenderAtom* TiledImageBase::GetRenderAtom() const
{
	return &m_atom[1];
}


void TiledImageBase::Clear()														
{ 
	memset( Mem(), 0, CX()*CY() ); 
}

void TiledImageBase::Requires( int* indexNeeded, int* vertexNeeded )
{
	int cx = CX();
	int cy = CY();
	*indexNeeded = cx*cy*6;
	*vertexNeeded = cx*cy*4;
}


void TiledImageBase::Queue( int *nIndex, int16_t* index, int *nVertex, Gamui::Vertex* vertex )
{
	int startVertex = *nVertex;

	int cx = CX();
	int cy = CY();
	float x = X();
	float y = Y();
	float dx = Width() / (float)cx;
	float dy = Height() / (float)cy;
	int8_t* mem = Mem();
	int count = 0;

	for( int j=0; j<cy; ++j ) {
		for( int i=0; i<cx; ++i ) {
			if (*mem >= 0 && *mem < MAX_ATOMS && m_atom[*mem].textureHandle ) {
				index[(*nIndex)++] = *nVertex + 0;
				index[(*nIndex)++] = *nVertex + 1;
				index[(*nIndex)++] = *nVertex + 2;
				index[(*nIndex)++] = *nVertex + 0;
				index[(*nIndex)++] = *nVertex + 2;
				index[(*nIndex)++] = *nVertex + 3;

				vertex[(*nVertex)++].Set( x,	y,		m_atom[*mem].tx0, m_atom[*mem].ty1 );
				vertex[(*nVertex)++].Set( x,	y+dy,	m_atom[*mem].tx0, m_atom[*mem].ty0 );
				vertex[(*nVertex)++].Set( x+dx, y+dy,	m_atom[*mem].tx1, m_atom[*mem].ty0 );
				vertex[(*nVertex)++].Set( x+dx, y,		m_atom[*mem].tx1, m_atom[*mem].ty1 );

				++count;
			}
			x += dx;
			++mem;
		}
		x = X();
		y += dy;
	}
	ApplyRotation( count*4, &vertex[startVertex] );
}


const RenderAtom* Image::GetRenderAtom() const
{
	return &m_atom;
}


void Image::Queue( int *nIndex, int16_t* index, int *nVertex, Gamui::Vertex* vertex )
{
	if ( m_atom.textureHandle == 0 ) {
		return;
	}

	int startVertex = *nVertex;

	if (    !m_slice
		 || ( m_width <= m_atom.srcWidth && m_height <= m_atom.srcHeight ) )
	{
		PushQuad( nIndex, index, *nVertex, 0, 1, 2, 0, 2, 3 );

		float x0 = X();
		float y0 = Y();
		float x1 = X() + m_width;
		float y1 = Y() + m_height;

		vertex[(*nVertex)++].Set( x0, y0, m_atom.tx0, m_atom.ty1 );
		vertex[(*nVertex)++].Set( x0, y1, m_atom.tx0, m_atom.ty0 );
		vertex[(*nVertex)++].Set( x1, y1, m_atom.tx1, m_atom.ty0 );
		vertex[(*nVertex)++].Set( x1, y0, m_atom.tx1, m_atom.ty1 );
		ApplyRotation( 4, &vertex[startVertex] );
	}
	else {
		float x[4] = { X(), X()+(m_atom.srcWidth*0.5f), X()+(m_width-m_atom.srcWidth*0.5f), X()+m_width };
		if ( x[2] < x[1] ) {
			x[2] = x[1] = X() + (m_atom.srcWidth*0.5f);
		}
		float y[4] = { Y(), Y()+(m_atom.srcHeight*0.5f), Y()+(m_height-m_atom.srcHeight*0.5f), Y()+m_height };
		if ( y[2] < y[1] ) {
			y[2] = y[1] = Y() + (m_atom.srcHeight*0.5f);
		}

		float tx[4] = { m_atom.tx0, Mean( m_atom.tx0, m_atom.tx1 ), Mean( m_atom.tx0, m_atom.tx1 ), m_atom.tx1 };
		float ty[4] = { m_atom.ty1, Mean( m_atom.ty0, m_atom.ty1 ), Mean( m_atom.ty0, m_atom.ty1 ), m_atom.ty0 };

		for( int j=0; j<4; ++j ) {
			for( int i=0; i<4; ++i ) {
				vertex[(*nVertex)+j*4+i].Set( x[i], y[j], tx[i], ty[j] );
			}
		}
		for( int j=0; j<3; ++j ) {
			for( int i=0; i<3; ++i ) {
				PushQuad( nIndex, index, *nVertex, j*4+i, (j+1)*4+i, (j+1)*4+(i+1),
												   j*4+i, (j+1)*4+(i+1), j*4+(i+1) );
			}
		}
		*nVertex += 16;
		ApplyRotation( 16, &vertex[startVertex] );
	}
}




Button::Button() : UIItem( Gamui::LEVEL_FOREGROUND ),
	m_up( true ),
	m_usingText1( false )
{
}


void Button::Init(	Gamui* gamui,
					const RenderAtom& atomUpEnabled,
					const RenderAtom& atomUpDisabled,
					const RenderAtom& atomDownEnabled,
					const RenderAtom& atomDownDisabled,
					const RenderAtom& decoEnabled, 
					const RenderAtom& decoDisabled )
{
	m_gamui = gamui;

	m_atoms[UP] = atomUpEnabled;
	m_atoms[UP_D] = atomUpDisabled;
	m_atoms[DOWN] = atomDownEnabled;
	m_atoms[DOWN_D] = atomDownDisabled;
	m_atoms[DECO] = decoEnabled;
	m_atoms[DECO_D] = decoDisabled;

	m_textLayout = CENTER;
	m_decoLayout = CENTER;
	m_textDX = 0;
	m_textDY = 0;
	m_decoDX = 0;
	m_decoDY = 0;	

	m_face.Init( gamui, atomUpEnabled );
	m_face.SetForeground( true );
	m_face.SetSlice( true );

	m_deco.Init( gamui, decoEnabled );
	m_deco.SetLevel( Gamui::LEVEL_DECO );

	m_label[0].Init( gamui );
	m_label[1].Init( gamui );
	gamui->Add( this );
}


float Button::Width() const
{
	float x0 = m_face.X() + m_face.Width();
	float x1 = m_deco.X() + m_deco.Width();
	float x2 = m_label[0].X() + m_label[0].Width();
	float x3 = x2;
	if ( m_usingText1 ) 
		x3 = m_label[1].X() + m_label[1].Width();

	float x = Max( x0, Max( x1, Max( x2, x3 ) ) );
	return x - X();
}


float Button::Height() const
{
	float y0 = m_face.Y() + m_face.Height();
	float y1 = m_deco.Y() + m_deco.Height();
	float y2 = m_label[0].Y() + m_label[0].Height();
	float y3 = y2;
	if ( m_usingText1 ) 
		y3 = m_label[1].Y() + m_label[1].Height();

	float y = Max( y0, Max( y1, Max( y2, y3 ) ) );
	return y - Y();
}


void Button::PositionChildren()
{
	// --- deco ---
	if ( m_face.Width() > m_face.Height() ) {
		m_deco.SetSize( m_face.Height() , m_face.Height() );

		if ( m_decoLayout == LEFT ) {
			m_deco.SetPos( X() + m_decoDX, Y() + m_decoDY );
		}
		else if ( m_decoLayout == RIGHT ) {
			m_deco.SetPos( X() + m_face.Width() - m_face.Height() + m_decoDX, Y() + m_decoDY );
		}
		else {
			m_deco.SetPos( X() + (m_face.Width()-m_face.Height())*0.5f + m_decoDX, Y() + m_decoDY );
		}
	}
	else {
		m_deco.SetSize( m_face.Width() , m_face.Width() );
		m_deco.SetPos( X() + m_decoDX, Y() + (m_face.Height()-m_face.Width())*0.5f + m_decoDY );
	}

	// --- face ---- //
	float x=0, y0=0, y1=0;

	if ( !m_usingText1 ) {
		float h = m_label[0].Height();
		m_label[1].SetVisible( false );
		y0 = Y() + (m_face.Height() - h)*0.5f;
	}
	else {
		float h0 = m_label[0].Height();
		float h2 = m_label[1].Height();
		float h = h0 + h2;

		y0 = Y() + (m_face.Height() - h)*0.5f;
		y1 = y0 + h0;

		m_label[1].SetVisible( Visible() );
	}

	float w = m_label[0].Width();
	if ( m_usingText1 ) {
		w = Max( w, m_label[1].Width() );
	}
	
	if ( m_textLayout == LEFT ) {
		x = X();
	}
	else if ( m_textLayout == RIGHT ) {
		x = X() + m_face.Width() - w;
	}
	else {
		x = X() + (m_face.Width()-w)*0.5f;
	}

	x += m_textDX;
	y0 += m_textDY;
	y1 += m_textDY;

	m_label[0].SetPos( x, y0 );
	m_label[1].SetPos( x, y1 );

	m_label[0].SetVisible( Visible() );
	m_deco.SetVisible( Visible() );
	m_face.SetVisible( Visible() );
}


void Button::SetPos( float x, float y )
{
	UIItem::SetPos( x, y );
	m_face.SetPos( x, y );
	m_deco.SetPos( x, y );
	m_label[0].SetPos( x, y );
	m_label[1].SetPos( x, y );
}

void Button::SetSize( float width, float height )
{
	m_face.SetSize( width, height );
}


void Button::SetSizeByScale( float sx, float sy )
{
	m_face.SetSize( m_face.GetRenderAtom()->srcWidth*sx, m_face.GetRenderAtom()->srcHeight*sy );
}


void Button::SetText( const char* text )
{
	m_label[0].SetText( text );
}


void Button::SetText2( const char* text )
{
	m_usingText1 = true;
	m_label[1].SetText( text );
}


void Button::SetState()
{
	int faceIndex = UP;
	int decoIndex = DECO;
	if ( m_enabled ) {
		if ( m_up ) {
			// defaults set
		}
		else {
			faceIndex = DOWN;
		}
	}
	else {
		if ( m_up ) {
			faceIndex = UP_D;
			decoIndex = DECO_D;
		}
		else {
			faceIndex = DOWN_D;
			decoIndex = DECO_D;
		}
	}
	m_face.SetAtom( m_atoms[faceIndex] );
	m_deco.SetAtom( m_atoms[decoIndex] );
	m_label[0].SetEnabled( m_enabled );
	m_label[1].SetEnabled( m_enabled );
}


void Button::SetEnabled( bool enabled )
{
	m_enabled = enabled;
	SetState();
}


const RenderAtom* Button::GetRenderAtom() const
{
	return 0;
}


void Button::Requires( int* indexNeeded, int* vertexNeeded )
{
	PositionChildren();
	*indexNeeded = 0;
	*vertexNeeded = 0;
}


void Button::Queue( int *nIndex, int16_t* index, int *nVertex, Gamui::Vertex* vertex )
{
	// does nothing - children draw
}


bool PushButton::HandleTap( TapAction action, float x, float y )
{
	bool activated = false;
	if ( action == TAP_DOWN ) {
		m_up = false;
		activated = true;
	}
	else if ( action == TAP_UP || action == TAP_CANCEL ) {
		m_up = true;
		if ( action == TAP_UP && x >= X() && x < X()+Width() && y >= Y() && y < Y()+Height() ) {
			activated = true;
		}
	}
	SetState();
	return activated;
}


void ToggleButton::Clear()
{
	RemoveFromToggleGroup();
	Button::Clear();
}


void ToggleButton::RemoveFromToggleGroup()
{
	if ( m_next ) {
		ToggleButton* other = m_next;
		
		// unlink
		m_prev->m_next = m_next;
		m_next->m_prev = m_prev;

		// clean up the group just left.
		if ( other != this ) {
			other->ProcessToggleGroup();
		}
		m_next = m_prev = this;
	}
}


void ToggleButton::AddToToggleGroup( ToggleButton* button )
{
	GAMUIASSERT( button != this );
	GAMUIASSERT( this->m_next && button->m_next );
	if ( this->m_next && button->m_next ) {

		button->RemoveFromToggleGroup();
		
		button->m_prev = this;
		button->m_next = this->m_next;

		this->m_next->m_prev = button;
		this->m_next = button;

		ProcessToggleGroup();
	}
}


bool ToggleButton::InToggleGroup()
{
	return m_next != this;
}


void ToggleButton::ProcessToggleGroup()
{
	if ( m_next && m_next != this ) {
		// One and only one button can be down.
		ToggleButton* firstDown = 0;
		ToggleButton* candidate = 0;

		if ( this->Down() )
			firstDown = this;

		for( ToggleButton* it = this->m_next; it != this; it = it->m_next ) {
			if ( firstDown )
				it->PriSetUp();
			else if ( it->Down() && it->Visible() && it->Enabled() )
				firstDown = it;
			else
				it->PriSetUp();

			if ( !firstDown && it->Visible() && it->Enabled() )
				candidate = it;
		}

		if ( !firstDown && Visible() && Enabled() )
			this->PriSetDown();
		else if ( candidate )
			candidate->PriSetDown();
		else
			this->PriSetDown();
	}
}


bool ToggleButton::HandleTap( TapAction action, float x, float y )
{
	bool activated = false;
	if ( action == TAP_DOWN ) {
		m_wasUp = m_up;
		m_up = false;
		activated = true;
	}
	else if ( action == TAP_UP || action == TAP_CANCEL ) {
		m_up = m_wasUp;
		if ( action == TAP_UP && x >= X() && x < X()+Width() && y >= Y() && y < Y()+Height() ) {
			activated = true;
			m_up = !m_wasUp;
			ProcessToggleGroup();
		}
	}
	SetState();
	return activated;
}


DigitalBar::DigitalBar() : UIItem( Gamui::LEVEL_FOREGROUND ),
	m_nTicks( 0 ),
	m_spacing( 0 )
{
}


void DigitalBar::Init(	Gamui* gamui,
						int nTicks,
						const RenderAtom& atom0,
						const RenderAtom& atom1,
						const RenderAtom& atom2,
						float spacing )
{
	m_gamui = gamui;
	m_gamui->Add( this );

	GAMUIASSERT( nTicks <= MAX_TICKS );
	m_nTicks = nTicks;
	m_t0 = 0;
	m_t1 = 0;

	m_atom[0] = atom0;
	m_atom[1] = atom1;
	m_atom[2] = atom2;

	for( int i=0; i<nTicks; ++i ) {
		m_image[i].Init( gamui, atom0 );
		m_image[i].SetForeground( true );
	}
	m_spacing = spacing;
	SetRange( 0, 0 );
}



void DigitalBar::SetRange( float t0, float t1 )
{
	if ( t0 < 0 ) t0 = 0;
	if ( t0 > 1 ) t1 = 1;
	if ( t1 < 0 ) t1 = 0;
	if ( t1 > 1 ) t1 = 1;

	t1 = Max( t1, t0 );

	m_t0 = t0;
	m_t1 = t1;

	int index0 = (int)( t0 * (float)m_nTicks + 0.5f );
	int index1 = (int)( t1 * (float)m_nTicks + 0.5f );

	for( int i=0; i<index0; ++i ) {
		m_image[i].SetAtom( m_atom[0] );
	}
	for( int i=index0; i<index1; ++i ) {
		m_image[i].SetAtom( m_atom[1] );
	}
	for( int i=index1; i<m_nTicks; ++i ) {
		m_image[i].SetAtom( m_atom[2] );
	}
}


float DigitalBar::Width() const
{
	float w = 0;
	if ( m_gamui ) {
		for( int i=0; i<m_nTicks-1; ++i ) {
			w += m_image[i].GetRenderAtom()->srcWidth;
			w += m_spacing;
		}
		w += m_image[m_nTicks-1].GetRenderAtom()->srcWidth;
	}
	return w;
}


float DigitalBar::Height() const
{
	float h = 0;
	if ( m_gamui ) {
		h = Max( m_atom[0].srcHeight, Max( m_atom[1].srcHeight, m_atom[2].srcHeight ) );
	}
	return h;
}


void DigitalBar::SetVisible( bool visible )
{
	UIItem::SetVisible( visible );
	for( int i=0; i<m_nTicks; ++i ) {
		m_image[i].SetVisible( visible );
	}
}


const RenderAtom* DigitalBar::GetRenderAtom() const
{
	return 0;
}


void DigitalBar::Requires( int* indexNeeded, int* vertexNeeded )
{
	float x = X();
	float y = Y();

	for( int i=0; i<m_nTicks; ++i ) {
		m_image[i].SetPos( x, y );
		x += m_spacing + m_image[i].GetRenderAtom()->srcWidth;
	}
	*indexNeeded = 0;
	*vertexNeeded = 0;
}


void DigitalBar::Queue( int *nIndex, int16_t* index, int *nVertex, Gamui::Vertex* vertex )
{
	// does nothing - children draw
}


Gamui::Gamui()
	:	m_itemTapped( 0 ),
		m_iText( 0 ),
		m_itemArr( 0 ),
		m_nItems( 0 ),
		m_nItemsAllocated( 0 )
{
}


Gamui::Gamui(	IGamuiRenderer* renderer,
				const RenderAtom& textEnabled, 
				const RenderAtom& textDisabled,
				IGamuiText* iText ) 
	:	m_itemTapped( 0 ),
		m_iText( 0 ),
		m_itemArr( 0 ),
		m_nItems( 0 ),
		m_nItemsAllocated( 0 )
{
	Init( renderer, textEnabled, textDisabled, iText );
}


Gamui::~Gamui()
{
	for( int i=0; i<m_nItems; ++i ) {
		m_itemArr[i]->Clear();
	}
	free( m_itemArr );
}


void Gamui::Init(	IGamuiRenderer* renderer,
					const RenderAtom& textEnabled, 
					const RenderAtom& textDisabled,
					IGamuiText* iText )
{
	m_iRenderer = renderer;
	m_textAtomEnabled = textEnabled;
	m_textAtomDisabled = textDisabled;
	m_iText = iText;
}


void Gamui::Add( UIItem* item )
{
	if ( m_nItemsAllocated == m_nItems ) {
		m_nItemsAllocated = m_nItemsAllocated*3/2 + 16;
		m_itemArr = (UIItem**) realloc( m_itemArr, m_nItemsAllocated*sizeof(UIItem*) );
	}
	m_itemArr[m_nItems++] = item;
}


void Gamui::Remove( UIItem* item )
{
	// hmm...linear search. could be better.
	for( int i=0; i<m_nItems; ++i ) {
		if ( m_itemArr[i] == item ) {
			// swap off the back.
			m_itemArr[i] = m_itemArr[m_nItems-1];
			m_nItems--;

			item->Clear();
			break;
		}
	}
}


const UIItem* Gamui::Tap( float x, float y )
{
	TapDown( x, y );
	return TapUp( x, y );
}


void Gamui::TapDown( float x, float y )
{
	GAMUIASSERT( m_itemTapped == 0 );
	m_itemTapped = 0;

	for( int i=0; i<m_nItems; ++i ) {
		UIItem* item = m_itemArr[i];

		if (    item->Enabled() 
			 && item->Visible()
			 && x >= item->X() && x < item->X()+item->Width()
			 && y >= item->Y() && y < item->Y()+item->Height() )
		{
			if ( item->HandleTap( UIItem::TAP_DOWN, x, y ) ) {
				m_itemTapped = item;
				break;
			}
		}
	}
}


const UIItem* Gamui::TapUp( float x, float y )
{
	const UIItem* result = 0;
	if ( m_itemTapped ) {
		if ( m_itemTapped->HandleTap( UIItem::TAP_UP, x, y ) )
			result = m_itemTapped;
	}
	m_itemTapped = 0;
	return result;
}


void Gamui::TapCancel()
{
	if ( m_itemTapped ) {
		m_itemTapped->HandleTap( UIItem::TAP_CANCEL, 0, 0 );
	}
	m_itemTapped = 0;

}

int Gamui::SortItems( const void* _a, const void* _b )
{ 
	const UIItem* a = *((const UIItem**)_a);
	const UIItem* b = *((const UIItem**)_b);
	// Priorities:
	// 1. Level
	// 2. RenderState
	// 3. Texture

	// Level wins.
	if ( a->Level() < b->Level() )
		return -1;
	else if ( a->Level() > b->Level() )
		return 1;

	const RenderAtom* atomA = a->GetRenderAtom();
	const RenderAtom* atomB = b->GetRenderAtom();

	const void* rStateA = (atomA) ? atomA->renderState : 0;
	const void* rStateB = (atomB) ? atomB->renderState : 0;

	// If level is the same, look at the RenderAtom;
	// to get to the state:
	if ( rStateA < rStateB )
		return -1;
	else if ( rStateA > rStateB )
		return 1;

	const void* texA = (atomA) ? atomA->textureHandle : 0;
	const void* texB = (atomB) ? atomB->textureHandle : 0;

	// finally the texture.
	if ( texA < texB )
		return -1;
	else if ( texA > texB )
		return 1;

	// just to guarantee a consistent order.
	return a - b;
}


void Gamui::Render()
{
	// Do a check for sort - hard to trust 'qsort' and don't want 
	// to call it on unsorted data.
	bool sorted = true;
	for( int i=0; i<m_nItems-1; ++i ) {
		if ( SortItems( m_itemArr+i, m_itemArr+i+1 ) > 0 ) {
			sorted = false;
			break;
		}
	}
	if ( !sorted ) {
		qsort( m_itemArr, m_nItems, sizeof(UIItem*), SortItems );
	}

	int nIndex = 0;
	int nVertex = 0;

	m_iRenderer->BeginRender();

	const void* renderState = 0;
	const void* textureHandle = 0;

	for( int i=0; i<m_nItems; ++i ) {
		UIItem* item = m_itemArr[i];
		const RenderAtom* atom = item->GetRenderAtom();

		// Requires() does layout / sets child visibility. Can't skip this step:
		int indexNeeded=0, vertexNeeded=0;
		item->Requires( &indexNeeded, &vertexNeeded );

		if ( !item->Visible() || !atom || !atom->textureHandle )
			continue;

		// flush:
		if (    nIndex
			 && ( atom->renderState != renderState || atom->textureHandle != textureHandle ) )
		{
			m_iRenderer->Render( renderState, textureHandle, nIndex, &m_indexBuffer[0], nVertex, &m_vertexBuffer[0] );
			nIndex = nVertex = 0;
		}

		if ( atom->renderState != renderState ) {
			m_iRenderer->BeginRenderState( atom->renderState );
			renderState = atom->renderState;
		}
		if ( atom->textureHandle != textureHandle ) {
			m_iRenderer->BeginTexture( atom->textureHandle );
			textureHandle = atom->textureHandle;
		}

		if ( nIndex + indexNeeded >= INDEX_SIZE || nVertex + vertexNeeded >= VERTEX_SIZE ) {
			m_iRenderer->Render( renderState, textureHandle, nIndex, &m_indexBuffer[0], nVertex, &m_vertexBuffer[0] );
			nIndex = nVertex = 0;
		}
		if ( nIndex + indexNeeded <= INDEX_SIZE && nVertex + vertexNeeded <= VERTEX_SIZE ) {
			item->Queue( &nIndex, m_indexBuffer, &nVertex, m_vertexBuffer );
		}
		else {
			GAMUIASSERT( 0 );	// buffers aren't big enough - stuff won't render.
		}
	}
	// flush:
	if ( nIndex ) {
		m_iRenderer->Render( renderState, textureHandle, nIndex, &m_indexBuffer[0], nVertex, &m_vertexBuffer[0] );
		nIndex = nVertex = 0;
	}

	m_iRenderer->EndRender();
}


void Gamui::Layout( UIItem** item, int nItems,
					int cx, int cy,
					float originX, float originY,
					float tableWidth, float tableHeight )
{
	float itemWidth = 0, itemHeight = 0;
	if ( nItems == 0 )
		return;

	for( int i=0; i<cx && i<nItems; ++i )
		itemWidth += item[i]->Width();
	for( int i=0; i<cy && (i*cx)<nItems; ++i )
		itemHeight += item[i*cx]->Height();

	float xSpacing = 0;
	if ( cx > 1 ) { 
		xSpacing = ( tableWidth - itemWidth ) / (float)(cx-1);
	}
	float ySpacing = 0;
	if ( cy > 1 ) {
		ySpacing = ( tableHeight - itemHeight ) / (float)(cy-1);
	}

	int c = 0;
	int r = 0;
	float x = originX;
	float y = originY;

	for( int i=0; i<nItems; ++i ) {
		item[i]->SetPos( x, y );
		x += item[i]->Width();
		x += xSpacing;
		
		++c;
		if ( c == cx ) {
			++r;
			x = originX;
			y += item[i]->Height();
			y += ySpacing;
			c = 0;
		}
		if ( r == cy )
			break;
	}
}


const char* SkipSpace( const char* p ) {
	while( p && *p && *p == ' ' ) {
		++p;
	}
	return p;
}

const char* EndOfWord( const char* p ) {
	while( p && *p && *p != ' ' && *p != '\n' ) {
		++p;
	}
	return p;
}


void Gamui::LayoutTextBlock(	const char* text,
								TextLabel* textLabels, int nText,
								float originX, float originY,
								float width )
{
	GAMUIASSERT( text );
	const char* p = text;
	int i = 0;

	TextLabel label;
	label.Init( this );
	label.SetText( "X" );
	float lineHeight = label.Height();

	while ( i < nText && *p ) {
		label.ClearText();
		
		// Add first word: always get at least one.		
		p = SkipSpace( p );
		const char* end = EndOfWord( p );
		end = SkipSpace( end );

		// (p,end) definitely gets added. The question is how much more?
		const char* q = end;
		while ( *q && *q != '\n' ) {
			q = EndOfWord( q );
			q = SkipSpace( q );
			label.SetText( p, q );
			if ( label.Width() > width ) {
				break;
			}
			else {
				end = q;
			}
		}
		
		textLabels[i].SetText( p, end );
		textLabels[i].SetPos( originX, originY + (float)i*lineHeight );
		p = end;
		++i;
		// We just put in a carriage return, so the first is free:
		if ( *p == '\n' )
			++p;

		// The rest advance i:
		while ( *p == '\n' && i < nText ) {
			textLabels[i].ClearText();
			++i;
			++p;
		}
	}
	while( i < nText ) {
		textLabels[i].ClearText();
		++i;
	}
}
