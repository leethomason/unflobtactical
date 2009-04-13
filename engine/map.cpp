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

#include "platformgl.h"
#include "map.h"
#include "model.h"
#include "loosequadtree.h"
#include "renderqueue.h"
#include "surface.h"
#include "text.h"

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

	texture1[0].Set( 0.0f, 1.0f );
	texture1[1].Set( 0.0f, 0.0f );
	texture1[2].Set( 1.0f, 0.0f );
	texture1[3].Set( 1.0f, 1.0f );

	lightMap = 0;

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
	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			Tile* tile = &tileArr[j*SIZE+i];
			int count = tile->CountItems();
			for( int k=0; k<count; ++k ) {
				GLASSERT( tile->CountItems() > 0 );
				DeleteAt( i, j );
			}
			GLASSERT( tile->CountItems() == 0 );
		}
	}
	memset( tileArr, 0, sizeof(Tile)*SIZE*SIZE );
}


void Map::Draw()
{
	U8* v = (U8*)vertex + Vertex::POS_OFFSET;
	U8* n = (U8*)vertex + Vertex::NORMAL_OFFSET;
	U8* t = (U8*)vertex + Vertex::TEXTURE_OFFSET;

	/* Texture 0 */
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

	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

	glDisable( GL_TEXTURE_2D );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glClientActiveTexture( GL_TEXTURE0 );
	glActiveTexture( GL_TEXTURE0 );

	CHECK_GL_ERROR
}

void Map::BindTextureUnits()
{
	/* Texture 0 */
	glBindTexture( GL_TEXTURE_2D, texture->glID );

	/* Texture 1 */
	glActiveTexture( GL_TEXTURE1 );
	glClientActiveTexture( GL_TEXTURE1 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, finalMapTex.glID );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 0, texture1 );

	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	
	CHECK_GL_ERROR
}


void Map::UnBindTextureUnits()
{
	glActiveTexture( GL_TEXTURE1 );
	glClientActiveTexture( GL_TEXTURE1 );
	glDisable( GL_TEXTURE_2D );

	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glClientActiveTexture( GL_TEXTURE0 );
	glActiveTexture( GL_TEXTURE0 );
}



void Map::SetLightMap( Surface* surface )
{
	if ( surface ) {
		GLASSERT( surface->Width() == Map::SIZE );
		GLASSERT( surface->Height() == Map::SIZE );
	}
	lightMap = surface;
}

	
void Map::GenerateLightMap( const grinliz::BitArray<SIZE, SIZE>& fogOfWar )
{
	BitArrayRowIterator<SIZE, SIZE> it( fogOfWar ); 
	U16* dst = (U16*) finalMap.Pixels();

	if ( lightMap ) {
		const U16* src =   (U16*)lightMap->Pixels();

		for( int j=0; j<SIZE; ++j ) {
			// Flip to account for origin in upper left.
			//U16* dst = (U16*)finalMap.Pixels() + finalMap.Height()*finalMap.Width() - (j+1)*finalMap.Width();

			for( int i=0; i<SIZE; ++i ) {
				*dst = fogOfWar.IsSet( i, SIZE-1-j ) ? *src : 0;
				++src;
				++dst;
			}
		}
	}
	else {
		for( int j=0; j<SIZE; ++j ) {
			// Flip to account for origin in upper left.
			for( int i=0; i<SIZE; ++i ) {
				*dst = fogOfWar.IsSet( i, SIZE-1-j ) ? 0xffff : 0;
				++dst;
			}
		}
	}
	finalMap.UpdateTexture( false, finalMapTex.glID );
}



Map::ItemDef* Map::InitItemDef( int i )
{
	GLASSERT( i > 0 );				// 0 is reserved
	GLASSERT( i < MAX_ITEM_DEF );
	return &itemDefArr[i];
}


const char* Map::GetItemDefName( int i )
{
	const char* result = "";
	if ( i > 0 && i < MAX_ITEM_DEF ) {
		result = itemDefArr[i].name;
	}
	return result;
}


