#ifndef UFOATTACK_FIREWIDGET_INCLUDED
#define UFOATTACK_FIREWIDGET_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../gamui/gamui.h"
#include "item.h"

class Unit;
class BattleScene;


class FireWidget
{
public:
	FireWidget()	{}
	~FireWidget()	{}

	void Init( gamui::Gamui* gamui, const gamui::ButtonLook& look );
	void Place( BattleScene*,
				const Unit* shooter,
				const Unit* target, 
				const grinliz::Vector2I& pos );
	void Hide();
	bool ConvertTap( const gamui::UIItem* item, int* mode );

private:
	enum {  MAX_FIRE_BUTTONS	= 5,
			FIRE_BUTTON_SPACING = 5 };

	gamui::PushButton	fireButton[MAX_FIRE_BUTTONS];
};

#endif // UFOATTACK_FIREWIDGET_INCLUDED