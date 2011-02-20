#ifndef BASETRADE_INCLUDED
#define BASETRADE_INCLUDED

#include "scene.h"

class Storage;
class StorageWidget;


class BaseTradeSceneData : public SceneData
{
public:
	BaseTradeSceneData( Storage* _base, Storage* _region, int *_cash ) : base( _base ), region( _region ), cash( _cash )
	{
	}
	Storage* base;
	Storage* region;
	int* cash;
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

	BackgroundUI		backgroundUI;
	gamui::PushButton	okay, cancel;
	BaseTradeSceneData* data;

	StorageWidget		*baseWidget, *regionWidget;
	Storage				*originalBase, *originalRegion;

	gamui::TextLabel	baseLabel, regionLabel;
	gamui::TextLabel	profitLabel, lossLabel, totalLabel, remainLabel;
};



#endif // BASETRADE_INCLUDED