#include "gamui.h"
#include <algorithm>

using namespace gamui;
using namespace std;


GamItem::GamItem() 
	: m_x( 0 ),
	  m_y( 0 ),
	  m_gamui( 0 ),
	  m_level( Gamui::ITEM_LEVEL )
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



TextLabel::TextLabel() : GamItem()
{
	buf[0] = 0;
	buf[ALLOCATE-1] = 0;

	// default text to the text level.
	m_level = Gamui::TEXT_LEVEL;
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


void TextLabel::Queue( std::vector< int16_t >* index, std::vector< Gamui::Vertex >* vertex )
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

	Gamui::Vertex v;
	IGamuiText::GlyphMetrics metrics;

	while ( p && *p ) {
		int16_t offset = (int16_t)vertex->size();
		iText->GamuiGlyph( *p, &metrics );

		v.x = x;
		v.y = y;
		v.tx = metrics.tx0;
		v.ty = metrics.ty0;
		vertex->push_back( v );

		v.x = x;
		v.y = (y+height);
		v.tx = metrics.tx0;
		v.ty = metrics.ty1;
		vertex->push_back( v );

		v.x = x + (float)metrics.width;
		v.y = y+height;
		v.tx = metrics.tx1;
		v.ty = metrics.ty1;
		vertex->push_back( v );

		v.x = x + (float)metrics.width;
		v.y = y;
		v.tx = metrics.tx1;
		v.ty = metrics.ty0;
		vertex->push_back( v );

		index->push_back( offset+0 );
		index->push_back( offset+1 );
		index->push_back( offset+2 );
		index->push_back( offset+0 );
		index->push_back( offset+2 );	// swapped for testing!
		index->push_back( offset+3 );

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
	m_indexBuffer.clear();
	m_vertexBuffer.clear();

	renderer->BeginRender();

	const void* renderState = 0;
	const void* textureHandle = 0;

	for( vector< GamItem* >::iterator it = m_itemArr.begin(); it != m_itemArr.end(); it++ ) {
		GamItem* item = *it;
		const RenderAtom* atom = item->GetCurrentAtom();

		// flush:
		if (    m_indexBuffer.size()
			 && ( atom->renderState != renderState || atom->textureHandle != textureHandle ) )
		{
			renderer->Render( renderState, textureHandle, (int)m_indexBuffer.size(), &m_indexBuffer[0], (int)m_vertexBuffer.size(), &m_vertexBuffer[0] );
			m_indexBuffer.clear();
			m_vertexBuffer.clear();
		}

		if ( atom->renderState != renderState ) {
			renderer->BeginRenderState( atom->renderState );
			renderState = atom->renderState;
		}
		if ( atom->textureHandle != textureHandle ) {
			renderer->BeginTexture( atom->textureHandle );
			textureHandle = atom->textureHandle;
		}

		item->Queue( &m_indexBuffer, &m_vertexBuffer );
	}
	// flush:
	if ( m_indexBuffer.size() ) {
		renderer->Render( renderState, textureHandle, (int)m_indexBuffer.size(), &m_indexBuffer[0], (int)m_vertexBuffer.size(), &m_vertexBuffer[0] );
	}

	renderer->EndRender();
}


