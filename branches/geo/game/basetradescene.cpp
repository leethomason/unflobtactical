#include "basetradescene.h"
#include "game.h"
#include "cgame.h"
#include "storageWidget.h"

using namespace grinliz;
using namespace gamui;

BaseTradeScene::BaseTradeScene( Game* _game, BaseTradeSceneData* data ) : Scene( _game )
{
	this->data = data;
	const Screenport& port = GetEngine()->GetScreenport();

	originalBase = new Storage( *data->base );
	originalRegion = new Storage( *data->region );

	static const float TEXTSPACE = 16.0f;
	const float PROFIT_X = port.UIWidth() / 2;

	backgroundUI.Init( _game, &gamui2D, false );
	
	const gamui::ButtonLook& blue = _game->GetButtonLook( Game::BLUE_BUTTON );
	const gamui::ButtonLook& green = _game->GetButtonLook( Game::GREEN_BUTTON );

	cancel.Init( &gamui2D, blue );
	cancel.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	cancel.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	cancel.SetText( "Cancel" );

	okay.Init( &gamui2D, blue );
	okay.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	okay.SetPos( port.UIWidth()-GAME_BUTTON_SIZE_F, port.UIHeight()-GAME_BUTTON_SIZE_F );
	okay.SetText( "Okay" );

	baseWidget = new StorageWidget( &gamui2D, green, blue, game->GetItemDefArr(), data->base );
	baseWidget->SetOrigin( 0, TEXTSPACE );

	regionWidget = new StorageWidget( &gamui2D, green, blue, game->GetItemDefArr(), data->region );
	regionWidget->SetOrigin( (float)port.UIWidth()-regionWidget->Width(), TEXTSPACE );

	baseLabel.Init( &gamui2D );
	baseLabel.SetPos( 0, 0 );
	baseLabel.SetText( "Base" );

	regionLabel.Init( &gamui2D );
	regionLabel.SetPos( regionWidget->X(), 0 );
	regionLabel.SetText( "Region" );

	profitLabel.Init( &gamui2D );
	profitLabel.SetPos( PROFIT_X, regionWidget->Y()+regionWidget->Height() );

	lossLabel.Init( &gamui2D );
	lossLabel.SetPos( profitLabel.X(), profitLabel.Y()+TEXTSPACE );

	totalLabel.Init( &gamui2D );
	totalLabel.SetPos( profitLabel.X(), profitLabel.Y()+TEXTSPACE*2 );

	remainLabel.Init( &gamui2D );
	remainLabel.SetPos( profitLabel.X(), profitLabel.Y()+TEXTSPACE*3 );
	//CStr<16> buf = *data->cash;
	//remainLabel.SetText( buf.c_str() );
}


BaseTradeScene::~BaseTradeScene()
{
	delete baseWidget;
	delete regionWidget;
	delete originalBase;
	delete originalRegion;
}


void BaseTradeScene::Activate()
{
	GetEngine()->SetMap( 0 );
}


void BaseTradeScene::Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )
{
	grinliz::Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( screen, &ui );

	const UIItem* uiItem = 0;
	if ( action == GAME_TAP_DOWN ) {
		gamui2D.TapDown( ui.x, ui.y );
		return;
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		return;
	}
	else if ( action == GAME_TAP_UP ) {
		uiItem = gamui2D.TapUp( ui.x, ui.y );
	}

	bool priceChange = false;
	if ( uiItem == &cancel ) {
		game->PopScene( 0 );
		*data->base = *originalBase;
		*data->region = *originalRegion;
	}
	else if ( uiItem == &okay ) {
		game->PopScene( 0 );
		int total = 0;
		if ( ComputePrice( &total ) ) {
			// Can afford:
			*data->cash += total;
		}
		else {
			// Can't pay:
			*data->base = *originalBase;
			*data->region = *originalRegion;
		}
	}
	else {
		const ItemDef* itemDef = 0;
		itemDef = baseWidget->ConvertTap( uiItem );
		if ( itemDef ) {
			Item item;
			if ( data->base->RemoveItem( itemDef, &item ) ) {
				data->region->AddItem( item );
				priceChange = true;
			}
		}

		itemDef = regionWidget->ConvertTap( uiItem );
		if ( itemDef ) {
			Item item;
			if ( data->region->RemoveItem( itemDef, &item ) ) {
				data->base->AddItem( item );
				priceChange = true;
			}
		}
	};

	baseWidget->SetButtons();
	regionWidget->SetButtons();

	if ( priceChange ) {
		int total;
		ComputePrice( &total );
	}
}


bool BaseTradeScene::ComputePrice( int* _total )
{
	int nSell=0, sell=0;
	int nBuy=0,  buy=0;

	for( int i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		int currentCount = data->base->GetCount( i );
		int originalCount = originalBase->GetCount( i );
		const ItemDef* itemDef = game->GetItemDefArr().GetIndex( i );

		if ( currentCount > originalCount ) {
			// Bought stuff.
			nBuy += (currentCount - originalCount );
			buy += (currentCount - originalCount ) * itemDef->price;
		}
		if ( currentCount < originalCount ) {
			// Sold stuff.
			nSell += (originalCount - currentCount);
			sell += (originalCount - currentCount) * itemDef->price;
		}
	}
	int total = sell - buy;

	CStr<16> buf, val;

	buf = "Buy: ";
	val = buy;
	buf += val.c_str();
	lossLabel.SetText( buf.c_str() );

	buf = "Sell: ";
	val = sell;
	buf += val.c_str();
	profitLabel.SetText( buf.c_str() );

	buf = "Total: ";
	val = (sell-buy);
	buf += val.c_str();
	totalLabel.SetText( buf.c_str() );

	buf = "Remaining: ";
	val = *data->cash + (sell-buy);
	buf += val.c_str();
	remainLabel.SetText( buf.c_str() );

	*_total = total;
	okay.SetEnabled( *data->cash + total > 0 );
	return okay.Enabled();
}

