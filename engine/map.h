#ifndef UFOATTACK_MAP_INCLUDED
#define UFOATTACK_MAP_INCLUDED

#include <stdio.h>
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glbitarray.h"
#include "vertex.h"
#include "surface.h"

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

	int Height() { return height; }
	int Width()  { return width; }

	void SetSize( int w, int h )					{ width = w; height = h; }

	void SetTexture( const Texture* texture )		{ this->texture = texture; }
	void BindTextureUnits();
	void UnBindTextureUnits();

	void GenerateLightMap( const grinliz::BitArray<SIZE, SIZE>& fogOfWar );
	
	void Draw( RenderQueue* queue );

	void SetItemDef( int i, ModelResource* mr );
	const char* GetItemDefName( int i );

	void AddToTile( int x, int y, int itemDefIndex, int rotation );
	void DeleteAt( int x, int y );

	void Save( FILE* fp );
	void Load( FILE* fp );
	void Clear();

private:
	int width, height;

	const Texture* texture;
	SpaceTree* tree;

	Vertex vertex[4];
	grinliz::Vector2F texture1[4];

	Texture finalMapTex;
	Surface finalMap;
	Surface lightMap;

	ItemDef itemDefArr[MAX_ITEM_DEF];
	Tile    tileArr[ SIZE*SIZE ];
};

#endif // UFOATTACK_MAP_INCLUDED
