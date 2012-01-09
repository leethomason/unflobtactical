#include "consolewidget.h"

ConsoleWidget::ConsoleWidget( gamui::Gamui* g )
{
	decay = 0;
	gamui = g;
	for( int i=0; i<MAX_TEXT; ++i ) {
		text[i].Init( gamui );
	}
	SetOrigin( 0, 0 );
}


void ConsoleWidget::SetOrigin( float x, float y ) 
{
	for( int i=0; i<MAX_TEXT; ++i ) {
		text[i].SetPos( x, y+(float)i*gamui->GetTextHeight() );
	}
}


void ConsoleWidget::DoTick( U32 delta ) 
{
	if ( decay <= delta ) {
		for( int i=0; i<MAX_TEXT-1; ++i ) {
			text[i].SetText( text[i+1].GetText() );
		}
		text[MAX_TEXT-1].SetText( "" );
		decay = DECAY;
	}
	else {
		decay -= delta;
	}
}


void ConsoleWidget::PushMessage( const char* message ) 
{
	const char* lastSlot = text[MAX_TEXT-1].GetText();
	if ( lastSlot && *lastSlot ) {
		DoTick( DECAY );
	}
	// find an open slot.
	for( int i=0; i<MAX_TEXT; ++i ) {
		const char* t = text[i].GetText();
		if ( !t || !*t ) {
			text[i].SetText( message );
			if ( i == 0 ) {
				decay = DECAY;
			}
			break;
		}
	}
}