int Map::Tile::FindFreeItem() const
{
	for( int i=0; i<ITEM_PER_TILE; ++i ) {
		if ( !item[i].InUse() )
			return i;
	}
	return -1;
}


int Map::Tile::CountItems() const
{
	int count = 0;
	for( int i=0; i<ITEM_PER_TILE; ++i ) {
		if ( item[i].InUse() )
			++count;
	}
	return count;
}


Map::Tile* Map::GetTileFromItem( const Item* item, int* _layer, int* x, int *y )
{
	int index = ((const U8*)item - (const U8*)tileArr) / sizeof( Tile );
	GLASSERT( index >= 0 && index < SIZE*SIZE );
	int layer = item - tileArr[index].item;
	GLASSERT( layer >= 0 && layer < ITEM_PER_TILE );

	if ( _layer ) {
		*_layer = layer;
	}
	if ( x && y ) {
		*y = index / SIZE;
		*x = index - (*y) * SIZE;
		GLASSERT( *x >=0 && *x < SIZE );
		GLASSERT( *y >=0 && *y < SIZE );
	}
	return tileArr + index;
}


Model* Map::CreatePreview( int x, int y, int defIndex, int rotation )
{
	Model* model = 0;
	if ( itemDefArr[defIndex].modelResource ) {
		Rectangle2I mapBounds;
		Vector2F modelPos;
		CalcModelPos( x, y, rotation, itemDefArr[defIndex], &mapBounds, &modelPos );

		model = tree->AllocModel( itemDefArr[defIndex].modelResource );
		model->SetPos( modelPos.x, 0.0f, modelPos.y );
		model->SetYRotation( 90.0f * rotation );
	}
	return model;
}


bool Map::AddToTile( int x, int y, int defIndex, int rotation )
{
	GLASSERT( x >= 0 && x < width );
	GLASSERT( y >= 0 && y < height );
	GLASSERT( defIndex > 0 && defIndex < MAX_ITEM_DEF );
	GLASSERT( rotation >=0 && rotation < 4 );
	
	if ( !itemDefArr[defIndex].modelResource ) {
		GLOUTPUT(( "No model resource.\n" ));
		return false;
	}

	Tile* tile = tileArr + ((y*SIZE)+x);

	Rectangle2I mapBounds;
	Vector2F modelPos;
	CalcModelPos( x, y, rotation, itemDefArr[defIndex], &mapBounds, &modelPos );
	
	// Check bounds on map.
	if ( mapBounds.min.x < 0 || mapBounds.min.y < 0 || mapBounds.max.x >= SIZE || mapBounds.max.y >= SIZE ) {
		GLOUTPUT(( "Out of bounds.\n" ));
		return false;
	}

	// Check for duplicate. Only check for exact dupe - same origin & rotation.
	// This isn't required, but prevents map creation mistakes.
	for( int nLayer=0; nLayer<ITEM_PER_TILE; ++nLayer )
	{
		Item* item = &tile->item[nLayer];

		if ( item->InUse() && !item->IsReference() ) {
			GLASSERT( item->model );
			if ( item->itemDefIndex == defIndex && item->rotation == rotation ) {
				GLOUTPUT(( "Duplicate layer.\n" ));
				return false;
			}
		}
	}

	// Check for open slots. This is required, since both the Item and the references
	// to it consume a map slot.
	for( int j=mapBounds.min.y; j<=mapBounds.max.y; ++j ) {
		for( int i=mapBounds.min.x; i<=mapBounds.max.x; ++i ) {
			if ( tileArr[j*SIZE+i].CountItems() == ITEM_PER_TILE ) {
				GLOUTPUT(( "Map full at %d,%d.\n", i, j ));
				return false;
			}
		}
	}

	// Finally add!!
	Item* mainItem = 0;
	for( int i=mapBounds.min.x; i<=mapBounds.max.x; ++i ) 
	{
		for( int j=mapBounds.min.y; j<=mapBounds.max.y; ++j ) 
		{
			Tile* refTile	= &tileArr[j*SIZE+i];
			GLASSERT( refTile->FindFreeItem() >= 0 );
			Item* item		= &refTile->item[ refTile->FindFreeItem() ];

			if ( i==mapBounds.min.x && j==mapBounds.min.y ) 
			{
				// Main tile (or only tile for 1x1)
				mainItem = item;
				item->itemDefIndex = defIndex;
				item->rotation = rotation;
				item->model = tree->AllocModel( itemDefArr[defIndex].modelResource );

				item->model->OwnedByMap( true );
				item->model->SetPos( modelPos.x, 0.0f, modelPos.y );
				item->model->SetYRotation( 90.0f * rotation );
			}
			else {
				item->itemDefIndex	= defIndex;
				item->rotation		= 255;
				GLASSERT( !item->model );
				GLASSERT( !item->ref );

				GLASSERT( mainItem );
				item->ref = mainItem;
			}
		}
	}
	return true;
}


