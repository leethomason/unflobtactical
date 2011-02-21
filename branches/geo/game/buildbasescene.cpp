#include "buildbasescene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/uirendering.h"
#include "chits.h"

using namespace grinliz;
using namespace gamui;

static const int facilityCost[ BuildBaseScene::NUM_FACILITIES ] = { 100, 100, 100, 100, 100 };

BuildBaseScene::BuildBaseScene( Game* _game, BuildBaseSceneData* data ) : Scene( _game )
{
	this->data = data;
	const Screenport& port = GetEngine()->GetScreenport();

	const gamui::ButtonLook& blue		= game->GetButtonLook( Game::BLUE_BUTTON );
	const gamui::ButtonLook& green		= game->GetButtonLook( Game::GREEN_BUTTON );

	backButton.Init( &gamui2D, blue );
	backButton.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	backButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	backButton.SetText( "Back" );

	static const float ORIGIN_X = 0;
	static const float ORIGIN_Y = 0;
	static const float SIZE = 256;

	RenderAtom mapAtom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, TextureManager::Instance()->GetTexture( "basemap" ), 0, 0, 1, 1, 256, 256 );
	mapImage.Init( &gamui2D, mapAtom, false );
	mapImage.SetPos( ORIGIN_X, ORIGIN_Y );

	static const Vector2F pos[NUM_FACILITIES] = {
		{ SIZE-GAME_BUTTON_SIZE_F, (SIZE-GAME_BUTTON_SIZE_F)/2 },
		{ SIZE-GAME_BUTTON_SIZE_F, 0 },
		{ (SIZE-GAME_BUTTON_SIZE_F)/2, 0 },
		{ 0, 0 },
		{ 0, (SIZE-GAME_BUTTON_SIZE_F)/2 }
	};
	static const char* names[NUM_FACILITIES] = { "Troops", "Missile", "Cargo", "Guns", "SciLab" };
	char buf[16];

	for( int i=0; i<NUM_FACILITIES; ++i ) {
		buyButton[i].Init( &gamui2D, green );
		buyButton[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		buyButton[i].SetPos( ORIGIN_X+pos[i].x, ORIGIN_Y+pos[i].y );
		buyButton[i].SetText( names[i] );

		SNPrintf( buf, 16, "$%d", facilityCost[i] );
		buyButton[i].SetText2( buf );

		progressLabel[i].Init( &gamui2D );
		progressLabel[i].SetCenterPos( ORIGIN_X+pos[i].x, ORIGIN_Y+pos[i].y );
		progressLabel[i].SetText( "Building..." );
	}

	UpdateButtons();
}


void BuildBaseScene::UpdateButtons()
{
	for( int i=0; i<NUM_FACILITIES; ++i ) {
		if ( data->baseChit->IsFacilityComplete( i ) ) {
			// Bought.
			buyButton[i].SetVisible( false );
			progressLabel[i].SetVisible( false );
		}
		else if ( data->baseChit->IsFacilityInProgress( i ) ) {
			// Being built. Can't buy.
			buyButton[i].SetVisible( false );
			progressLabel[i].SetVisible( true );
		}
		else {
			// Can purchase.
			buyButton[i].SetVisible( true );
			progressLabel[i].SetVisible( false );
			progressLabel[i].SetEnabled( *data->cash >= facilityCost[i] );
		}
	}
}


void BuildBaseScene::Tap(	int action, 
							const grinliz::Vector2F& screen,
							const grinliz::Ray& world )
{
	Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( screen, &ui );

	const UIItem* item = 0;
	if ( action == GAME_TAP_DOWN ) {
		gamui2D.TapDown( ui.x, ui.y );
		return;
	}
	else if ( action == GAME_TAP_MOVE ) {
		return;
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		return;
	}
	else if ( action == GAME_TAP_UP ) {
		item = gamui2D.TapUp( ui.x, ui.y );
	}

	if ( item == &backButton ) {
		game->PopScene( 0 );
		return;
	}
	for( int i=0; i<NUM_FACILITIES; ++i ) {
		if ( item == &buyButton[i] ) {
			if ( *data->cash >= facilityCost[i] ) {
				*data->cash -= facilityCost[i];
				data->baseChit->BuildFacility( i );
			}
			UpdateButtons();
		}
	}
}
