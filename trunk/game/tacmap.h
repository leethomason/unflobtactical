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

#ifndef TACMAP_INCLUDED
#define TACMAP_INCLUDED

#include "../engine/map.h"


class Storage;
class ItemDefArr;
class ItemDef;


class TacMap : public Map
{
public:
	enum {
		NUM_ITEM_DEF = 84,
	};


	TacMap( SpaceTree* tree, const ItemDefArr& itemDefArr );
	virtual ~TacMap();

	Storage* LockStorage( int x, int y );	// always returns something.
	void ReleaseStorage( Storage* storage );				// updates the image

	const Storage* GetStorage( int x, int y ) const;		//< take a peek
	grinliz::Vector2I FindStorage( const ItemDef* itemDef, const grinliz::Vector2I& source );
	Storage* CollectAllStorage();

	// Gets a starting location for a unit on the map.
	// TERRAN_TEAM - from the lander
	// ALIEN_TEAM, guard or scout - on the map metadata
	//
	void PopLocation( int team, bool guard, grinliz::Vector2I* pos, float* rotation );
	
	virtual int GetNumItemDef()	{ return NUM_ITEM_DEF; }
	virtual const char* GetItemDefName( int i );
	virtual const MapItemDef* GetItemDef( const char* name );
	static const MapItemDef* StaticGetItemDef( const char* name );		// slow...but doesn't need an instance.

	const Model* GetLanderModel();

protected:
	virtual void SubSave( FILE* fp, int depth );
	virtual void SubLoad( const TiXmlElement* mapNode );

private:
	const MapItem* FindLander();

	struct Debris {
		Storage* storage;
		Model* crate;
	};
	void SaveDebris( const Debris& d, FILE* fp, int depth );
	void LoadDebris( const TiXmlElement* mapNode );

	CDynArray< Debris > debris;
	const ItemDefArr& gameItemDefArr;

	const MapItem* lander;
	int nLanderPos;

	static const MapItemDef itemDefArr[NUM_ITEM_DEF];
	CStringMap< const MapItemDef* >	itemDefMap;
};


#endif // TACMAP_INCLUDED