void Map::DeleteAt( int x, int y )
{
	GLASSERT( x >= 0 && x < width );
	GLASSERT( y >= 0 && y < height );

	Tile* tile = tileArr + ((y*SIZE)+x);
	int layer = tile->CountItems() - 1;
	if ( layer < 0 ) {
		return;
	}

	// If reference, change everything to the main object.
	if ( tile->item[layer].IsReference() ) {
		tile = GetTileFromItem( tile->item[layer].ref, &layer, &x, &y );
	}

	GLASSERT( layer >= 0 && layer < ITEM_PER_TILE );
	if ( layer < 0 || layer >= ITEM_PER_TILE ) {
		return;
	}

	const Item* mainItem = &tile->item[layer];

	Rectangle2I bounds;
	Vector2F p;
	CalcModelPos( x, y, mainItem->rotation, itemDefArr[mainItem->itemDefIndex], &bounds, &p );
	
	// Free the main item.
	GLASSERT( tile->item[layer].model );
	tree->FreeModel( tile->item[layer].model );

	tile->item[layer].itemDefIndex = 0;
	tile->item[layer].rotation = 0;
	tile->item[layer].model = 0;

	// free the references
	for( int j=bounds.min.y; j<=bounds.max.y; ++j ) {
		for( int i=bounds.min.x; i<=bounds.max.x; ++i ) {
			if ( j == bounds.min.y && i == bounds.min.x )
				continue;

			int k=0;
			for( k=0; k<ITEM_PER_TILE; ++k ) {
				if (    tileArr[j*SIZE+i].item[k].IsReference()
					 && tileArr[j*SIZE+i].item[k].ref == mainItem ) 
				{
					// Found it!
					tileArr[j*SIZE+i].item[k].itemDefIndex = 0;
					tileArr[j*SIZE+i].item[k].rotation = 0;
					tileArr[j*SIZE+i].item[k].ref = 0;
					break;
				}
			}
			GLASSERT( k != ITEM_PER_TILE );
		}
	}
}


void Map::CalcModelPos(	int x, int y, int r, const ItemDef& itemDef, 
						grinliz::Rectangle2I* mapBounds,
						grinliz::Vector2F* origin )
{
	int cx = itemDef.cx;
	int cy = itemDef.cy;
	float halfCX = (float)cx * 0.5f;
	float halfCY = (float)cy * 0.5f;

	float xf = (float)x;
	float yf = (float)y;
	float cxf = (float)cx;
	float cyf = (float)cy;

	ModelResource* resource = itemDef.modelResource;
	// rotates around the upper left, irrespective
	// of the actual model origin.
	//
	//		 0	 0	
	//		0ab	0bd
	//		 cd	 ac
	//

	if ( r & 1 ) {
		mapBounds->Set( x, y, x+cy-1, y+cx-1 );
	}
	else {
		mapBounds->Set( x, y, x+cx-1, y+cy-1 );
	}

	if ( resource->IsOriginUpperLeft() ) {
		switch( r ) {
			case 0:		origin->Set( xf,			yf );			break;
			case 1:		origin->Set( xf,			yf+cxf );		break;
			case 2:		origin->Set( xf+cxf,		yf+cyf );		break;
			case 3:		origin->Set( xf+cyf,		yf );			break;
			default:	GLASSERT( 0 );								break;
		}
	}
	else {
		switch( r ) {
			case 0:		
			case 2:
				origin->Set( xf + halfCX, yf + halfCY );	
				break;

			case 1:		
			case 3:
				origin->Set( xf + halfCY, yf + halfCX );	
				break;

			default:
				GLASSERT( 0 );
				break;
		}
	}
}

