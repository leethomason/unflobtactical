#ifndef BASETRADE_INCLUDED
#define BASETRADE_INCLUDED

#include "scene.h"
#include "item.h"

class Storage;
class StorageWidget;



class BaseTradeSceneData : public SceneData
{
public:
	BaseTradeSceneData( const ItemDefArr& arr ) : region( 0, 0, arr ) {}

	const char* baseName;
	const char* regionName;
	Storage* base;
	int* cash;
	float costMult;			// multiplier applies to purchase price	

	bool soldierBoost;
	Unit* soldiers;			// can hire more soldiers!
	int *scientists;

	Storage region;
};


class BaseTradeScene : public Scene
{
public:
	BaseTradeScene( Game* _game, BaseTradeSceneData* data );
	virtual ~BaseTradeScene();

	virtual void Activate();

	// UI
	virtual void Tap(	int action, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );

	// Rendering
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )	
	{ 
		clip3D->SetInvalid(); 
		clip2D->SetInvalid(); 
		return RENDER_2D;
	}

protected:
	bool ComputePrice( int* total );
	void SetHireButtons();

	BackgroundUI		backgroundUI;
	gamui::PushButton	okay;
	gamui::PushButton	hireSoldier, hireScientist;
	BaseTradeSceneData* data;

	StorageWidget		*baseWidget, *regionWidget;
	Storage				*originalBase, *originalRegion;

	gamui::TextLabel	baseLabel, regionLabel;
	gamui::TextLabel	profitLabel, lossLabel, totalLabel, remainLabel;
};



#endif // BASETRADE_INCLUDED