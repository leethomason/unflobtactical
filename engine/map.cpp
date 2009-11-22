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
#include "../game/material.h"		// bad call to less general directory. FIXME. Move map to game?
#include "../game/unit.h"			// bad call to less general directory. FIXME. Move map to game?
#include "../game/game.h"			// bad call to less general directory. FIXME. Move map to game?
#include "../sqlite3/sqlite3.h"
#include "../shared/gldatabase.h"

using namespace grinliz;
using namespace micropather;

Map::Map( SpaceTree* tree )
	: itemPool( "mapItemPool", sizeof( MapItem ), sizeof( MapItem ) * 200, false )
{
	memset( visMap, 0, SIZE*SIZE );
	memset( pathMap, 0, SIZE*SIZE );
	memset( itemDefArr, 0, sizeof(MapItemDef)*MAX_ITEM_DEF );
	mapDB = 0;

	microPather = new MicroPather(	this,			// graph interface
									SIZE*SIZE,		// max possible states (+1)
									6 );			// max adjacent states

	this->tree = tree;
	width = height = SIZE;
	texture = 0;
	nWalkingVertex = 0;
	invalidLightMap.Set( 0, 0, SIZE, SIZE );

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

	pathQueryID = 1;
	visibilityQueryID = 1;

	finalMap.Set( Surface::RGB16, SIZE, SIZE );
	baseMap.Set( Surface::RGB16, SIZE, SIZE );
	//transMap.Set( Surface::ALPHA, SIZE, SIZE );
	fogOfWar.SetAll();

	memset( finalMap.Pixels(), 255, SIZE*SIZE*2 );
	memset( baseMap.Pixels(), 255, SIZE*SIZE*2 );
	//memset( transMap.Pixels(), 255, SIZE*SIZE );

	U32 id = finalMap.CreateTexture();
	finalMapTex.Set( "lightmap", id, false );
}


Map::~Map()
{
	Clear();
	delete microPather;
}


void Map::Clear()
{
	// Find everything:
	Rectangle2I b;
	b.Set( 0, 0, SIZE-1, SIZE-1 );
	MapItem* pItem = quadTree.FindItems( b );

	while( pItem ) {
		MapItem* temp = pItem;
		pItem = pItem->next;
		if ( temp->model )
			tree->FreeModel( temp->model );
		itemPool.Free( temp );
	}
	quadTree.Clear();

	memset( visMap, 0, SIZE*SIZE );
	memset( pathMap, 0, SIZE*SIZE );
}


