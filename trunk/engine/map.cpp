#include "platformgl.h"
#include "map.h"
#include "model.h"
#include "loosequadtree.h"
#include "renderqueue.h"
#include "surface.h"

using namespace grinliz;


Map::Map( SpaceTree* tree )
{
	memset( itemDefArr, 0, sizeof(ItemDef)*MAX_ITEM_DEF );
	memset( tileArr, 0, sizeof(Tile)*SIZE*SIZE );
	this->tree = tree;
	width = height = SIZE;
	texture = 0;

	vertex[0].pos.Set( 0.0f,		0.0f, 0.0f );
	vertex[1].pos.Set( 0.0f,		0.0f, (float)SIZE );
	vertex[2].pos.Set( (float)SIZE, 0.0f, (float)SIZE );
	vertex[3].pos.Set( (float)SIZE, 0.0f, 0.0f );

	vertex[0].tex.Set( 0.0f, 1.0f );
	vertex[1].tex.Set( 0.0f, 0.0f );
	vertex[2].tex.Set( 1.0f, 0.0f );
	vertex[3].tex.Set( 1.0f, 1.0f );

	vertex[0].normal.Set( 0.0f, 1.0f, 0.0f );
	vertex[1].normal.Set( 0.0f, 1.0f, 0.0f );
	vertex[2].normal.Set( 0.0f, 1.0f, 0.0f );
	vertex[3].normal.Set( 0.0f, 1.0f, 0.0f );

	texture1[0].Set( 0.0f, 0.0f );
	texture1[1].Set( 0.0f, 1.0f );
	texture1[2].Set( 1.0f, 1.0f );
	texture1[3].Set( 1.0f, 0.0f );

	lightMap.Set( SIZE, SIZE, 2 );
	memset( lightMap.Pixels(), 255, SIZE*SIZE*2 );

	finalMap.Set( SIZE, SIZE, 2 );
	memset( finalMap.Pixels(), 255, SIZE*SIZE*2 );
	U32 id = finalMap.CreateTexture( false );
	finalMapTex.Set( "lightmap", id, false );
}


Map::~Map()
{
	Clear();
}


void Map::Clear()
{
	for( int i=0; i<SIZE*SIZE; ++i ) {
		Tile* tile = &tileArr[i];

		for( int k=0; k<LAYER_PER_TILE; ++k ) {
			if ( tile->layer[k].model ) {
				tree->FreeModel( tile->layer[k].model );
			}
		}
		memset( tile, 0, sizeof( Tile ) );
	}
}


void Map::Draw( RenderQueue* queue )
{
	//model->Queue( queue );
	//queue->Flush();

	/* Texture 0 */
	U8* v = (U8*)vertex + Vertex::POS_OFFSET;
	U8* n = (U8*)vertex + Vertex::NORMAL_OFFSET;
	U8* t = (U8*)vertex + Vertex::TEXTURE_OFFSET;

	glBindTexture( GL_TEXTURE_2D, texture->glID );

	glVertexPointer(   3, GL_FLOAT, sizeof(Vertex), v);			// last param is offset, not ptr
	glNormalPointer(      GL_FLOAT, sizeof(Vertex), n);		
	glTexCoordPointer( 2, GL_FLOAT, sizeof(Vertex), t);  

	/* Texture 1 */
	glActiveTexture( GL_TEXTURE1 );
	glClientActiveTexture( GL_TEXTURE1 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, finalMapTex.glID );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 0, texture1 );

	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	CHECK_GL_ERROR

	glDrawArrays( GL_QUADS, 0, 4 );

	glDisable( GL_TEXTURE_2D );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glClientActiveTexture( GL_TEXTURE0 );
	glActiveTexture( GL_TEXTURE0 );

	CHECK_GL_ERROR
}


void Map::GenerateLightMap( const grinliz::BitArray<SIZE, SIZE>& fogOfWar )
{
	BitArrayRowIterator<SIZE, SIZE> it( fogOfWar ); 
	const U16* src = (U16*) lightMap.Pixels();
	U16* dst = (U16*) finalMap.Pixels();

	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			*dst = fogOfWar.IsSet( i, j ) ? *src : 0;
			++src;
			++dst;
		}
	}
	finalMap.UpdateTexture( false, finalMapTex.glID );
}



