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
	  m_gamui( 0 ),
	  m_level( p_level )
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



TextLabel::TextLabel() : GamItem( Gamui::TEXT_LEVEL )
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


const RenderAtom* TextLabel::GetCurrentAtom() const
{
	GAMUIASSERT( m_gamui );
	GAMUIASSERT( m_gamui->GetTextAtom() );
	return m_gamui->GetTextAtom();
}


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

		index[(*nIndex)++] = *nVertex + 0;
		index[(*nIndex)++] = *nVertex + 1;
		index[(*nIndex)++] = *nVertex + 2;
		index[(*nIndex)++] = *nVertex + 0;
		index[(*nIndex)++] = *nVertex + 2;
		index[(*nIndex)++] = *nVertex + 3;

		vertex[(*nVertex)++].Set( x, y, metrics.tx0, metrics.ty0 );
		vertex[(*nVertex)++].Set( x, (y+height), metrics.tx0, metrics.ty1 );
		vertex[(*nVertex)++].Set( x + (float)metrics.width, y+height, metrics.tx1, metrics.ty1 );
		vertex[(*nVertex)++].Set( x + (float)metrics.width, y, metrics.tx1, metrics.ty0 );

		++p;
		x += (float)metrics.advance;
	}
}


Gamui::Gamui()
	:	m_textAtom( 0 ),
		m_iText( 0 ),
		m_textHeight( 0 )
{
}


Gamui::~Gamui()
{
	for( vector< GamItem* >::iterator it = m_itemArr.begin(); it != m_itemArr.end(); it++ ) {
		(*it)->Attach( 0 );
	}
}


void Gamui::InitText( const RenderAtom* atom, int pixelHeight, IGamuiText* iText )
{
	m_textAtom = atom;
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

	// If level is the same, look at the RenderAtom;
	const RenderAtom* rA = a->GetCurrentAtom();
	const RenderAtom* rB = b->GetCurrentAtom();

	// to get to the state:
	if ( rA->renderState < rB->renderState )
		return true;
	else if ( rA->renderState > rB->renderState )
		return false;

	// finally the texture.
	return rA->textureHandle < rB->textureHandle;
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
		const RenderAtom* atom = item->GetCurrentAtom();

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

		int indexNeeded=0, vertexNeeded=0;
		item->Requires( &indexNeeded, &vertexNeeded );
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


