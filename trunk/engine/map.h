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

#ifndef UFOATTACK_MAP_INCLUDED
#define UFOATTACK_MAP_INCLUDED

#include <stdio.h>
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glbitarray.h"
#include "vertex.h"
#include "surface.h"
#include "enginelimits.h"

class Model;
class ModelResource;
class SpaceTree;
class RenderQueue;
class Texture;

class Map
{
public:
	enum {
		SIZE = 64,					// maximum size
		MAX_ITEM_DEF = 256,
		ITEM_PER_TILE = 4,
	};

	struct ItemDef 
	{
		enum { MAX_CX = 6, MAX_CY = 6 };

		void Init() {	name[0] = 0;
						
						cx = 1; 
						cy = 1; 
						hp = 0; 
						flammable = 0; 
						explosive = 0;

						modelResource			= 0;
						modelResourceOpen		= 0;
						modelResourceDestroyed	= 0;
					}

		U16		cx, cy;
		U16		hp;					// 0 infinite, >0 can be damaged
		U16		flammable;			// 0: none, 1: wood, 2: gas
		U16		explosive;			// 0: no, >0 blast radius

		ModelResource* modelResource;
		ModelResource* modelResourceOpen;
		ModelResource* modelResourceDestroyed;

		char	name[EL_FILE_STRING_LEN];
		U8		pather[2][MAX_CX][MAX_CY];
	};

	struct Item
	{
		U8		itemDefIndex;	// 0: not in use, >0 is the index
		U8		rotation;		// 0-3: rotation, 255: reference	
		U16		_pad0;

		union {
			Model*	model;
			const Item*	ref;
		};

		bool InUse() const			{ return itemDefIndex > 0; }
		bool IsReference() const	{ return (rotation == 255); }
	};

	struct Tile
	{
		Item item[ITEM_PER_TILE];

		int CountItems() const;
		int FindFreeItem() const;
	};


	Map( SpaceTree* tree );
	~Map();

	int Height() { return height; }
	int Width()  { return width; }

	void SetSize( int w, int h )					{ width = w; height = h; }

	void SetTexture( const Texture* texture )		{ this->texture = texture; }
	void BindTextureUnits();
	void UnBindTextureUnits();

	void		SetLightMap( Surface* surface );
	Surface*	GetLightMap()	{ return lightMap; }
	void		GenerateLightMap( const grinliz::BitArray<SIZE, SIZE>& fogOfWar );
	
	void Draw();
	void DrawPath();

	Map::ItemDef* InitItemDef( int i );
	const char* GetItemDefName( int i );

#ifdef MAPMAKER
	Model* CreatePreview( int x, int z, int itemDefIndex, int rotation );
#endif
	bool AddToTile( int x, int z, int itemDefIndex, int rotation );
	void DeleteAt( int x, int z );
	int GetPathMask( int x, int z ) const;

	void Save( FILE* fp );
	void Load( FILE* fp );
	void Clear();

	void DumpTile( int x, int z );

private:
	struct IMat
	{
		int a, b, c, d, x, z;

		void Init( int w, int h, int r );
		void Mult( const grinliz::Vector2I& in, grinliz::Vector2I* out );
	};

	void CalcModelPos(	int x, int y, int r, const ItemDef& itemDef, 
						grinliz::Rectangle2I* mapBounds,
						grinliz::Vector2F* origin );

	// Performs no translation of references.
	// item: input
	// output layer (to get item from returned tile)
	// x and y: map locations
	Tile* GetTileFromItem( const Item* item, int* layer, int* x, int *y ) const;

	// resolve a reference:
	// outItem: item referred to
	// outTile: tile referred to
	// dx, dy: position of inItem relative to outItem. (Will be >= 0 )
	void ResolveReference( const Item* inTtem, Item** outItem, Tile** outTile, int *dx, int* dy ) const;
	
	int width, height;
	const Texture* texture;
	SpaceTree* tree;

	Vertex vertex[4];
	grinliz::Vector2F texture1[4];

	Texture finalMapTex;
	Surface finalMap;
	Surface* lightMap;

	ItemDef itemDefArr[MAX_ITEM_DEF];
	Tile    tileArr[ SIZE*SIZE ];
};

#endif // UFOATTACK_MAP_INCLUDED
