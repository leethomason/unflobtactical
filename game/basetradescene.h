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
		clip3D->Zero(); 
		clip2D->Zero(); 
		return RENDER_2D;
	}

	virtual void Resize();

protected:
	bool ComputePrice( int* total );
	void SceneDone();

	//BackgroundUI		backgroundUI;
	gamui::Image		background;
	gamui::PushButton	okay;
	gamui::ToggleButton	sellAll;
	BaseTradeSceneData* data;
	int					minSoldiers;

	StorageWidget		*baseWidget, *regionWidget;
	Storage				*originalBase, *originalRegion;

	gamui::TextLabel	baseLabel, regionLabel;
	gamui::TextLabel	profitLabel, lossLabel, totalLabel, remainLabel;
	gamui::PushButton	helpButton;
};



#endif // BASETRADE_INCLUDED