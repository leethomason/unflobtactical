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

using namespace gamui;
using namespace std;


GamItem::GamItem( int p_level ) 
	: m_x( 0 ),
	  m_y( 0 ),
	  m_level( p_level ),
	  m_visible( true ),
	  m_gamui( 0 ),
	  m_enabled( true )
{}


void GamItem::Attach( Gamui* g ) 
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


void GamItem::PushQuad( int *nIndex, int16_t* index, int base, int a, int b, int c, int d, int e, int f )
{
	index[(*nIndex)++] = base+a;
	index[(*nIndex)++] = base+b;
	index[(*nIndex)++] = base+c;
	index[(*nIndex)++] = base+d;
	index[(*nIndex)++] = base+e;
	index[(*nIndex)++] = base+f;

}


TextLabel::TextLabel() : GamItem( Gamui::LEVEL_TEXT )
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
}


const RenderAtom* TextLabel::GetRenderAtom() const
{
	GAMUIASSERT( m_gamui );
	GAMUIASSERT( m_gamui->GetTextAtom() );

	return Enabled() ? m_gamui->GetTextAtom() : m_gamui->GetDisabledTextAtom();
}


/*void TextLabel::AddItems( std::vector< RenderItem >* renderItems )
{
	GAMUIASSERT( m_gamui );
	GAMUIASSERT( m_gamui->GetTextAtom() );
	RenderItem rItem = { m_gamui->GetTextAtom(), this };
	renderItems->push_back( rItem );
}
*/


void TextLabel::SetText( const char* t )
{
	// already in string mode? use that.
	if ( buf[ALLOCATE-1] ) {
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
	if ( buf[ALLOCATE-1] ) {
		len = (int)str->size();
	}
	else {
		len = strlen( buf );
	}
	*indexNeeded  = len*6;
	*vertexNeeded = len*4;
}


void TextLabel::CalcSize( int* width, int* height )
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
	int x = 0;

	while ( p && *p ) {
		iText->GamuiGlyph( *p, &metrics );
		++p;
		x += metrics.advance;
	}
	*width = x;
	*height = m_gamui->GetTextHeight();
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
	float height = (float)m_gamui->GetTextHeight();

	IGamuiText::GlyphMetrics metrics;

	while ( p && *p ) {
		iText->GamuiGlyph( *p, &metrics );

		PushQuad( nIndex, index, *nVertex, 0, 1, 2, 0, 2, 3 );

		vertex[(*nVertex)++].Set( x, y, metrics.tx0, metrics.ty0 );
		vertex[(*nVertex)++].Set( x, (y+height), metrics.tx0, metrics.ty1 );
		vertex[(*nVertex)++].Set( x + (float)metrics.width, y+height, metrics.tx1, metrics.ty1 );
		vertex[(*nVertex)++].Set( x + (float)metrics.width, y, metrics.tx1, metrics.ty0 );

		++p;
		x += (float)metrics.advance;
	}
}


Image::Image() : GamItem( Gamui::LEVEL_BACKGROUND ),
	  m_srcWidth( 0 ),
	  m_srcHeight( 0 ),
	  m_width( 0 ),
	  m_height( 0 ),
	  m_slice( false )
{
}


Image::~Image()
{
}


void Image::Init( const RenderAtom& atom, int srcWidth, int srcHeight )
{
	SetAtom( atom, srcWidth, srcHeight );
	m_width = srcWidth;
	m_height = srcHeight;	
}


