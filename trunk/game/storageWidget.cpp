#include "storageWidget.h"

StorageWidget::StorageWidget(	const Screenport& port, 
								const Storage* _storage )
	: storage( _storage )
{
	//this->buttonTexture = texture;
	//this->decoTexture = decoTexture;
//	index = -1;
	valid = false;

	selectWidget = new UIButtonBox( port );
	boxWidget = new UIButtonBox( port );

	{
		int icons[4] = { ICON_BLUE_BUTTON, ICON_BLUE_BUTTON, ICON_BLUE_BUTTON, ICON_BLUE_BUTTON };
		selectWidget->InitButtons( icons, 4 );
		selectWidget->SetColumns( 1 );
		selectWidget->SetAlpha( 0.8f );
	}

	{
		int icons[12] = { ICON_GREEN_BUTTON };
		boxWidget->InitButtons( icons, 12 );
		boxWidget->SetColumns( 3 );
		boxWidget->SetAlpha( 0.8f );
		groupSelected = 0;
	}	
	SetButtonSize( 60, 60 );
	SetPadding( 0, 0 );
	SetOrigin( 0, 0 );
}


StorageWidget::~StorageWidget()
{
	delete selectWidget;
	delete boxWidget;
}


void StorageWidget::CalcBounds( grinliz::Rectangle2I* _bounds )
{
	grinliz::Rectangle2I b0, b1;
	selectWidget->CalcBounds( &b0 );
	boxWidget->CalcBounds( &b1 );

	*_bounds = b0;
	_bounds->DoUnion( b1 );
}


void StorageWidget::SetOrigin( int x, int y )
{
	selectWidget->SetOrigin( x, y );
	boxWidget->SetOrigin( x + selectWidget->GetButtonSize().x + selectWidget->GetPadding().x, y );
}


void StorageWidget::SetButtonSize( int dx, int dy ) 
{
	selectWidget->SetButtonSize( dx, dy );
	boxWidget->SetButtonSize( dx, dy );
}


void StorageWidget::SetPadding( int dx, int dy )
{
	selectWidget->SetPadding( dx, dy );
	boxWidget->SetPadding( dx, dy );
}


const ItemDef* StorageWidget::Tap( int ux, int uy )
{
	const ItemDef* result = 0;
	int tap = selectWidget->QueryTap( ux, uy );

	if ( tap >= 0 ) {
		if ( tap != groupSelected ) {
			groupSelected = tap;
			valid = false;
		}
	}
	else {
		tap = boxWidget->QueryTap( ux, uy );
		if ( tap >= 0 && itemDefMap[tap] && storage->GetCount( itemDefMap[tap] )>0 ) {
			result = itemDefMap[tap];
		}
	}
	return result;
}

	
void StorageWidget::SetButtons()
{
	int itemsPerGroup[4] = { 0, 0, 0, 0 };

	if ( !valid ) {
		// First the selection.
		for( int i=0; i<4; ++i ) {
			selectWidget->SetButton( i, (i==groupSelected) ? ICON_BLUE_BUTTON_DOWN : ICON_BLUE_BUTTON );
		}
		selectWidget->SetDeco( 0, DECO_PISTOL );
		selectWidget->SetDeco( 1, DECO_RAYGUN );
		selectWidget->SetDeco( 2, DECO_ARMOR );
		selectWidget->SetDeco( 3, DECO_ALIEN );

		// Then the storage.
		int slot = 0;

		const CDynArray<ItemDef*>& itemArr = storage->GetItemDefArray();

		for( unsigned i=0; i<itemArr.Size(); ++i ) {
			bool add = false;
			int group = 3;

			const WeaponItemDef* wid = itemArr[i]->IsWeapon();
			const ClipItemDef* cid = itemArr[i]->IsClip();

			// Terran non-melee weapons and clips.
			if ( wid && wid->weapon[0].power == 0 && wid->weapon[0].range > 1 )
				group=0;
			else if ( cid && cid->type != ITEM_CLIP_CELL )
				group=0;

			// alien non-melee weapons and clips
			if ( wid && wid->weapon[0].power > 0 && wid->weapon[0].range > 1 )
				group=1;
			else if ( cid && cid->type == ITEM_CLIP_CELL )
				group=1;

			// armor, melee
			if ( wid && wid->weapon[0].range == 1 )
				group=2;
			if ( itemArr[i]->IsArmor() )
				group=2;

			itemsPerGroup[group] += storage->GetCount( itemArr[i] );

			if ( group==groupSelected ) {
				GLASSERT( slot < 12 );
				if ( slot < 12 ) {
					itemDefMap[slot] = itemArr[i];
					boxWidget->SetDeco( slot, itemArr[i]->deco );

					char buffer[16];
					if ( storage->GetCount( itemArr[i] ) ) {
						sprintf( buffer, "(%d)", storage->GetCount( itemArr[i] ) );
						boxWidget->SetText( slot, itemArr[i]->name, buffer );
						boxWidget->SetEnabled( slot, true );
					}
					else {
						boxWidget->SetText( slot, itemArr[i]->name, " " );
						boxWidget->SetEnabled( slot, false );
					}
				}
				++slot;
			}
		}
		for( ; slot<12; ++slot ) {
			itemDefMap[slot] = 0;
			boxWidget->SetDeco( slot, DECO_NONE );
			boxWidget->SetText( slot, 0, 0 );
			boxWidget->SetEnabled( slot, false );
		}

		for( int i=0; i<4; ++i ) {
			selectWidget->SetEnabled( i, itemsPerGroup[i]>0 );
		}

		valid = true;
	}
}


void StorageWidget::Draw()
{
	if ( !valid ) {
		SetButtons();
	}
	selectWidget->Draw();
	boxWidget->Draw();
}
