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
#include <algorithm>
#include <math.h>

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


void UIItem::Attach( Gamui* g ) 
{
	if ( g ) {
		GAMUIASSERT( !m_gamui );
		m_gamui = g;
	}
	else {
		GAMUIASSERT( m_gamui );
		m_gamui = 0;
	}
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
	buf[0] = 0;
	buf[ALLOCATE-1] = 0;
}


TextLabel::~TextLabel()
{
	if ( m_gamui ) 
		m_gamui->Remove( this );
	ClearText();
}


void TextLabel::ClearText()
{
	if ( buf[ALLOCATE-1] != 0 ) {
		delete str;
	}
	buf[0] = 0;
	buf[ALLOCATE-1] = 0;
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
	if ( IsStr() )
		return str->c_str();
	else
		return buf;
}

void TextLabel::SetText( const char* t )
{
	m_width = m_height = -1;
	// already in string mode? use that.
	if ( IsStr() ) {
		*str = t;
	}
	else {
		ClearText();

		if ( t ) {
			int len = strlen( t );
			if ( len < ALLOCATE-1 ) {
				memcpy( buf, t, len );
				buf[len] = 0;
			}
			else {
				str = new string( t );
				buf[ALLOCATE-1] = 127;
			}
		}
	}
}


void TextLabel::Requires( int* indexNeeded, int* vertexNeeded ) 
{
	int len = 0;
	if ( IsStr() ) {
		len = (int)str->size();
	}
	else {
		len = strlen( buf );
	}
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

	const char* p = 0;
	if ( buf[ALLOCATE-1] ) { 
		p = str->c_str();
	}
	else {
		p = buf;
	}

	IGamuiText::GlyphMetrics metrics;
	float x = 0;

	while ( p && *p ) {
		iText->GamuiGlyph( *p, &metrics );
		++p;
		x += metrics.advance;
		*height = metrics.height;
	}
	*width = x;
}