void Map::SetItemDef( int i, ModelResource* mr )
{
	GLASSERT( i > 0 );	// 0 is reserved
	GLASSERT( i < MAX_ITEM_DEF );

	itemDefArr[i].modelResource = mr;
}


const char* Map::GetItemDefName( int i )
{
	const char* result = "";
	if ( i > 0 && i < MAX_ITEM_DEF ) {
		if ( itemDefArr[i].modelResource ) {
			result = itemDefArr[i].modelResource->name;
		}
	}
	return result;
}


int Map::Tile::CountLayers()
{
	int count = 0;
	for( int i=0; i<LAYER_PER_TILE; ++i ) {
		if ( layer[i].defIndex )
			++count;
	}
	return count;
}


void Map::AddToTile( int x, int y, int defIndex, int rotation )
{
	GLASSERT( x >= 0 && x < width );
	GLASSERT( y >= 0 && y < height );
	GLASSERT( defIndex > 0 && defIndex < MAX_ITEM_DEF );
	GLASSERT( rotation >=0 && rotation < 4 );
	
	if ( !itemDefArr[defIndex].modelResource ) {
		GLOUTPUT(( "No model resource.\n" ));
		return;
	}

	Tile* tile = tileArr + ((y*SIZE)+x);

	// Look for an open slot, don't allow duplicates.
	int nLayer = 0;
	for( nLayer=0; nLayer<LAYER_PER_TILE; ++nLayer )
	{
		Layer* layer = &tile->layer[nLayer];
		if ( layer->defIndex ) {
			GLASSERT( layer->model );
			if ( layer->defIndex == defIndex && layer->rotation == rotation ) {
				GLOUTPUT(( "Duplicate layer.\n" ));
				return;
			}
		}
		else {
			break;
		}
	}

	if ( nLayer < LAYER_PER_TILE ) 
	{
		Layer* layer = &tile->layer[nLayer];
		layer->defIndex = defIndex;
		layer->rotation = rotation;
		
		layer->model = tree->AllocModel( itemDefArr[defIndex].modelResource );

		layer->model->SetPos( (float)x+0.5f, 0.0f, (float)y+0.5f );
		layer->model->SetYRotation( Fixed(90) * rotation );
	}
	else {
		GLOUTPUT(( "layers full.\n" ));
	}
}


void Map::DeleteAt( int x, int y )
{
	GLASSERT( x >= 0 && x < width );
	GLASSERT( y >= 0 && y < height );
	Tile* tile = tileArr + ((y*SIZE)+x);

	for( int i=LAYER_PER_TILE-1; i>=0; --i ) {
		Layer* layer = &tile->layer[i];
		if ( layer->defIndex ) {
			tree->FreeModel( layer->model );
			layer->model = 0;
			layer->defIndex = 0;
			break;
		}
	}
}


void Map::Save( FILE* fp )
{
	// Version (so I can load old files.)
	U8 version = 1;
	fwrite( &version, 1, 1, fp );

	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			// Write every tile. The minimal information is a count of layers.
			// The layer data is only written if needed.

			Tile* tile = tileArr + ((j*SIZE)+i);
			
			U8 count = (U8) tile->CountLayers();
			fwrite( &count, 1, 1, fp );
			
			for( int k=0; k<count; ++k ) 
			{
				Layer* layer = &tile->layer[k];
				fwrite( &layer->defIndex, 1, 1, fp );
				fwrite( &layer->rotation, 1, 1, fp );
			}
		}
	}
	GLOUTPUT(( "Map saved.\n" ));
}


void Map::Load( FILE* fp )
{
	U8 version = 0;
	fread( &version, 1, 1, fp );

	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			Tile* tile = tileArr + ((j*SIZE)+i);
			
			U8 count;
			fread( &count, 1, 1, fp );

			for( int k=0; k<count; ++k ) 
			{
				Layer* layer = &tile->layer[k];
				fread( &layer->defIndex, 1, 1, fp );
				fread( &layer->rotation, 1, 1, fp );

				layer->model = tree->AllocModel( itemDefArr[layer->defIndex].modelResource );
				layer->model->SetPos( (float)i+0.5f, 0.0f, (float)j+0.5f );
				layer->model->SetYRotation( Fixed(90) * Fixed(layer->rotation) );
			}
		}
	}
	GLOUTPUT(( "Map loaded.\n" ));
}

