#ifndef UFOATTACK_STORAGE_WIDGET_INCLUDED
#define UFOATTACK_STORAGE_WIDGET_INCLUDED

#include "../engine/uirendering.h"
#include "item.h"
#include "../engine/ufoutil.h"

class StorageWidget
{
public:
	StorageWidget(	const Screenport& port,
					ItemDef* const* itemDefArr,
					const Storage* storage );

	~StorageWidget();

	const ItemDef* Tap( int x, int y );	
	void Draw();

	void SetOrigin( int x, int y );
	void SetButtonSize( int dx, int dy );
	void SetPadding( int dx, int dy );

	void CalcBounds( grinliz::Rectangle2I* _bounds );
	void Update()	{ valid = false; }

private:
	void SetButtons();

	bool valid;
	UIButtonBox* selectWidget;
	UIButtonBox* boxWidget;

	int groupSelected;
	const Storage* storage;
	const ItemDef* itemDefMap[12];
	ItemDef* const* itemDefArr;
};


#endif // UFOATTACK_STORAGE_WIDGET_INCLUDED