void TextLabel::Queue( int *nIndex, int16_t* index, int *nVertex, Gamui::Vertex* vertex )
{
	if ( !m_gamui )
		return;
	IGamuiText* iText = m_gamui->GetTextInterface();

	const char* p = 0;
	if ( buf[ALLOCATE-1] ) { 
		p = str->c_str();
	}
	else {
		p = buf;
	}

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


Image::Image() : UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( 0 ),
	  m_height( 0 ),
	  m_slice( false )
{
}


Image::Image( const RenderAtom& atom ): UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( 0 ),
	  m_height( 0 ),
	  m_slice( false )
{
	Init( atom );
}

Image::~Image()
{
}


void Image::Init( const RenderAtom& atom )
{
	m_atom = atom;
	m_width = atom.srcWidth;
	m_height = atom.srcHeight;
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
	if ( m_atom.textureHandle == 0 ) {
		*indexNeeded = 0;
		*vertexNeeded = 0;
	}
	else if ( m_slice ) {
		*indexNeeded = 6*9;
		*vertexNeeded = 4*9;
	}
	else {
		*indexNeeded = 6;
		*vertexNeeded = 4;
	}
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
	m_up( true )
{
}


void Button::Init(	const RenderAtom& atomUpEnabled,
					const RenderAtom& atomUpDisabled,
					const RenderAtom& atomDownEnabled,
					const RenderAtom& atomDownDisabled,
					const RenderAtom& decoEnabled, 
					const RenderAtom& decoDisabled )
{
	m_atoms[UP] = atomUpEnabled;
	m_atoms[UP_D] = atomUpDisabled;
	m_atoms[DOWN] = atomDownEnabled;
	m_atoms[DOWN_D] = atomDownDisabled;
	m_atoms[DECO] = decoEnabled;
	m_atoms[DECO_D] = decoDisabled;
	
	m_face.Init( atomUpEnabled );
	m_face.SetForeground( true );
	m_face.SetSlice( true );

	m_deco.Init( decoEnabled );
	m_deco.SetLevel( Gamui::LEVEL_DECO );
}


void Button::Attach( Gamui* gamui )
{
	if ( gamui ) {
		gamui->Add( &m_face );
		gamui->Add( &m_deco );
		gamui->Add( &m_label );
	}
	UIItem::Attach( gamui );
}


void Button::PositionChildren()
{
	if ( m_face.Width() > m_face.Height() ) {
		m_deco.SetSize( m_face.Height() , m_face.Height() );
		m_deco.SetPos( X() + (m_face.Width()-m_face.Height())*0.5f, Y() );
	}
	else {
		m_deco.SetSize( m_face.Width() , m_face.Width() );
		m_deco.SetPos( X(), Y() + (m_face.Height()-m_face.Width())*0.5f );
	}

	float h = m_label.Height();
	float w = m_label.Width();
	m_label.SetPos( X() + ((m_face.Width()-w)*0.5f),
					Y() + ((m_face.Height()-h)*0.5f) );

	m_label.SetVisible( Visible() );
	m_deco.SetVisible( Visible() );
	m_face.SetVisible( Visible() );
}


void Button::SetPos( float x, float y )
{
	UIItem::SetPos( x, y );
	m_face.SetPos( x, y );
	m_deco.SetPos( x, y );
	m_label.SetPos( x, y );
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
	m_label.SetText( text );
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
	m_label.SetEnabled( m_enabled );
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


bool PushButton::HandleTap( int action, float x, float y )
{
	bool handled = false;
	if ( action == TAP_DOWN ) {
		m_up = false;
		handled = true;
		SetState();
	}
	else if ( action == TAP_UP ) {
		m_up = true;
		SetState();
	}
	return handled;
}


bool ToggleButton::HandleTap( int action, float x, float y )
{
	bool handled = false;
	if ( action == TAP_DOWN ) {
		m_up = !m_up;
		handled = true;
		SetState();
	}
	return handled;
}


DigitalBar::DigitalBar() : UIItem( Gamui::LEVEL_FOREGROUND ),
	m_nTicks( 0 ),
	m_spacing( 0 )
{
}


void DigitalBar::Init(	int nTicks,
						const RenderAtom& atom0,
						const RenderAtom& atom1,
						const RenderAtom& atom2,
						float spacing )
{
	GAMUIASSERT( nTicks <= MAX_TICKS );
	m_nTicks = nTicks;
	m_t0 = 0;
	m_t1 = 0;

	m_atom[0] = atom0;
	m_atom[1] = atom1;
	m_atom[2] = atom2;

	for( int i=0; i<nTicks; ++i ) {
		m_image[i].Init( atom0 );
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

	int index0 = (int)( t0 * (m_nTicks-1.0f) + 0.5f );
	int index1 = (int)( t1 * (m_nTicks-1.0f) + 0.5f );

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


void DigitalBar::Attach( Gamui* gamui )
{
	if ( gamui ) {
		for( int i=0; i<m_nTicks; ++i ) {
			gamui->Add( &m_image[i] );
		}
	}
	UIItem::Attach( gamui );
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
		m_iText( 0 )
{
}


Gamui::~Gamui()
{
	for( vector< UIItem* >::iterator it = m_itemArr.begin(); it != m_itemArr.end(); it++ ) {
		(*it)->Attach( 0 );
	}
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
	item->Attach( this );

	if ( item->IsToggle() && item->IsToggle()->ToggleGroup() > 0 ) {
		ToggleButton* toggle = item->IsToggle();

		bool somethingDown = false;

		for( unsigned i=0; i<m_itemArr.size(); ++i ) {
			if (    m_itemArr[i]->IsToggle() 
				 && m_itemArr[i]->IsToggle()->ToggleGroup() == toggle->ToggleGroup() 
				 && m_itemArr[i]->IsToggle()->Down() ) 
			{
				somethingDown = true;
				break;
			}
		}
		if ( !somethingDown ) {
			toggle->SetDown();
			toggle = toggle;
		}
	}

	m_itemArr.push_back( item );
}


void Gamui::Remove( UIItem* item )
{
	// hmm...linear search. could be better.
	for( vector< UIItem* >::iterator it = m_itemArr.begin(); it != m_itemArr.end(); it++ ) {
		if ( *it == item ) {
			m_itemArr.erase( it );
			item->Attach( 0 );
			break;
		}
	}
}


const UIItem* Gamui::TapDown( float x, float y )
{
	GAMUIASSERT( m_itemTapped == 0 );
	m_itemTapped = 0;

	for( vector< UIItem* >::iterator it = m_itemArr.begin(); it != m_itemArr.end(); it++ ) {
		UIItem* item = *it;

		if (    item->Enabled() 
			 && item->Visible()
			 && x >= item->X() && x < item->X()+item->Width()
			 && y >= item->Y() && y < item->Y()+item->Height() )
		{
			// Toggles. Grr. Only and only one toggle can be down.
			if ( item->IsToggle() && item->IsToggle()->ToggleGroup() > 0 ) {
				ToggleButton* toggle = item->IsToggle();
				if ( toggle->Down() ) {
					// Do nothing. Can't go up. It is the down button.
				}
				else {
					toggle->HandleTap( UIItem::TAP_DOWN, x, y );
					GAMUIASSERT( toggle->Down() );
					m_itemTapped = toggle;

					for ( unsigned i=0; i<m_itemArr.size(); ++i ) {
						if (    m_itemArr[i] != toggle
							 && m_itemArr[i]->IsToggle()
							 && m_itemArr[i]->IsToggle()->ToggleGroup() == toggle->ToggleGroup() )
						{
							m_itemArr[i]->IsToggle()->SetUp();
							// should be able to break, but make it all consistent.
						}
					}
				}
			}
			else {
				if ( item->HandleTap( UIItem::TAP_DOWN, x, y ) ) {
					m_itemTapped = item;
				}
			}
		}
	}
	return m_itemTapped;
}


const UIItem* Gamui::TapUp( float x, float y )
{
	if ( m_itemTapped ) {
		m_itemTapped->HandleTap( UIItem::TAP_UP, x, y );
	}
	m_itemTapped = 0;
	return 0;
}


bool Gamui::SortItems( const UIItem* a, const UIItem* b )
{ 
	// Priorities:
	// 1. Level
	// 2. RenderState
	// 3. Texture

	// Level wins.
	if ( a->Level() < b->Level() )
		return true;
	else if ( a->Level() > b->Level() )
		return false;

	const RenderAtom* atomA = a->GetRenderAtom();
	const RenderAtom* atomB = b->GetRenderAtom();

	const void* rStateA = (atomA) ? atomA->renderState : 0;
	const void* rStateB = (atomB) ? atomB->renderState : 0;

	// If level is the same, look at the RenderAtom;
	// to get to the state:
	if ( rStateA < rStateB )
		return true;
	else if ( rStateA > rStateB )
		return false;

	const void* texA = (atomA) ? atomA->textureHandle : 0;
	const void* texB = (atomB) ? atomB->textureHandle : 0;

	// finally the texture.
	return texA < texB;
}


void Gamui::Render()
{
	sort( m_itemArr.begin(), m_itemArr.end(), SortItems );
	int nIndex = 0;
	int nVertex = 0;

	m_iRenderer->BeginRender();

	const void* renderState = 0;
	const void* textureHandle = 0;

	for( vector< UIItem* >::iterator it = m_itemArr.begin(); it != m_itemArr.end(); it++ ) {
		UIItem* item = *it;
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
					float tableWidth, float tableHeight,
					int flags )
{
	float itemWidth = 0, itemHeight = 0;
	for( int i=0; i<nItems; ++i ) {
		itemWidth += item[i]->Width();
		itemHeight += item[i]->Height();
	}
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
		}
		if ( r == cy )
			break;
	}
}