void Map::Draw()
{
	GenerateLightMap();

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


void Map::DrawOverlay()
{
	if ( nWalkingVertex ) {
		const Texture* iTex = TextureManager::Instance()->GetTexture( "icons" );
		GLASSERT( iTex );

		U8* v = (U8*)walkingVertex + Vertex::POS_OFFSET;
		U8* t = (U8*)walkingVertex + Vertex::TEXTURE_OFFSET;

		glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
		glBindTexture( GL_TEXTURE_2D, iTex->glID );
		glVertexPointer(   3, GL_FLOAT, sizeof(Vertex), v);
		glTexCoordPointer( 2, GL_FLOAT, sizeof(Vertex), t); 
		glDisableClientState( GL_NORMAL_ARRAY );
		glEnable( GL_BLEND );

		glDrawArrays( GL_TRIANGLES, 0, nWalkingVertex );

		glEnableClientState( GL_NORMAL_ARRAY );
		glDisable( GL_BLEND );
	}

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


void Map::SetLightMap( const Surface* surface )
{
	if ( surface ) {
		GLASSERT( surface->BytesPerPixel() == 2 );
		GLASSERT( surface->Width() == SIZE );
		GLASSERT( surface->Height() == SIZE	 );
		memcpy( baseMap.Pixels(), surface->Pixels(), SIZE*SIZE*2 );
	}
	else {
		memset( baseMap.Pixels(), 255, SIZE*SIZE*2 );
	}
	invalidLightMap.Set( 0, 0, SIZE, SIZE );
}


grinliz::BitArray<Map::SIZE, Map::SIZE, 1>* Map::LockFogOfWar()
{
	invalidLightMap.Set( 0, 0, SIZE, SIZE );
	return &fogOfWar;
}


void Map::ReleaseFogOfWar()
{
	invalidLightMap.Set( 0, 0, SIZE, SIZE );
}


void Map::GenerateLightMap()
{
	if (    invalidLightMap.max.x > invalidLightMap.min.x 
		 && invalidLightMap.max.y > invalidLightMap.min.y ) 
	{
		// Input:
		//		Basemap - input light colors.
		//		Lights: add color to basemap
		//		FogOfWar: flip on or off
		// Output:
		//		finalMap: final computed light map (for rendering)

		const U16* src = (const U16*)baseMap.Pixels();
		U16* dst = (U16*)finalMap.Pixels();
		//Surface::RGBA rgba;

		for( int j=invalidLightMap.min.y; j<invalidLightMap.max.y; ++j ) {
			for( int i=invalidLightMap.min.x; i<invalidLightMap.max.x; ++i ) {

				if ( fogOfWar.IsSet( i, SIZE-1-j ) ) {
					//Surface::RGB16( *(src+j*SIZE+i), &rgba );
					*(dst+j*SIZE+i) = *(src+j*SIZE+i);
				}
				else {
					*(dst+j*SIZE+i) = 0;
				}
			}
		}
		finalMap.UpdateTexture( finalMapTex.glID );
		invalidLightMap.Set( 0, 0, 0, 0 );
	}
}


/*
void Map::GenerateLightMap( const grinliz::BitArray<SIZE, SIZE, 1>& fogOfWar )
{
	BitArrayRowIterator<SIZE, SIZE, 1> it( fogOfWar ); 
	U16* dst = (U16*) finalMap.Pixels();

	const U16* src =   (U16*)lightMap.Pixels();
	GLASSERT( lightMap.BytesPerPixel() == 2 );

	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
#ifdef MAPMAKER
			*dst = *src;
#else
			*dst = fogOfWar.IsSet( i, SIZE-1-j ) ? *src : 0;
#endif
			++src;
			++dst;
		}
	}
	finalMap.UpdateTexture( finalMapTex.glID );
}
*/


Map::MapItemDef* Map::InitItemDef( int i )
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


void Map::IMat::Init( int w, int h, int r )
{
	x = 0;
	z = 0;

	// Matrix to map from map coordinates
	// back to object coordinates.
	switch ( r ) {
		case 0:
			a = 1;	b = 0;	x = 0;
			c = 0;	d = 1;	z = 0;
			break;
		
		case 1:
			a = 0;	b = -1;	x = w-1;
			c = 1;	d = 0;	z = 0;
			break;

		case 2:
			a = -1;	b = 0;	x = w-1;
			c = 0;	d = -1;	z = h-1;
			break;

		case 3:
			a = 0;	b = 1;	x = 0;
			c = -1;	d = 0;	z = h-1;
			break;

		default:
			break;
	};
}


void Map::IMat::Mult( const grinliz::Vector2I& in, grinliz::Vector2I* out  )
{
	out->x = a*in.x + b*in.y + x;
	out->y = c*in.x + d*in.y + z;
}


void Map::DoDamage( int baseDamage, Model* m, int shellFlags )
{
	if ( m->IsFlagSet( Model::MODEL_OWNED_BY_MAP ) ) 
	{
		MapItem* item = quadTree.FindItem( m );
		const MapItemDef& itemDef = itemDefArr[item->itemDefIndex];
		int hp = MaterialDef::CalcDamage( baseDamage, shellFlags, itemDef.materialFlags );

		if ( itemDef.CanDamage() && item->DoDamage(hp) ) {
			// Destroy the current model. Replace it with "destroyed"
			// model if there is one.
			Vector3F pos = item->model->Pos();
			float rot = item->model->GetYRotation();
			tree->FreeModel( item->model );
			item->model = 0;

			if ( itemDef.modelResourceDestroyed ) {
				item->model = tree->AllocModel( itemDef.modelResourceDestroyed );
				item->model->SetFlag( Model::MODEL_OWNED_BY_MAP );
				item->model->SetPos( pos );
				item->model->SetYRotation( rot );
			}
			ResetPath();
		}
	}
}


#ifdef MAPMAKER
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
#endif


bool Map::AddItem( int x, int y, int rotation, int defIndex, int hp, int flags, Storage* storage )
{
	GLASSERT( x >= 0 && x < width );
	GLASSERT( y >= 0 && y < height );
	GLASSERT( defIndex > 0 && defIndex < MAX_ITEM_DEF );
	GLASSERT( rotation >=0 && rotation < 4 );
	
	if ( !itemDefArr[defIndex].modelResource ) {
		GLOUTPUT(( "No model resource.\n" ));
		GLASSERT( 0 );
		return false;
	}

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
	MapItem* root = quadTree.FindItems( mapBounds );
	while( root ) {
		if ( root->x == x && root->y == y && root->rot == rotation && root->itemDefIndex == defIndex ) {
			GLOUTPUT(( "Duplicate layer.\n" ));
			return false;
		}
		root = root->next;
	}

	// Finally add!!
	MapItem* item = (MapItem*) itemPool.Alloc();
	item->model = 0;
	if ( hp == -1 )
		hp = itemDefArr[defIndex].hp;

	const ModelResource* res = 0;
	if ( itemDefArr[defIndex].CanDamage() && hp == 0 ) {
		res = itemDefArr[defIndex].modelResourceDestroyed;
	}
	else {
		res = itemDefArr[defIndex].modelResource;
	}
	if ( res ) {
		Model* model = tree->AllocModel( res );
		model->SetFlag( Model::MODEL_OWNED_BY_MAP );
		model->SetPos( modelPos.x, 0.0f, modelPos.y );
		model->SetYRotation( 90.0f * rotation );
		item->model = model;
	}

	item->x = x;
	item->y = y;
	item->rot = rotation;
	item->itemDefIndex = defIndex;
	item->hp = hp;
	item->flags = flags;
	GLASSERT( mapBounds.min.x >= 0 && mapBounds.max.x < 256 );	// using int8
	GLASSERT( mapBounds.min.y >= 0 && mapBounds.max.y < 256 );	// using int8
	item->mapBounds8.Set( mapBounds.min.x, mapBounds.min.y, mapBounds.max.x, mapBounds.max.y );
	item->next = 0;
	item->storage = storage;
	
	quadTree.Add( item );

	// Patch the world states:
	ResetPath();
	ClearVisPathMap( mapBounds );
	CalcVisPathMap( mapBounds );

	if ( mapDB ) {
		DeleteRow( item->x, item->y, item->rot, item->itemDefIndex );
		InsertRow( item->x, item->y, item->rot, item->itemDefIndex, item->hp, 0, 0 );
	}

	return true;
}


void Map::DeleteAt( int x, int y )
{
	GLASSERT( x >= 0 && x < width );
	GLASSERT( y >= 0 && y < height );

	MapItem* item = quadTree.FindItems( x, y );
	
	if ( item ) {
		quadTree.UnlinkItem( item );
		Rectangle2I mapBounds;
		item->MapBounds( &mapBounds );

		if ( mapDB ) {
			DeleteRow( item->x, item->y, item->rot, item->itemDefIndex );
		}

		if ( item->model )
			tree->FreeModel( item->model );
		if ( item->storage )
			delete item->storage;

		itemPool.Free( item );

		ResetPath();
		ClearVisPathMap( mapBounds );
		CalcVisPathMap( mapBounds );
	}
}


void Map::CalcModelPos(	int x, int y, int r, const MapItemDef& itemDef, 
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

	const ModelResource* resource = itemDef.modelResource;
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


void Map::DeleteRow( int x, int y, int r, int def )
{
	sqlite3_stmt* stmt = 0;
	int result=0;

	// Clear out the existing row.
	const int BUFSIZE=200;
	char buf[BUFSIZE];
#ifdef _MSC_VER
	_snprintf_s( buf, BUFSIZE, BUFSIZE, 
#else
	snprintf( buf, BUFSIZE, 
#endif
				"DELETE FROM %s WHERE (x=? AND y=? AND r=? AND defIndex=?);",
				dbTableName.c_str() );
	GLASSERT( strlen( buf ) < BUFSIZE-1 );

	result = sqlite3_prepare_v2( mapDB,	buf, -1, &stmt, 0 );
	GLASSERT( result == SQLITE_OK );

	sqlite3_bind_int( stmt, 1, x );
	sqlite3_bind_int( stmt, 2, y );
	sqlite3_bind_int( stmt, 3, r );
	sqlite3_bind_int( stmt, 4, def );

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}


void Map::InsertRow( int x, int y, int r, int def, int hp, int flags, const Storage* storage )
{
	// And add the new one.
	sqlite3_stmt* stmt = 0;
	const int BUFSIZE=200;
	char buf[BUFSIZE];
#ifdef _MSC_VER
	_snprintf_s( buf, BUFSIZE, BUFSIZE, 
#else
				snprintf( buf, BUFSIZE, 
#endif
				"INSERT INTO %s VALUES (?,?,?,?,?,?,?);",
				dbTableName.c_str() );
	GLASSERT( strlen( buf ) < BUFSIZE-1 );

	int result = sqlite3_prepare_v2(mapDB, buf, -1, &stmt, 0 );
	GLASSERT( result == SQLITE_OK );

	sqlite3_bind_int( stmt, 1, x );
	sqlite3_bind_int( stmt, 2, y );
	sqlite3_bind_int( stmt, 3, r );
	sqlite3_bind_int( stmt, 4, def );		
	sqlite3_bind_int( stmt, 5, hp );		
	sqlite3_bind_int( stmt, 6, flags );

	if ( storage ) {
		sqlite3_bind_blob( stmt, 7, storage->Rounds(), sizeof(int)*EL_MAX_ITEM_DEFS, 0 );
	}
	else {
		sqlite3_bind_blob( stmt, 7, 0, 0, 0 );
	}
	sqlite3_step(stmt);
	result = sqlite3_finalize(stmt);
	GLASSERT( result == SQLITE_OK );
}
		

void Map::SyncToDB( sqlite3* db, const char* tableName )
{
	if ( db && tableName ) {
		Clear();
		int result=0;

		dbTableName = tableName;

		// Make sure the table exists. Do a full load. From now on, all changes
		// will be written to this DB. If the table exists, this will fail.
		sqlite3_stmt* stmt = 0;
		// This doesn't work. Frustrating:
		//result = sqlite3_prepare_v2( db, "CREATE TABLE IF NOT EXISTS ? (x INT, y INT, r INT, defIndex INT, hp INT);", -1, &stmt, 0 );
		
		const int BUFSIZE=200;
		char buf[BUFSIZE];
#ifdef _MSC_VER
		_snprintf_s( buf, BUFSIZE, BUFSIZE, 
#else
					snprintf( buf, BUFSIZE, 
#endif
					"CREATE TABLE IF NOT EXISTS %s "
					 "(x INT, y INT, r INT, defIndex INT, hp INT, flags INT, storage TEXT );",
					 tableName );
		GLASSERT( strlen( buf ) < BUFSIZE-1 );

		result = sqlite3_prepare_v2( db, buf,-1, &stmt, 0 );
		GLASSERT( result == SQLITE_OK );

		sqlite3_step( stmt );
		result = sqlite3_finalize(stmt);

		// Now walk and add!
		stmt = 0;
		
#ifdef _MSC_VER
					_snprintf_s( buf, BUFSIZE, BUFSIZE, 
#else
								snprintf( buf, BUFSIZE, 
#endif
										 
					"SELECT * FROM %s;",
					 tableName );
		GLASSERT( strlen( buf ) < BUFSIZE-1 );

		result = sqlite3_prepare_v2( db, buf,-1, &stmt, 0 );
		GLASSERT( result == SQLITE_OK );

		while (sqlite3_step(stmt) == SQLITE_ROW) {
			int x	= sqlite3_column_int( stmt, 0 );
			int y	= sqlite3_column_int( stmt, 1 );
			int r	= sqlite3_column_int( stmt, 2 );
			int def = sqlite3_column_int( stmt, 3 );
			int hp	= sqlite3_column_int( stmt, 4 );
			//int flags = sqlite3_column_int( stmt, 5 );

			int nBytes = sqlite3_column_bytes( stmt, 6 );
			Storage* storage = 0;
			if ( nBytes ) {
				storage = new Storage();
				GLASSERT( nBytes == EL_MAX_ITEM_DEFS*sizeof(int) );
				storage->Init( (const int*) sqlite3_column_blob( stmt, 6 ) );
			}
			AddItem( x, y, r, def, hp, 0, storage );
		}
		sqlite3_finalize(stmt);
		mapDB = db;
	}
	else {
		mapDB = 0;
	}
}


/*static*/ sqlite3* Map::CreateMap( const std::string& savePath, sqlite3* res )
{
	sqlite3_stmt* stmt = 0;
	int result = sqlite3_prepare_v2( res, "SELECT * FROM map",-1, &stmt, 0 );
	GLASSERT( result == SQLITE_OK );
	(void)result;

	int id=0;

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		id	= sqlite3_column_int( stmt, 1 );
	}
	sqlite3_finalize(stmt);

	GLASSERT( id );
	int size=0;

	BinaryDBReader reader( res );
	reader.ReadSize( id, &size );

	char* mem = new char[size];
	reader.ReadData( id, size, mem );
	
	std::string path = savePath + "currentMap.db";
	
	FILE* fp = fopen( path.c_str(), "wb" );
	GLASSERT( fp );
	fwrite( mem, size, 1, fp );
	fclose( fp );
	delete [] mem;

	sqlite3* db = 0;
	sqlite3_open( path.c_str(), &db );
	return db;
}




void Map::SetStorage( int x, int y, Storage* storage )
{
	/*
	RemoveStorage( x, y );	// delete existing
	MapTile* tile = GetTile( x, y );
	tile->storage = storage;
	
	// Find an item:
	const ItemDef* itemDef = storage->SomeItem();
	if ( !itemDef ) {
		// empty.
		delete storage;
		tile->storage = 0;
	}
	else { 
		const ModelResource* res = itemDef->resource;
		if ( !res ) {
			res = ModelResourceManager::Instance()->GetModelResource( "crate" );
		}
		tile->debris = tree->AllocModel( res );
		const Rectangle3F& aabb = tile->debris->AABB();
		tile->debris->SetPos( (float)(x)+0.5f, -aabb.min.y, (float)(y)+0.5f );
		tile->debris->SetFlag( Model::MODEL_OWNED_BY_MAP );
	}
	*/
}


Storage* Map::RemoveStorage( int x, int y )
{
	/*
	MapTile* tile = GetTile( x, y );
	if ( tile->debris ) {
		tree->FreeModel( tile->debris );
		tile->debris = 0;
	}
	Storage* result = tile->storage;
	tile->storage = 0;
	return result;
	*/
	return 0;
}


void Map::DumpTile( int x, int y )
{
	if ( InRange( x, 0, SIZE-1 ) && InRange( y, 0, SIZE-1 )) 
	{
		int i=0;
		MapItem* root = quadTree.FindItems( x, y );
		while ( root ) {
			GLASSERT( root->itemDefIndex > 0 );
			const MapItemDef& itemDef = itemDefArr[ root->itemDefIndex ];
			GLASSERT( itemDef.name && *itemDef.name );

			int r = root->rot;
			UFOText::Draw( 0, 100-12*i, "%s r=%d", itemDef.name, r*90 );

			++i;
			root = root->next;
		}
	}
}


void Map::DrawPath()
{
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDisable( GL_TEXTURE_2D );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			float x = (float)i;
			float y = (float)j;

			//const Tile& tile = tileArr[j*SIZE+i];
			int path = GetPathMask( PATH_TYPE, i, j );

			if ( path == 0 ) 
				continue;

			Vector3F red[12];
			Vector3F green[12];
			int	nRed = 0, nGreen = 0;	
			const float EPS = 0.2f;

			Vector3F edge[5] = {
				{ x, EPS, y+1.0f },
				{ x+1.0f, EPS, y+1.0f },
				{ x+1.0f, EPS, y },
				{ x, EPS, y },
				{ x, EPS, y+1.0f }
			};
			Vector3F center = { x+0.5f, 0, y+0.5f };

			for( int k=0; k<4; ++k ) {
				int mask = (1<<k);

				if ( path & mask ) {
					red[nRed+0] = edge[k];
					red[nRed+1] = edge[k+1];
					red[nRed+2] = center;
					nRed += 3;
				}
				else {
					green[nGreen+0] = edge[k];
					green[nGreen+1] = edge[k+1];
					green[nGreen+2] = center;
					nGreen += 3;
				}
			}
			GLASSERT( nRed <= 12 );
			GLASSERT( nGreen <= 12 );

			glColor4f( 1.f, 0.f, 0.f, 0.5f );
			glVertexPointer( 3, GL_FLOAT, 0, red );
 			glDrawArrays( GL_TRIANGLES, 0, nRed );	
			CHECK_GL_ERROR;

			glColor4f( 0.f, 1.f, 0.f, 0.5f );
			glVertexPointer( 3, GL_FLOAT, 0, green );
 			glDrawArrays( GL_TRIANGLES, 0, nGreen );	
			CHECK_GL_ERROR;
		}
	}

	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );
}


void Map::ResetPath()
{
	microPather->Reset();
	++pathQueryID;
	++visibilityQueryID;
}


void Map::ClearPathBlocks()
{
	ResetPath();
	pathBlock.ClearAll();
}


void Map::SetPathBlock( int x, int y )
{
	ResetPath();
	pathBlock.Set( x, y );
}


void Map::ClearVisPathMap( grinliz::Rectangle2I& bounds )
{
	for( int j=bounds.min.y; j<=bounds.max.y; ++j ) {
		for( int i=bounds.min.x; i<=bounds.max.x; ++i ) {
			visMap[j*SIZE+i] = 0;
			pathMap[j*SIZE+i] = 0;
		}
	}
}


void Map::CalcVisPathMap( grinliz::Rectangle2I& bounds )
{
	MapItem* item = quadTree.FindItems( bounds );
	while( item ) {
		
		if ( !item->Destroyed() ) {
			GLASSERT( item->itemDefIndex > 0 );
			const MapItemDef& itemDef = itemDefArr[item->itemDefIndex];
			
			int rot = item->rot;
			GLASSERT( rot >= 0 && rot < 4 );

			const Vector2I size  = { itemDef.cx, itemDef.cy };
			Vector2I walk = size;
			if ( rot & 1 )
				grinliz::Swap( &walk.x, &walk.y );

			Vector2I prime = { 0, 0 };

			for( int y=0; y<walk.y; ++y ) {
				for( int x=0; x<walk.x; ++x ) {

					Vector2I origin = { x, y };

					// Account for object rotation (if needed). Maps from the world space
					// back to object space to get the visibility.
					IMat iMat;
					if ( size.x > 1 || size.y > 1 ) {
						iMat.Init( size.x, size.y, rot );
						iMat.Mult( origin, &prime );
					}
					GLASSERT( prime.x >= 0 && prime.x < itemDef.cx );
					GLASSERT( prime.y >= 0 && prime.y < itemDef.cy );

					// Account for tile rotation. (Actually a bit rotation too, which is handy.)
					// The OR operation is important. This routine will write outside of the bounds,
					// and should do no damage.
					{
						// Path
						U32 p = ( itemDef.pather[prime.y][prime.x] << rot );
						p = p | (p>>4);
						pathMap[ (y+item->y)*SIZE + (x+item->x) ] |= p;
					}
					{
						// Visibility
						U32 p = ( itemDef.visibility[prime.y][prime.x] << rot );
						p = p | (p>>4);
						visMap[ (y+item->y)*SIZE + (x+item->x) ] |= p;
					}
				}
			}
		}
		item = item->next;
	}
}


int Map::GetPathMask( ConnectionType c, int x, int y )
{
	// fast return: if the pathBlock is set, we're done.
	if ( c == PATH_TYPE && pathBlock.IsSet( x, y ) ) {
		return 0xf;
	}
	return ( c==PATH_TYPE) ? pathMap[y*SIZE+x] : visMap[y*SIZE+x];
}


float Map::LeastCostEstimate( void* stateStart, void* stateEnd )
{
	Vector2<S16> start, end;
	StateToVec( stateStart, &start );
	StateToVec( stateEnd, &end );

	float dx = (float)(start.x-end.x);
	float dy = (float)(start.y-end.y);

	return sqrtf( dx*dx + dy*dy );
}


bool Map::Connected( ConnectionType c, int x, int y, int dir )
{
	const Vector2I next[4] = {
		{ 0, 1 },
		{ 1, 0 },
		{ 0, -1 },
		{ -1, 0 } 
	};

	GLASSERT( dir >= 0 && dir < 4 );
	GLASSERT( x >=0 && x < SIZE );
	GLASSERT( y >=0 && y < SIZE );

	int bit = 1<<dir;
	Vector2I pos = { x, y };
	Vector2I nextPos = pos + next[dir];

	// To be connected, it must be clear from a->b and b->a
	if ( InRange( pos.x, 0, SIZE-1 )     && InRange( pos.y, 0, SIZE-1 ) &&
		 InRange( nextPos.x, 0, SIZE-1 ) && InRange( nextPos.y, 0, SIZE-1 ) ) 
	{
		int mask0 = GetPathMask( c, pos.x, pos.y );
		int maskN = GetPathMask( c, nextPos.x, nextPos.y );
		int inv = InvertPathMask( bit );

		if ( (( mask0 & bit ) == 0 ) && (( maskN & inv ) == 0 ) ) {
			return true;
		}
	}
	return false;
}


void Map::AdjacentCost( void* state, std::vector< micropather::StateCost > *adjacent )
{
	Vector2<S16> pos;
	StateToVec( state, &pos );

	const Vector2<S16> next[8] = {
		{ 0, 1 },
		{ 1, 0 },
		{ 0, -1 },
		{ -1, 0 },

		{ 1, 1 },
		{ 1, -1 },
		{ -1, -1 },
		{ -1, 1 },
	};
	const float SQRT2 = 1.414f;
	const float cost[8] = {	1.0f, 1.0f, 1.0f, 1.0f, SQRT2, SQRT2, SQRT2, SQRT2 };

	adjacent->resize( 0 );
	// N S E W
	for( int i=0; i<4; i++ ) {
		if ( Connected( PATH_TYPE, pos.x, pos.y, i ) ) {
			Vector2<S16> nextPos = pos + next[i];

			micropather::StateCost stateCost;
			stateCost.cost = cost[i];
			stateCost.state = VecToState( nextPos );
			adjacent->push_back( stateCost );
		}
	}
	// Diagonals. Need to check if all the NSEW connections work. If
	// so, then the diagonal connection works too.
	for( int i=0; i<4; i++ ) {
		int j=(i+1)&3;
		Vector2<S16> nextPos = pos + next[i+4];
		int iInv = (i+2)&3;
		int jInv = (j+2)&3;
		 
		if (    Connected( PATH_TYPE, pos.x, pos.y, i ) 
			 && Connected( PATH_TYPE, pos.x, pos.y, j )
			 && Connected( PATH_TYPE, nextPos.x, nextPos.y, iInv )
			 && Connected( PATH_TYPE, nextPos.x, nextPos.y, jInv ) ) 
		{
			micropather::StateCost stateCost;
			stateCost.cost = cost[i+4];
			stateCost.state = VecToState( nextPos );
			adjacent->push_back( stateCost );
		}
	}
}


void Map::PrintStateInfo( void* state )
{
	Vector2<S16> pos;
	StateToVec( state, &pos );
	GLOUTPUT(( "[%d,%d]", pos.x, pos.y ));
}


int Map::SolvePath( const Vector2<S16>& start, const Vector2<S16>& end, float *cost, std::vector< void* >* path )
{
	GLASSERT( sizeof( int ) == sizeof( void* ));	// fix this for 64 bit
	GLASSERT( sizeof(Vector2<S16>) == sizeof( void* ) );

	int result = microPather->Solve(	VecToState( start ),
										VecToState( end ),
										path,
										cost );

	/*
	switch( result ) {
		case MicroPather::SOLVED:			GLOUTPUT(( "Solved nPath=%d\n", *nPath ));			break;
		case MicroPather::NO_SOLUTION:		GLOUTPUT(( "NoSolution nPath=%d\n", *nPath ));		break;
		case MicroPather::START_END_SAME:	GLOUTPUT(( "StartEndSame nPath=%d\n", *nPath ));	break;
		case MicroPather::OUT_OF_MEMORY:	GLOUTPUT(( "OutOfMemory nPath=%d\n", *nPath ));		break;
		default:	GLASSERT( 0 );	break;
	}
	*/
	return result;
}


void Map::ShowNearPath(	const grinliz::Vector2<S16>& start, float cost0, float cost1, float cost2 )
{
	GLASSERT( cost2 <= (float)MAX_TRAVEL );
	walkingMap.ClearAll();

	int result = microPather->SolveForNearStates( VecToState( start ), &stateCostArr, cost2 );
	/*
	GLOUTPUT(( "Near states, result=%d\n", result ));
	for( unsigned m=0; m<stateCostArr.size(); ++m ) {
		Vector2<S16> v;
		StateToVec( stateCostArr[m].state, &v );
		GLOUTPUT(( "  (%d,%d) cost=%.1f\n", v.x, v.y, stateCostArr[m].cost ));
	}
	*/

	if ( result == MicroPather::SOLVED ) {
		for( unsigned i=0; i<stateCostArr.size(); ++i ) {

			const micropather::StateCost& stateCost = stateCostArr[i];
			Vector2<S16> v;
			StateToVec( stateCost.state, &v );

#ifdef DEBUG
			{
				Rectangle3I rect;
				rect.Set( v.x, v.y, 0, v.x, v.y, 2 );
				GLASSERT( walkingMap.IsRectEmpty( rect ));
			}
#endif

			if ( stateCost.cost <= cost0 ) {
				walkingMap.Set( v.x, v.y, 0 );
			}
			else if ( stateCost.cost <= cost1 ) {
				walkingMap.Set( v.x, v.y, 1 );
			}
			else {
				walkingMap.Set( v.x, v.y, 2 );
			}
		}
	}

	/*
	nWalkingVertex = 0;
	PushWalkingVertex( 0,  0,   0.f,       0.f );
	PushWalkingVertex( 64, 64, 0.25f, 0.25f );
	PushWalkingVertex( 64, 0,   0.25f, 0.f );

	PushWalkingVertex( 0,   0,   0.f,       0.f );
	PushWalkingVertex( 0,   64, 0.f,       0.25f );
	PushWalkingVertex( 64, 64, 0.25f, 0.25f );
	*/

	nWalkingVertex = 0;
	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			U32 set = walkingMap.IsSet( i, j, 0 ) | walkingMap.IsSet( i, j, 1 ) | walkingMap.IsSet( i, j, 2 );
			if ( set ) {
				float tx = 0.0f;
				float ty = 0.75f;
				if ( walkingMap.IsSet( i, j, 1 ) )
					tx = 0.25f;
				else if (walkingMap.IsSet( i, j, 2 ) )
					tx = 0.50f;

				PushWalkingVertex( i,   j,   tx,       ty );
				PushWalkingVertex( i+1, j+1, tx+0.25f, ty+0.25f );
				PushWalkingVertex( i+1, j,   tx+0.25f, ty );

				PushWalkingVertex( i,   j,   tx,       ty );
				PushWalkingVertex( i,   j+1, tx,       ty+0.25f );
				PushWalkingVertex( i+1, j+1, tx+0.25f, ty+0.25f );
			}
		}
	}
}


void Map::ClearNearPath()
{
	nWalkingVertex = 0;
}


bool Map::CanSee( const grinliz::Vector2I& p, const grinliz::Vector2I& q )
{
	GLASSERT( InRange( q.x-p.x, -1, 1 ) );
	GLASSERT( InRange( q.y-p.y, -1, 1 ) );
	GLASSERT( p.x >=0 && p.x < SIZE );
	GLASSERT( p.y >=0 && p.y < SIZE );
	GLASSERT( q.x >=0 && q.x < SIZE );
	GLASSERT( q.y >=0 && q.y < SIZE );

	int dx = q.x-p.x;
	int dy = q.y-p.y;
	bool canSee = false;
	int dir0 = -1;
	int dir1 = -1;

	switch( dy*4 + dx ) {
		case 4:		// S
			dir0 = 0;
			break;
		case 5:		// SE
			dir0 = 0;
			dir1 = 1;
			break;
		case 1:		// E
			dir0 = 1;
			break;
		case -3:	// NE
			dir0 = 1;
			dir1 = 2;
			break;
		case -4:	// N
			dir0 = 2;
			break;
		case -5:	// NW
			dir0 = 2;
			dir1 = 3;
			break;
		case -1:	// W
			dir0 = 3;
			break;
		case 3:		// SW
			dir0 = 3;
			dir1 = 0;
			break;
		default:
			GLASSERT( 0 );
			break;
	}
	if ( dir1 == -1 ) {
		canSee = Connected( VISIBILITY_TYPE, p.x, p.y, dir0 );
	}
	else {
		const Vector2I next[4] = {	{ 0, 1 },
									{ 1, 0 },
									{ 0, -1 },
									{ -1, 0 } };

		Vector2I q0 = p+next[dir0];
		Vector2I q1 = p+next[dir1];

		canSee =  ( Connected( VISIBILITY_TYPE, p.x, p.y, dir0 ) && Connected( VISIBILITY_TYPE, q0.x, q0.y, dir1 ) )
			   || ( Connected( VISIBILITY_TYPE, p.x, p.y, dir1 ) && Connected( VISIBILITY_TYPE, q1.x, q1.y, dir0 ) );
	}
	return canSee;
}


void Map::MapBoundsOfModel( const Model* m, grinliz::Rectangle2I* mapBounds )
{
	GLASSERT( m->IsFlagSet( Model::MODEL_OWNED_BY_MAP ) );
	MapItem* item = quadTree.FindItem( m );
	GLASSERT( item );
	item->MapBounds( mapBounds );
	GLASSERT( mapBounds->min.x <= mapBounds->max.x );
	GLASSERT( mapBounds->min.y <= mapBounds->max.y );
}


Map::QuadTree::QuadTree()
{
	Clear();
	filterModel = 0;

	int base = 0;
	for( int i=0; i<QUAD_DEPTH+1; ++i ) {
		depthBase[i] = base;
		base += (1<<i)*(1<<i);
	}
}


void Map::QuadTree::Clear()
{
	memset( tree, 0, sizeof(MapItem*)*NUM_QUAD_NODES );
	memset( depthUse, 0, sizeof(int)*(QUAD_DEPTH+1) );
}


void Map::QuadTree::Add( MapItem* item )
{
	int d=0;
	int i = CalcNode( item->mapBounds8, &d );
	item->nextQuad = tree[i];
	tree[i] = item;
	depthUse[d] += 1;

	GLOUTPUT(( "Depth: %2d %2d %2d %2d %2d\n", 
			   depthUse[0], depthUse[1], depthUse[2], depthUse[3], depthUse[4] ));
}


void Map::QuadTree::UnlinkItem( MapItem* item )
{
	int index = CalcNode( item->mapBounds8, 0 );
	GLASSERT( tree[index] ); // the item should be in the linked list somewhere.

	MapItem* prev = 0;
	for( MapItem* p=tree[index]; p; prev=p, p=p->nextQuad ) {
		if ( p == item ) {
			if ( prev )
				prev->nextQuad = p->nextQuad;
			else
				tree[index] = p->nextQuad;
			return;
		}
	}
	GLASSERT( 0 );	// should have found the item.
}


Map::MapItem* Map::QuadTree::FindItems( const Rectangle2I& bounds )
{
	// Walk the map and pull out items in bounds.
	MapItem* root = 0;

	GLASSERT( bounds.min.x >= 0 && bounds.max.x < 256 );	// using int8
	GLASSERT( bounds.min.y >= 0 && bounds.max.y < 256 );	// using int8
	Rectangle2<U8> bounds8;
	bounds8.Set( bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y );

	for( int depth=0; depth<QUAD_DEPTH; ++depth ) 
	{
		// Translate into coordinates for this node level:
		int x0 = WorldToNode( bounds.min.x, depth );
		int x1 = WorldToNode( bounds.max.x, depth );
		int y0 = WorldToNode( bounds.min.y, depth );
		int y1 = WorldToNode( bounds.max.y, depth );

		// i, j, x0.. all in node coordinates.
		for( int j=y0; j<=y1; ++j ) {
			for( int i=x0; i<=x1; ++i ) {
				MapItem* pItem = *(tree + depthBase[depth] + NodeOffset( i, j, depth ) );

				if ( filterModel ) {
					while( pItem ) {
						if ( pItem->model == filterModel ) {
							pItem->next = 0;
							return pItem;
						}
						pItem = pItem->nextQuad;
					}
				}
				else {
					while( pItem ) { 
						if ( pItem->mapBounds8.Intersect( bounds8 ) ) {
							pItem->next = root;
							root = pItem;
						}
						pItem = pItem->nextQuad;
					}
				}
			}
		}
	}
	return root;
}


Map::MapItem* Map::QuadTree::FindItem( const Model* model )
{
	Rectangle2I b;
	b.min.x = (int)floorf( model->X() );
	b.min.y = (int)floorf( model->Z() );
	b.max.x = (int)ceilf( model->X() );
	b.max.y = (int)ceilf( model->Z() );

	filterModel = model;
	MapItem* root = FindItems( b );
	filterModel = 0;
	return root;
}


int Map::QuadTree::CalcNode( const Rectangle2<U8>& bounds, int* d )
{
	int offset = 0;

	for( int depth=QUAD_DEPTH-1; depth>0; --depth ) 
	{
		int x0 = WorldToNode( bounds.min.x, depth );
		int y0 = WorldToNode( bounds.min.y, depth );

		int wSize = SIZE >> depth;

		Rectangle2<U8> wBounds;
		wBounds.Set(	NodeToWorld( x0, depth ),
						NodeToWorld( y0, depth ),
						NodeToWorld( x0, depth ) + wSize - 1,
						NodeToWorld( y0, depth ) + wSize - 1 );

		if ( wBounds.Contains( bounds ) ) {
			offset = depthBase[depth] + NodeOffset( x0, y0, depth );
			if ( d )
				*d = depth;
			break;
		}
	}
	GLASSERT( offset >= 0 && offset < NUM_QUAD_NODES );
	return offset;
}
