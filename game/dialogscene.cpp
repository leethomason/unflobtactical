#include "dialogscene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/engine.h"
#include "../engine/uirendering.h"
#include "../grinliz/glstringutil.h"

using namespace gamui;

DialogScene::DialogScene( Game* _game, const DialogSceneData* _data ) : Scene( _game ), data( _data )
{
	Engine* engine = GetEngine();
	engine->EnableMap( false );
	const Screenport& port = engine->GetScreenport();

	static const float GUTTER = 20.0f;

	// 248, 228, 8
	const float INV = 1.f/255.f;
	//uiRenderer.SetTextColor( 248.f*INV, 228.f*INV, 8.f*INV );

	RenderAtom roundRectangle = UIRenderer::CalcPaletteAtom(	UIRenderer::PALETTE_GREEN, 
																UIRenderer::PALETTE_GREEN, 
																UIRenderer::PALETTE_DARK, 
																10, 10, true );

	background.Init( &gamui2D, roundRectangle, false );
	background.SetSlice( true );
	background.SetPos( 0, 0 );
	background.SetSize( port.UIWidth(), port.UIHeight() );

	textBox.Init( &gamui2D );
	textBox.SetPos( GUTTER, GUTTER );
	textBox.SetSize( port.UIWidth()-GUTTER*2.0f, port.UIHeight()-GUTTER*2.0f );
	textBox.SetText( data->text.c_str() );

	const ButtonLook& look = game->GetButtonLook( Game::BLUE_BUTTON );

	button0.Init( &gamui2D, look );
	button0.SetSize( GAME_BUTTON_SIZE_F*2.0f, GAME_BUTTON_SIZE_F );
	button0.SetPos( GUTTER, port.UIHeight()-GUTTER-GAME_BUTTON_SIZE_F );

	button1.Init( &gamui2D, look );
	button1.SetSize( GAME_BUTTON_SIZE_F*2.0f, GAME_BUTTON_SIZE_F );
	button1.SetPos( port.UIWidth()-GUTTER-GAME_BUTTON_SIZE_F*2.0f, port.UIHeight()-GUTTER-GAME_BUTTON_SIZE_F );

	switch ( data->type ) {
	case DialogSceneData::DS_YESNO:
		button0.SetText( "No" );
		button1.SetText( "Yes" );
		break;

	default:
		button0.SetText( "Cancel" );
		button1.SetText( "Okay" );
		break;

	}
}



void DialogScene::Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )
{
	grinliz::Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( screen, &ui );

	const UIItem* item = 0;
	if ( action == GAME_TAP_DOWN ) {
		gamui2D.TapDown( ui.x, ui.y );
		return;
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		return;
	}
	else if ( action == GAME_TAP_UP ) {
		item = gamui2D.TapUp( ui.x, ui.y );
	}

	if ( item == &button0 ) {
		game->PopScene( 0 );
	}
	else if ( item == &button1 ) {
		game->PopScene( 1 );
	}
}