void Image::SetAtom( const RenderAtom& atom, int srcWidth, int srcHeight )
{
	GAMUIASSERT( srcWidth > 0 );
	GAMUIASSERT( srcHeight > 0 );

	m_atom = atom;
	m_srcWidth = srcWidth;
	m_srcHeight = srcHeight;
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


/*void Image::AddItems( std::vector< RenderItem >* renderItems )
{
	if ( m_atom.textureHandle ) {
		RenderItem rItem = { &m_atom, this };
		renderItems->push_back( rItem );
	}
}
*/

void Image::Queue( int *nIndex, int16_t* index, int *nVertex, Gamui::Vertex* vertex )
{
	if ( m_atom.textureHandle == 0 ) {
		return;
	}

	if (    !m_slice
		 || ( m_width <= m_srcWidth && m_height <= m_srcHeight ) )
	{
		PushQuad( nIndex, index, *nVertex, 0, 1, 2, 0, 2, 3 );

		float x0 = X();
		float y0 = Y();
		float x1 = X() + (float)m_width;
		float y1 = Y() + (float)m_height;

		vertex[(*nVertex)++].Set( x0, y0, m_atom.tx0, m_atom.ty1 );
		vertex[(*nVertex)++].Set( x0, y1, m_atom.tx0, m_atom.ty0 );
		vertex[(*nVertex)++].Set( x1, y1, m_atom.tx1, m_atom.ty0 );
		vertex[(*nVertex)++].Set( x1, y0, m_atom.tx1, m_atom.ty1 );
	}
	else {
		float x[4] = { X(), X()+(float)(m_srcWidth/2), X()+(float)(m_width-m_srcWidth/2), X()+(float)m_width };
		if ( x[2] < x[1] ) {
			x[2] = x[1] = X() + (float)(m_srcWidth/2);
		}
		float y[4] = { Y(), Y()+(float)(m_srcHeight/2), Y()+(float)(m_height-m_srcHeight/2), Y()+(float)m_height };
		if ( y[2] < y[1] ) {
			y[2] = y[1] = Y() + (float)(m_srcHeight/2);
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
	}
}




Button::Button() : GamItem( Gamui::LEVEL_FOREGROUND )
{
}


void Button::Init(	const RenderAtom& atomUpEnabled,
					const RenderAtom& atomUpDisabled,
					const RenderAtom& atomDownEnabled,
					const RenderAtom& atomDownDisabled,
					const RenderAtom& decoEnabled, 
					const RenderAtom& decoDisabled,
					int srcWidth,
					int srcHeight )
{
	m_atoms[UP] = atomUpEnabled;
	m_atoms[UP_D] = atomUpDisabled;
	m_atoms[DOWN] = atomDownEnabled;
	m_atoms[DOWN_D] = atomDownDisabled;
	m_atoms[DECO] = decoEnabled;
	m_atoms[DECO_D] = decoDisabled;
	
	m_face.Init( atomUpEnabled, srcWidth, srcHeight );
	m_face.SetForeground( true );
	m_face.SetSlice( true );

	m_deco.Init( decoEnabled, srcWidth, srcHeight );
	m_deco.SetLevel( Gamui::LEVEL_DECO );
}


void Button::Attach( Gamui* gamui )
{
	if ( gamui ) {
		gamui->Add( &m_face );
		gamui->Add( &m_deco );
		gamui->Add( &m_label );
	}
	GamItem::Attach( gamui );
}


void Button::PositionChildren()
{
	if ( m_face.Width() > m_face.Height() ) {
		m_deco.SetSize( m_face.Height() , m_face.Height() );
		m_deco.SetPos( X() + (m_face.Width()-m_face.Height())/2, Y() );
	}
	else {
		m_deco.SetSize( m_face.Width() , m_face.Width() );
		m_deco.SetPos( X(), Y() + (m_face.Height()-m_face.Width())/2 );
	}

	int h, w;
	m_label.CalcSize( &w, &h );
	m_label.SetPos( X() + (float)((m_face.Width()-w)/2),
					Y() + (float)((m_face.Height()-h)/2) );

	m_label.SetVisible( Visible() );
	m_deco.SetVisible( Visible() );
	m_face.SetVisible( Visible() );
}


void Button::SetPos( float x, float y )
{
	GamItem::SetPos( x, y );
	m_face.SetPos( x, y );
	m_deco.SetPos( x, y );
	m_label.SetPos( x, y );
}

void Button::SetSize( int width, int height )
{
	m_face.SetSize( width, height );
}


void Button::SetText( const char* text )
{
	m_label.SetText( text );
}


void Button::SetEnabled( bool enabled )
{
	if ( enabled ) {
		m_face.SetAtom( m_atoms[UP], m_face.SrcWidth(), m_face.SrcHeight() );
		m_deco.SetAtom( m_atoms[DECO], m_deco.SrcWidth(), m_deco.SrcHeight() );
	}
	else {
		m_face.SetAtom( m_atoms[UP_D], m_face.SrcWidth(), m_face.SrcHeight() );
		m_deco.SetAtom( m_atoms[DECO_D], m_deco.SrcWidth(), m_deco.SrcHeight() );
	}
	m_label.SetEnabled( enabled );
}


const RenderAtom* Button::GetRenderAtom() const
{
	return 0;
}

	/*void Button::AddItems( std::vector< RenderItem >* renderItems )
{

}
*/


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



PushButton::PushButton() : Button()
{
}


Gamui::Gamui()
	:	m_iText( 0 ),
		m_textHeight( 0 )
{
}


Gamui::~Gamui()
{
	for( vector< GamItem* >::iterator it = m_itemArr.begin(); it != m_itemArr.end(); it++ ) {
		(*it)->Attach( 0 );
	}
}


void Gamui::InitText(	const RenderAtom& enabled, 
						const RenderAtom& disabled,
						int pixelHeight, 
						IGamuiText* iText )
{
	m_textAtomEnabled = enabled;
	m_textAtomDisabled = disabled;
	m_textHeight = pixelHeight;
	m_iText = iText;
}


void Gamui::Add( GamItem* item )
{
	item->Attach( this );
	m_itemArr.push_back( item );
}


void Gamui::Remove( GamItem* item )
{
	// hmm...linear search. could be better.
	for( vector< GamItem* >::iterator it = m_itemArr.begin(); it != m_itemArr.end(); it++ ) {
		if ( *it == item ) {
			m_itemArr.erase( it );
			item->Attach( 0 );
			break;
		}
	}
}


bool Gamui::SortItems( const GamItem* a, const GamItem* b )
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


void Gamui::Render( IGamuiRenderer* renderer )
{
	sort( m_itemArr.begin(), m_itemArr.end(), SortItems );
	int nIndex = 0;
	int nVertex = 0;

	renderer->BeginRender();

	const void* renderState = 0;
	const void* textureHandle = 0;

	for( vector< GamItem* >::iterator it = m_itemArr.begin(); it != m_itemArr.end(); it++ ) {
		GamItem* item = *it;
		const RenderAtom* atom = item->GetRenderAtom();

		// Requires() does layout / sets child visibility. Can't skip this step:
		int indexNeeded=0, vertexNeeded=0;
		item->Requires( &indexNeeded, &vertexNeeded );

		if ( !item->Visible() || !atom )
			continue;

		// flush:
		if (    nIndex
			 && ( atom->renderState != renderState || atom->textureHandle != textureHandle ) )
		{
			renderer->Render( renderState, textureHandle, nIndex, &m_indexBuffer[0], nVertex, &m_vertexBuffer[0] );
			nIndex = nVertex = 0;
		}

		if ( atom->renderState != renderState ) {
			renderer->BeginRenderState( atom->renderState );
			renderState = atom->renderState;
		}
		if ( atom->textureHandle != textureHandle ) {
			renderer->BeginTexture( atom->textureHandle );
			textureHandle = atom->textureHandle;
		}

		if ( nIndex + indexNeeded >= INDEX_SIZE || nVertex + vertexNeeded >= VERTEX_SIZE ) {
			renderer->Render( renderState, textureHandle, nIndex, &m_indexBuffer[0], nVertex, &m_vertexBuffer[0] );
			nIndex = nVertex = 0;
		}
		if ( nIndex + indexNeeded <= INDEX_SIZE && nVertex + vertexNeeded <= VERTEX_SIZE ) {
			item->Queue( &nIndex, m_indexBuffer, &nVertex, m_vertexBuffer );
		}
	}
	// flush:
	if ( nIndex ) {
		renderer->Render( renderState, textureHandle, nIndex, &m_indexBuffer[0], nVertex, &m_vertexBuffer[0] );
		nIndex = nVertex = 0;
	}

	renderer->EndRender();
}
