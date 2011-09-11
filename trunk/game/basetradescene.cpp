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

#include "basetradescene.h"
#include "game.h"
#include "cgame.h"
#include "storageWidget.h"
#include "unit.h"
#include "tacticalintroscene.h"
#include "helpscene.h"

using namespace grinliz;
using namespace gamui;

static const int SOLDIER_COST   = 80;
static const int SCIENTIST_COST = 120;

BaseTradeScene::BaseTradeScene( Game* _game, BaseTradeSceneData* data ) : Scene( _game )
{
	this->data = data;
	const Screenport& port = GetEngine()->GetScreenport();

	data->base->ClearItem( "Soldr" );
	data->base->ClearItem( "Sctst" );
	data->region.ClearItem( "Soldr" );
	data->region.ClearItem( "Sctst" );

	minSoldiers = 0;

	if ( data->soldiers ) {
		minSoldiers = Unit::Count( data->soldiers, MAX_TERRANS, Unit::STATUS_ALIVE );
		data->base->AddItem( "Soldr", minSoldiers );
		data->region.AddItem( "Soldr", MAX_TERRANS - minSoldiers );
	}
	if ( data->scientists ) {
		data->base->AddItem( "Sctst", *data->scientists );
		data->region.AddItem( "Sctst", MAX_SCIENTISTS - *data->scientists );
	}

	originalBase = new Storage( *data->base );
	originalRegion = new Storage( data->region );

	static const float TEXTSPACE = 16.0f;

	backgroundUI.Init( _game, &gamui2D, false );
	backgroundUI.background.SetVisible( false );
	
	const gamui::ButtonLook& tab = _game->GetButtonLook( Game::BLUE_TAB_BUTTON );
	const gamui::ButtonLook& blue = _game->GetButtonLook( Game::BLUE_BUTTON );
	const gamui::ButtonLook& green = _game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& red = _game->GetButtonLook( Game::RED_BUTTON );

	helpButton.Init( &gamui2D, green );
	helpButton.SetPos( port.UIWidth()-GAME_BUTTON_SIZE_F, port.UIHeight()-GAME_BUTTON_SIZE_F );
	helpButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	helpButton.SetDeco(  UIRenderer::CalcDecoAtom( DECO_HELP, true ), UIRenderer::CalcDecoAtom( DECO_HELP, false ) );	

	okay.Init( &gamui2D, blue );
	okay.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	okay.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	okay.SetText( "Okay" );

	float w = GAME_BUTTON_SIZE_F*0.95f;
	float h = GAME_BUTTON_SIZE_F*0.95f;

	baseWidget = new StorageWidget( &gamui2D, green, tab, game->GetItemDefArr(), data->base, w, h );
	baseWidget->SetFudgeFactor( -5, -5 );
	baseWidget->SetOrigin( 0, TEXTSPACE );

	sellAll.Init( &gamui2D, red );
	sellAll.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	sellAll.SetPos( baseWidget->X()+baseWidget->Width()-GAME_BUTTON_SIZE_F, port.UIHeight()-GAME_BUTTON_SIZE_F ); 
	sellAll.SetText( "Sell" );
	sellAll.SetText2( "All" );

	regionWidget = new StorageWidget( &gamui2D, green, tab, game->GetItemDefArr(), &data->region, w, h, data->costMult );
	regionWidget->SetFudgeFactor( -5, -5 );
	regionWidget->SetOrigin( (float)port.UIWidth()-regionWidget->Width(), TEXTSPACE );
	regionWidget->SetInfoVisible( false );

	baseLabel.Init( &gamui2D );
	baseLabel.SetPos( 0, 0 );
	baseLabel.SetText( data->baseName );

	regionLabel.Init( &gamui2D );
	regionLabel.SetPos( regionWidget->X(), 0 );
	regionLabel.SetText( data->regionName );

	const float PROFIT_X = regionWidget->X();

	profitLabel.Init( &gamui2D );
	profitLabel.SetPos( PROFIT_X, 320.f-TEXTSPACE*4.f );

	lossLabel.Init( &gamui2D );
	lossLabel.SetPos( profitLabel.X(), 320.f-TEXTSPACE*3.f );

	totalLabel.Init( &gamui2D );
	totalLabel.SetPos( profitLabel.X(), 320.f-TEXTSPACE*2.f );

	remainLabel.Init( &gamui2D );
	remainLabel.SetPos( profitLabel.X(), 320.f-TEXTSPACE*1.f );

	ComputePrice( 0 );
	//SetHireButtons();
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


	if ( uiItem == &helpButton ) {
		game->PushScene( Game::HELP_SCENE, new HelpSceneData( "supplyBaseHelp", false ) );
	}
	if ( uiItem == &okay ) {
		game->PopScene( 0 );
		ComputePrice( data->cash );
		const ItemDef* soldierDef = game->GetItemDefArr().Query( "Soldr" );

		if ( data->soldiers ) {
			// Generate new soldiers.
			int nSoldiers = data->base->GetCount( soldierDef );

			if ( nSoldiers < minSoldiers ) {
				for( int i=nSoldiers; i<minSoldiers; ++i ) {
					data->soldiers[i].Free();
					memset( &data->soldiers[i], 0, sizeof(Unit) );
				}	
			}
			else if ( nSoldiers > minSoldiers ) {
				for( int i=minSoldiers; i<nSoldiers; ++i ) {
					data->soldiers[i].Free();
					memset( &data->soldiers[i], 0, sizeof(Unit) );
				}
				int seed = *data->cash ^ nSoldiers;
				TacticalIntroScene::GenerateTerranTeam( &data->soldiers[minSoldiers], nSoldiers-minSoldiers, 
														data->soldierBoost ? 0.0f : 0.5f,
														game->GetItemDefArr(),
														seed );
			}
		}		
		// And scientists
		const ItemDef* sDef = 0;
		if ( data->scientists ) {
			sDef = game->GetItemDefArr().Query( "Sctst" );
			*data->scientists = data->base->GetCount( sDef );
		}

		// And clear out the fake items:
		Item dummy;
		while( data->base->Contains( soldierDef ) )
			data->base->RemoveItem( soldierDef, &dummy );
		if ( data->scientists ) {
			while( data->base->Contains( sDef ) )
				data->base->RemoveItem( sDef, &dummy );
		}
	}
	else {
		const ItemDef* itemDef = 0;
		itemDef = baseWidget->ConvertTap( uiItem );
		if ( itemDef ) {
			Item item;
			while( data->base->Contains( itemDef ) ) {
				if ( data->base->RemoveItem( itemDef, &item ) ) {
					data->region.AddItem( item );
				}
				if ( !sellAll.Down() )
					break;
			}
		}

		itemDef = regionWidget->ConvertTap( uiItem );
		if ( itemDef ) {
			Item item;
			if ( data->region.RemoveItem( itemDef, &item ) ) {
				data->base->AddItem( item );

				if ( !ComputePrice( 0 ) ) {
					// unroll
					data->region.AddItem( item );
					data->base->RemoveItem( item.GetItemDef(), &item );
				}
			}
		}
	}

	ComputePrice( 0 );
	baseWidget->SetButtons();
	regionWidget->SetButtons();
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
			buy += (currentCount - originalCount ) * itemDef->Price( data->costMult );
		}
		if ( currentCount < originalCount ) {
			// Sold stuff.
			nSell += (originalCount - currentCount);
			sell += (originalCount - currentCount) * abs( itemDef->price );
		}
	}
	//int total = sell - buy;

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

	int remaining = *data->cash + (sell-buy);
	buf = "Remaining: ";
	val = remaining;
	buf += val.c_str();
	remainLabel.SetText( buf.c_str() );

	if ( _total )
		*_total = remaining;

	return    (remaining >= 0);
}

