#ifndef UFOATTACK_MAP_INCLUDED
#define UFOATTACK_MAP_INCLUDED

#include <stdio.h>
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

class Model;
class ModelResource;
class SpaceTree;
class RenderQueue;

class Map
{
public:
	enum {
		SIZE = 64,					// maximum size
		MAX_ITEM_DEF = 256,
		LAYER_PER_TILE = 4,
	};

	struct ItemDef {
		ModelResource* modelResource;
	};

	struct Layer
	{
		U8 defIndex;
		U8 rotation;
		Model* model;
	};

	struct Tile
	{
		Layer layer[LAYER_PER_TILE];

		int CountLayers();
	};


	Map( SpaceTree* tree );
	~Map();

	void SetModel( Model* m );
	void Draw( RenderQueue* queue );

	int Width()		{ return width; }	// the size in use
	int Height()	{ return height; }	

	void SetItemDef( int i, ModelResource* mr );
	const char* GetItemDefName( int i );

	void AddToTile( int x, int y, int itemDefIndex, int rotation );
	void DeleteAt( int x, int y );

	void Save( FILE* fp );
	void Load( FILE* fp );
	void Clear();

private:
	Model* model;
	int width, height;
	SpaceTree* tree;

	ItemDef itemDefArr[MAX_ITEM_DEF];
	Tile    tileArr[ SIZE*SIZE ];
};

#endif // UFOATTACK_MAP_INCLUDED