/*
void Map::SetModel( Model* model, int x, int y, const Item& item )
{
	Rectangle2I mapBounds;
	Vector2F	origin;

	CalcModelPos( x, y, item.rotation, itemDefArr[item.itemDefIndex], &mapBounds, &origin );

	model->SetPos( origin.x, 0.0f, origin.y );
	model->SetYRotation( 90.0f * (float)item.rotation );

	for( int j=mapBounds.min.y; j<=mapBounds.max.y; ++j ) {
		for( int i=mapBounds.min.x; i<=mapBounds.max.x; ++i ) {
			if ( i != mapBounds.min.x && j != mapBounds.min.y ) 
			{
				Tile* tile = &tileArr[SIZE*j+i];
				GLASSERT( tile->CountItems() < ITEM_PER_TILE );

				int count = tile->FindFreeItem();
				GLASSERT( count >= 0 );
				if ( count >= 0 ) {
					tile->item[count].itemDefIndex = item.itemDefIndex;
					tile->item[count].rotation = 255;
					tile->item[count].ref = &item;
				}
			}
		}
	}
}
*/


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
			
			U8 count = 0;
			for( int k=0; k<ITEM_PER_TILE; ++k ) {
				if ( tile->item[k].InUse() && !tile->item[k].IsReference() ) {
					++count;
				}
			}

			fwrite( &count, 1, 1, fp );
			
			for( int k=0; k<ITEM_PER_TILE; ++k ) {
				if ( tile->item[k].InUse() && !tile->item[k].IsReference() ) {
					Item* item = &tile->item[k];

					fwrite( &item->itemDefIndex, 1, 1, fp );
					fwrite( &item->rotation, 1, 1, fp );
				}
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
				U8 defIndex = 0;
				U8 rotation = 0;
				fread( &defIndex, 1, 1, fp );
				fread( &rotation, 1, 1, fp );
				if ( defIndex > 0 ) {
					bool result = AddToTile( i, j, defIndex, rotation ); 
					GLASSERT( result );
				}
			}
		}
	}
	GLOUTPUT(( "Map loaded.\n" ));
}


void Map::DumpTile( int x, int y )
{
	if ( InRange( x, 0, SIZE-1 ) && InRange( y, 0, SIZE-1 )) 
	{
		const Tile& tile = tileArr[y*SIZE+x];

		for( int i=0; i<ITEM_PER_TILE; ++i ) {
			int index = tile.item[i].itemDefIndex;
			const ItemDef& itemDef = itemDefArr[index];

			if ( tile.item[i].IsReference() ) {
				int rx, ry, layer;
				Tile* t = GetTileFromItem( tile.item[i].ref, &layer, &rx, &ry );

				UFOText::Draw( 0, 100-12*i, "->(%d,%d) %s r=%d", 
							   rx-x, ry-y, itemDef.name, t->item[layer].rotation );
			}
			else {
				if ( itemDef.name[0] ) {
					int r = tile.item[i].rotation;
					UFOText::Draw( 0, 100-12*i, "%s r=%d", itemDef.name, r*90 );
				}
				else {
					UFOText::Draw( 0, 100-12*i, "%s", "--" );
				}
			}
		}
	}
}
