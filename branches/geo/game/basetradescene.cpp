#include "basetradescene.h"
#include "game.h"
#include "cgame.h"
#include "storageWidget.h"
#include "unit.h"
#include "tacticalintroscene.h"

using namespace grinliz;
using namespace gamui;

static const int SOLDIER_COST   = 80;
static const int SCIENTIST_COST = 120;

BaseTradeScene::BaseTradeScene( Game* _game, BaseTradeSceneData* data ) : Scene( _game )
{
	this->data = data;
	const Screenport& port = GetEngine()->GetScreenport();

	originalBase = new Storage( *data->base );
	originalRegion = new Storage( *data->region );

	static const float TEXTSPACE = 16.0f;

	backgroundUI.Init( _game, &gamui2D, false );
	
	const gamui::ButtonLook& tab = _game->GetButtonLook( Game::BLUE_TAB_BUTTON );
	const gamui::ButtonLook& blue = _game->GetButtonLook( Game::BLUE_BUTTON );
	const gamui::ButtonLook& green = _game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& red = _game->GetButtonLook( Game::RED_BUTTON );

#if 0
	cancel.Init( &gamui2D, blue );
	cancel.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	cancel.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	cancel.SetText( "Cancel" );
#endif

	okay.Init( &gamui2D, blue );
	okay.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	okay.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	okay.SetText( "Okay" );

	hireSoldier.Init( &gamui2D, blue );
	hireSoldier.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	hireSoldier.SetPos( GAME_BUTTON_SIZE_F*1.0f, port.UIHeight()-GAME_BUTTON_SIZE_F ); 
	hireSoldier.SetDeco( UIRenderer::CalcDecoAtom( DECO_CHARACTER, true ), UIRenderer::CalcDecoAtom( DECO_CHARACTER, false ) );

	hireScientist.Init( &gamui2D, blue );
	hireScientist.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	hireScientist.SetPos( GAME_BUTTON_SIZE_F*2.0f, port.UIHeight()-GAME_BUTTON_SIZE_F ); 
	hireScientist.SetDeco( UIRenderer::CalcDecoAtom( DECO_RESEARCH, true ), UIRenderer::CalcDecoAtom( DECO_RESEARCH, false ) );

	baseWidget = new StorageWidget( &gamui2D, green, tab, game->GetItemDefArr(), data->base );
	baseWidget->SetFudgeFactor( -5, -5 );
	baseWidget->SetOrigin( 0, TEXTSPACE );

	regionWidget = new StorageWidget( &gamui2D, green, tab, game->GetItemDefArr(), data->region, data->costMult );
	regionWidget->SetFudgeFactor( -5, -5 );
	regionWidget->SetOrigin( (float)port.UIWidth()-regionWidget->Width(), TEXTSPACE );

	baseLabel.Init( &gamui2D );
	baseLabel.SetPos( 0, 0 );
	baseLabel.SetText( data->baseName );

	regionLabel.Init( &gamui2D );
	regionLabel.SetPos( regionWidget->X(), 0 );
	regionLabel.SetText( data->regionName );

	const float PROFIT_X = regionWidget->X();

	profitLabel.Init( &gamui2D );
	profitLabel.SetPos( PROFIT_X, regionWidget->Y()+regionWidget->Height() );

	lossLabel.Init( &gamui2D );
	lossLabel.SetPos( profitLabel.X(), profitLabel.Y()+TEXTSPACE );

	totalLabel.Init( &gamui2D );
	totalLabel.SetPos( profitLabel.X(), profitLabel.Y()+TEXTSPACE*2 );

	remainLabel.Init( &gamui2D );
	remainLabel.SetPos( profitLabel.X(), profitLabel.Y()+TEXTSPACE*3 );

	ComputePrice( 0 );
	SetHireButtons();
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


	if ( uiItem == &okay ) {
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
	else if ( uiItem == &hireSoldier ) {
		if ( *data->cash >= SOLDIER_COST ) {

			*data->cash -= SOLDIER_COST;
			int nSoldiers = Unit::Count( data->soldiers, MAX_TERRANS, Unit::STATUS_ALIVE );
			GLASSERT( nSoldiers < MAX_TERRANS && !data->soldiers[nSoldiers].InUse() );

			int seed = *data->cash ^ nSoldiers;
			TacticalIntroScene::GenerateTerranTeam( &data->soldiers[nSoldiers], 1, 
													data->soldierBoost ? 0.0f : 0.5f,
													game->GetItemDefArr(),
													seed );
			ComputePrice( 0 );
			SetHireButtons();
		}
	}
	else if ( uiItem == &hireScientist ) {
		if ( *data->cash >= SCIENTIST_COST ) {
			*data->cash -= SCIENTIST_COST;
			*data->scientists = *data->scientists + 1;
			ComputePrice( 0 );
			SetHireButtons();
		}
	}
	else {
		const ItemDef* itemDef = 0;
		itemDef = baseWidget->ConvertTap( uiItem );
		if ( itemDef ) {
			Item item;
			if ( data->base->RemoveItem( itemDef, &item ) ) {
				data->region->AddItem( item );

				if ( !ComputePrice( 0 ) ) {
					// unroll
					data->base->AddItem( item );
					data->region->RemoveItem( item.GetItemDef(), &item );
				}
			}
		}

		itemDef = regionWidget->ConvertTap( uiItem );
		if ( itemDef ) {
			Item item;
			if ( data->region->RemoveItem( itemDef, &item ) ) {
				data->base->AddItem( item );

				if ( !ComputePrice( 0 ) ) {
					// unroll
					data->region->AddItem( item );
					data->base->RemoveItem( item.GetItemDef(), &item );
				}
			}
		}
	};

	baseWidget->SetButtons();
	regionWidget->SetButtons();
}


void BaseTradeScene::SetHireButtons()
{
	int nSoldiers = Unit::Count( data->soldiers, MAX_TERRANS, Unit::STATUS_ALIVE );
	static const int SZ=8;
	char buf[SZ];

	SNPrintf( buf, SZ, "%d/%d", nSoldiers, MAX_TERRANS );
	hireSoldier.SetText( buf );
	SNPrintf( buf, SZ, "$%d", SOLDIER_COST );
	hireSoldier.SetText2( buf );
	hireSoldier.SetEnabled( nSoldiers < MAX_TERRANS );

	if ( data->scientists ) {
		SNPrintf( buf, SZ, "%d/%d", *data->scientists, MAX_SCIENTISTS );
		hireScientist.SetText( buf );
		SNPrintf( buf, SZ, "$%d", SCIENTIST_COST );
		hireScientist.SetText2( buf );
		hireScientist.SetEnabled( *data->scientists < MAX_SCIENTISTS );
	}
	else {
		hireScientist.SetVisible( false );
	};
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
			buy += (currentCount - originalCount ) * LRintf( (float)itemDef->price * data->costMult );
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

	if ( _total )
		*_total = total;
	okay.SetEnabled( *data->cash + total > 0 );
	return okay.Enabled();
}

