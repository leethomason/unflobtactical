#include "buildbasescene.h"
#include "game.h"
#include "cgame.h"
#include "../engine/uirendering.h"
#include "chits.h"
#include "helpscene.h"

using namespace grinliz;
using namespace gamui;

static const int   facilityCost[BuildBaseScene::NUM_FACILITIES]  = { 100, 100, 100, 100, 100 };
static const char* facilityNames[BuildBaseScene::NUM_FACILITIES] 
     = { "Troops", "Missile", "Cargo", "Guns", "SciLab" };


BuildBaseScene::BuildBaseScene( Game* _game, BuildBaseSceneData* data ) : Scene( _game )
{
	this->data = data;
	const Screenport& port = GetEngine()->GetScreenport();

	const gamui::ButtonLook& blue		= game->GetButtonLook( Game::BLUE_BUTTON );
	const gamui::ButtonLook& green		= game->GetButtonLook( Game::GREEN_BUTTON );

	backButton.Init( &gamui2D, blue );
	backButton.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	backButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	//backButton.SetText( "Back" );
	backButton.SetDeco(	UIRenderer::CalcDecoAtom( DECO_OKAY_CHECK, true ),
						UIRenderer::CalcDecoAtom( DECO_OKAY_CHECK, false ) );	

	helpButton.Init( &gamui2D, green );
	helpButton.SetPos( port.UIWidth()-GAME_BUTTON_SIZE_F, 0 );
	helpButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	helpButton.SetDeco(  UIRenderer::CalcDecoAtom( DECO_HELP, true ), UIRenderer::CalcDecoAtom( DECO_HELP, false ) );	

	static const float ORIGIN_X = (port.UIWidth()-256.0f)/2.0f;
	static const float ORIGIN_Y = 0;
	static const float SIZE = 256;

	RenderAtom mapAtom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, TextureManager::Instance()->GetTexture( "basemap" ), 0, 0, 1, 1 );
	mapImage.Init( &gamui2D, mapAtom, false );
	mapImage.SetPos( ORIGIN_X, ORIGIN_Y );
	mapImage.SetSize( SIZE, SIZE );

	static const Vector2F pos[NUM_FACILITIES] = {
		{ SIZE-GAME_BUTTON_SIZE_F, (SIZE-GAME_BUTTON_SIZE_F)/2 },
		{ SIZE-GAME_BUTTON_SIZE_F, 0 },
		{ (SIZE-GAME_BUTTON_SIZE_F)/2, 0 },
		{ 0, 0 },
		{ 0, (SIZE-GAME_BUTTON_SIZE_F)/2 }
	};
	char buf[16];

	for( int i=0; i<NUM_FACILITIES; ++i ) {
		buyButton[i].Init( &gamui2D, green );
		buyButton[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		buyButton[i].SetPos( ORIGIN_X+pos[i].x, ORIGIN_Y+pos[i].y );
		buyButton[i].SetText( facilityNames[i] );

		SNPrintf( buf, 16, "$%d", facilityCost[i] );
		buyButton[i].SetText2( buf );

		progressLabel[i].Init( &gamui2D );
		progressLabel[i].SetCenterPos( ORIGIN_X+pos[i].x, ORIGIN_Y+pos[i].y );
		progressLabel[i].SetPos( progressLabel[i].X(), ORIGIN_Y+pos[i].y );
		progressLabel[i].SetText( "Building..." );
	}

	cashImage.Init( &gamui2D, UIRenderer::CalcIconAtom( ICON_GREEN_STAND_MARK ), false );
	cashImage.SetPos( port.UIWidth()-GAME_BUTTON_SIZE_F*2.0f, port.UIHeight()-GAME_BUTTON_SIZE_F*0.5f );
	cashImage.SetSize( GAME_BUTTON_SIZE_F*2.0f, GAME_BUTTON_SIZE_F );
	cashImage.SetSlice( true );

	cashLabel.Init( &gamui2D );
	cashLabel.SetPos( cashImage.X()+10.0f, cashImage.Y()+10.0f );

	UpdateButtons();
}


void BuildBaseScene::UpdateButtons()
{
	for( int i=0; i<NUM_FACILITIES; ++i ) {
		if ( data->baseChit->IsFacilityComplete( i ) ) {
			// Bought.
			buyButton[i].SetVisible( false );
			progressLabel[i].SetVisible( true );
			progressLabel[i].SetText( facilityNames[i] );
		}
//		else if ( data->baseChit->IsFacilityInProgress( i ) ) {
//			// Being built. Can't buy.
//			buyButton[i].SetVisible( false );
//			progressLabel[i].SetVisible( true );
//		}
		else {
			// Can purchase.
			buyButton[i].SetVisible( true );
			progressLabel[i].SetVisible( false );
			progressLabel[i].SetEnabled( *data->cash >= facilityCost[i] );
		}
	}
	char cashBuf[16];
	SNPrintf( cashBuf, 16, "$%d", *data->cash );
	cashLabel.SetText( cashBuf );
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
	else if ( item == &helpButton ) {
		game->PushScene( Game::HELP_SCENE, new HelpSceneData( "buildBaseHelp", false ) );
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
