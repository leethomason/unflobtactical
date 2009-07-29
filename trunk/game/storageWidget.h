#ifndef UFOATTACK_STORAGE_WIDGET_INCLUDED
#define UFOATTACK_STORAGE_WIDGET_INCLUDED

#include "../engine/uirendering.h"
#include "item.h"
#include "../engine/ufoutil.h"

class StorageWidget
{
public:
	StorageWidget(	const Texture* texture, const Texture* decoTexture, 
					const Screenport& port,
					const Storage* storage );

	~StorageWidget();

	bool Tap( int x, int y );	
	void Draw();

	void SetOrigin( int x, int y );
	void SetButtonSize( int dx, int dy );
	void SetPadding( int dx, int dy );

	void CalcBounds( grinliz::Rectangle2I* _bounds );

	const ItemDef* TappedItemDef();
	int TappedIndex()				{ return index; }

private:
	void SetButtons();

	UIButtonBox* selectWidget;
	UIButtonBox* boxWidget;

	const Texture* buttonTexture;
	const Texture* decoTexture;

	int groupSelected;
	//const CDynArray<ItemDef*>& itemArr;
	//const CDynArray<ItemDefInfo>& infoArr;
	const Storage* storage;
	int itemDefMap[12];

	int index;
};


#endif // UFOATTACK_STORAGE_WIDGET_INCLUDED
