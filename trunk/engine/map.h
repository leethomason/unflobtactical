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
#include "../grinliz/glmemorypool.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glgeometry.h"
#include "../micropather/micropather.h"
#include "vertex.h"
#include "enginelimits.h"
#include "serialize.h"
#include "ufoutil.h"
#include "surface.h"
#include "texture.h"
#include "gpustatemanager.h"
#include "../shared/glmap.h"
#include "../gamui/gamui.h"
#include "../game/gamelimits.h"			// bad call to less general directory. FIXME. Move map to game?

class Model;
class ModelResource;
class SpaceTree;
class RenderQueue;
class ItemDef;
class Storage;
class Game;
struct DamageDesc;
class SustainedPyroEffect;
class ParticleSystem;
class TiXmlElement;
class Map;
class ItemDefArr;


class IPathBlocker
{
public:
	virtual void MakePathBlockCurrent( Map* map, const void* user ) = 0;
};

#ifdef USE_MAP_CACHE
struct MapRenderBlock 
{

	U16 nVertex;
	U16 tempNVertex;
	U16 nIndex;
	U16 tempNIndex;

	MapRenderBlock* next;
	Texture* texture;
	GPUVertexBuffer vertexBuffer;
	GPUIndexBuffer indexBuffer;

	void Init() {
		nVertex = 0;
		tempNVertex = 0;
		nIndex = 0;
		tempNIndex = 0;

		next = 0;
		texture = 0;
		vertexBuffer.Clear();
		indexBuffer.Clear();
	}
};
#endif


// some strange android bug - the size of the structure gets mangled
// by the compiler if all the fields weren't 32 bits
struct MapItemDef 
{
	enum {
		OBSCURES = 0x01,
		EXPLODES = 0x02
	};

	const char* resource;
	const char* resourceOpen;
	const char* resourceDestroyed;

	int		cx, cy;
	int		hp;					// 0xffff infinite, 0 destroyed
	int		flammable;			// 0 - 255 flammability

	const char* patherStr;
	const char* visibilityStr;
	int flags;					// obscures, like smoke, haypiles, and trees

	int		lightDef;			// itemdef index of the light associated with this (is auto-created)
	int		lightOffsetX;		// if light, offset of light origin from model origin
	int		lightOffsetY;
	int		lightTX;			// if a light, x location in texture
	int		lightTY;			// if a light, y location in texture

	const char* Name() const { return resource; }
	const ModelResource* GetModelResource() const;
	const ModelResource* GetOpenResource() const;
	const ModelResource* GetDestroyedResource() const;

	U32 Pather( int x, int y ) const {
		GLRELASSERT( x >= 0 && y >= 0 && x < cx && y < cy );
		
		if ( !patherStr )
			return 0;
		
		GLASSERT( strlen( patherStr ) == cx*cy );
		GLASSERT( strlen( visibilityStr ) == cx*cy );

		const char c = *(patherStr + y*cx + x );
		return grinliz::HexLowerCharToInt( c );
	}

	U32 Visibility( int x, int y ) const 
	{
		GLRELASSERT( x < cx && y < cy );
		if ( !visibilityStr )
			return 0;

		GLASSERT( strlen( patherStr ) == cx*cy );
		GLASSERT( strlen( visibilityStr ) == cx*cy );

		const char c = *(visibilityStr + y*cx + x );
		return grinliz::HexLowerCharToInt( c );
	}

	// return true if the object can take damage
	bool CanDamage() const	{ return hp != 0xffff; }
	int HasLight() const	{ return lightDef; }
	bool IsLight() const	{ return lightTX || lightTY; }
	bool IsDoor() const		{ return resourceOpen != 0; }

	grinliz::Rectangle2I Bounds() const 
	{
		grinliz::Rectangle2I b;
		b.Set( 0, 0, cx-1, cy-1 );
		return b;
	}
};


class Map : public micropather::Graph,
			public ITextureCreator,
			public gamui::IGamuiRenderer
{
public:
	enum {
		SIZE = 64,					// maximum size. FIXME: duplicated in gamelimits.h
		LOG2_SIZE = 6,

		NUM_ITEM_DEF = 72,
	};


	struct MapItem
	{
		enum {
			MI_NOT_IN_DATABASE		= 0x02,		// lights are usually generated, and are not stored in the DB
			MI_DOOR					= 0x04,
		};

		struct MatPacked {
			S8	a, b, c, d;
			S16	x, y;
		};

		U8			open;
		U8			itemDefIndex;	
		U8			modelRotation;
		U16			hp;
		U16			flags;
		MatPacked	xform;			// xform in map coordinates
		grinliz::Rectangle2<U8> mapBounds8;
		float		modelX, modelZ;
		
		Model*	 model;
		MapItem* light;
		
		MapItem* next;			// the 'next' after a query
		MapItem* nextQuad;		// next pointer in the quadTree

		grinliz::Rectangle2I MapBounds() const 
		{	
			grinliz::Rectangle2I r;
			r.Set( mapBounds8.min.x, mapBounds8.min.y, mapBounds8.max.x, mapBounds8.max.y );
			return r;
		}
		Matrix2I XForm() const {
			Matrix2I m;
			m.a = xform.a;	m.b = xform.b; 
			m.c = xform.c;	m.d = xform.d; 
			m.x = xform.x;	m.y = xform.y;
			return m;
		}
		void SetXForm( const Matrix2I& m ) {
			GLASSERT( m.a > -128 && m.a < 128 );
			GLASSERT( m.b > -128 && m.b < 128 );
			GLASSERT( m.c > -128 && m.c < 128 );
			GLASSERT( m.d > -128 && m.d < 128 );
			GLASSERT( m.x > -30000 && m.x < 30000 );
			GLASSERT( m.y > -30000 && m.y < 30000 );
			xform.a = (S8)m.a;
			xform.b = (S8)m.b;
			xform.c = (S8)m.c;
			xform.d = (S8)m.d;
			xform.x = (S16)m.x;
			xform.y = (S16)m.y;
		}

		grinliz::Vector3F ModelPos() const { 
			grinliz::Vector3F v = { modelX, 0.0f, modelZ };
			return v;
		}
		float ModelRot() const { return (float)(modelRotation*90); }

		// returns true if destroyed
		bool DoDamage( int _hp )		
		{	
			if ( _hp >= hp ) {
				hp = 0;
				return true;
			}
			hp -= _hp;
			return false;						
		}
		bool Destroyed() const { return hp == 0; }
	};

	/* FIXME: The map lives between the game and the engine. It should probably be moved
	   to the Game layer. Until then, it takes an engine primitive (SpaceTree) and the game
	   basic class (Game) when it loads. Very strange.
	*/
	Map( SpaceTree* tree );
	virtual ~Map();

	void SetPathBlocker( IPathBlocker* blocker ) { pathBlocker = blocker; pathBlock.ClearAll(); }

	// The size of the map in use, which is <=SIZE
	int Height() const { return height; }
	int Width()  const { return width; }
	grinliz::Rectangle2I Bounds() const					{	grinliz::Rectangle2I b;
															b.Set( 0, 0, width-1, height-1 ); 
															return b;
														}
	void ClipToMap( grinliz::Rectangle2I* b ) const		{	if ( b->min.x < 0 ) b->min.x = 0;
															if ( b->min.y < 0 ) b->min.y = 0;
															if ( b->max.x >= width ) b->max.x = width-1;
															if ( b->max.y >= height ) b->max.y = height-1;
														}

	void SetSize( int w, int h );
	const Model* GetLanderModel()					{ return lander ? lander->model : 0; }

	bool DayTime() const { return dayTime; }
	void SetDayTime( bool day );

	const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>&	GetFogOfWar()		{ return fogOfWar; }
	grinliz::BitArray<Map::SIZE, Map::SIZE, 1>*			LockFogOfWar();
	void												ReleaseFogOfWar();

	// Light Map
	// 0: Light map that was set in "SetLightMap", or white if none set
	// Not currently used: 1: Light map 0 + lights
	// Not currently used: 2: Light map 0 + lights + FogOfWar
	const Surface* GetLightMap()	{ GenerateLightMap(); return lightMap; }
	void SetLightMap0( int x, int y, float r, float g, float b );

	void GenerateSeenUnseen();	// NOT cached, so call only once/frame. (I'm not sure how to cache this effectively. Caching 
								// attempts were very buggy.) Call before DrawSeen(), DrawUnseen(), or DrawPastSeen().
	void DrawSeen();		//< draw the map that is currently visible
	void DrawUnseen();		//< draw the map that currently can't be seen
	void DrawPastSeen( const Color4F& );		//< draw the map that currently can't be seen

	void DrawPath( int mode );		//< debugging
	void DrawOverlay( int layer );		//< draw the "where can I walk" alpha overlay. Set up by ShowNearPath().

	// Do damage to a singe map object.
	void DoDamage( Model* m, const DamageDesc& damage, grinliz::Rectangle2I* destroyedBounds, grinliz::Vector2I* explosion  );
	// Do damage to an entire map tile.
	void DoDamage( int x, int y, const DamageDesc& damage, grinliz::Rectangle2I* destroyedBounds, grinliz::Vector2I* explosion );
	
	// Process a sub-turn: fire moves, smoke goes away, etc.
	void DoSubTurn( grinliz::Rectangle2I* changeBounds );

	//void SetLanderFlight( float normal );
	//float GetLanderFlight() const			{ return landerFlight; }

	// Smoke from weapons, explosions, effects, etc.
	void AddSmoke( int x, int y, int subturns );
	// Returns true if view obscured by smoke, fire, etc.
	bool Obscured( int x, int y ) const		{ return ( obscured[y*SIZE+x] | PyroOn( x, y ) ) != 0; }
	void EmitParticles( U32 deltaTime );

	// Set the path block (does nothing if they are equal.)
	// Generally called by MakePathBlockCurrent
	void SetPathBlocks( const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>& block );

	Storage* LockStorage( int x, int y, const ItemDefArr& );	// always returns something.
	void ReleaseStorage( Storage* storage );					// updates the image

	const Storage* GetStorage( int x, int y ) const;		//< take a peek
	void FindStorage( const ItemDef* itemDef, int maxLoc, grinliz::Vector2I* loc, int* numLoc );

	const char* GetItemDefName( int i );
	const MapItemDef* GetItemDef( const char* name );

	// MapMaker method to create a translucent preview.
	Model* CreatePreview( int x, int z, int itemDefIndex, int rotation );

	// hp = -1 default
	//       0 destroyed
	//		1+ hp remaining
	// Storage is owned by the map after this call.
	MapItem* AddItem( int x, int z, int rotation, int itemDefIndex, int hp, int flags );
	// Utility call to AddItem() that preprocesses the x,z params.
	//MapItem* AddLightItem( int x, int z, int rotation, int itemDefIndex, int hp, int flags );

	void DeleteAt( int x, int z );
	void MapBoundsOfModel( const Model* m, grinliz::Rectangle2I* mapBounds );

	void ResetPath();	// normally called automatically
	void Clear();

	void DumpTile( int x, int z );

	// Solves a path on the map. Returns total cost. 
	// returns MicroPather::SOLVED, NO_SOLUTION, START_END_SAME, or OUT_OF_MEMORY
	int SolvePath(	const void* user,
					const grinliz::Vector2<S16>& start,
					const grinliz::Vector2<S16>& end,
					float* cost,
					MP_VECTOR< grinliz::Vector2<S16> >* path );
	
	// Show the path that the unit can walk to.
	void ShowNearPath(	const grinliz::Vector2I& unitPos,
						const void* user,
						const grinliz::Vector2<S16>& start,
						float maxCost,
						const grinliz::Vector2F* range );
	void ClearNearPath();	

	// micropather:
	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
	virtual void  AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent );
	virtual void  PrintStateInfo( void* state );

	// ITextureCreator
	virtual void CreateTexture( Texture* t );

	// IGamuiRenderer
	enum {
		RENDERSTATE_MAP_NORMAL = 100,
		RENDERSTATE_MAP_OPAQUE,
		RENDERSTATE_MAP_TRANSLUCENT
	};
	virtual void BeginRender();
	virtual void EndRender();
	virtual void BeginRenderState( const void* renderState );
	virtual void BeginTexture( const void* textureHandle );
	virtual void Render( const void* renderState, const void* textureHandle, int nIndex, const uint16_t* index, int nVertex, const gamui::Gamui::Vertex* vertex );

	Texture* BackgroundTexture()	{ return backgroundTexture; }
	Texture* LightMapTexture()		{ return lightMapTex; }
	Texture* LightFogMapTexture()	{ return lightFogMapTex; }

#ifdef USE_MAP_CACHE
	const MapRenderBlock* CalcRenderBlocks( const grinliz::Plane* planes, int nPlanes );
#endif

	enum ConnectionType {
		PATH_TYPE,
		VISIBILITY_TYPE
	};
	// visibility (but similar to AdjacentCost conceptually). If PATH_TYPE is
	// passed in for the connection, it becomes CanWalk
	bool CanSee( const grinliz::Vector2I& p, const grinliz::Vector2I& q, ConnectionType connection=VISIBILITY_TYPE );

	bool ProcessDoors( const grinliz::Vector2I* openers, int nOpeners );
	void SetPyro( int x, int y, int duration, int fire );

	void Save( FILE* fp, int depth );
	void Load( const TiXmlElement* mapNode, const ItemDefArr& );

	// Gets a starting location for a unit on the map.
	// TERRAN_TEAM - from the lander
	// ALIEN_TEAM, guard or scout - on the map metadata
	//
	void PopLocation( int team, bool guard, grinliz::Vector2I* pos, float* rotation );

	static void MapImageToWorld( int x, int y, int w, int h, int tileRotation, Matrix2I* mat );

	enum {
		LAYER_UNDER_LOW,		// obscurred by unseen and past-seen
		LAYER_UNDER_HIGH,		// covers unseen and past-seen
		LAYER_OVER,				// on top of models
		NUM_LAYERS
	};
	gamui::Gamui	overlay[NUM_LAYERS];
	grinliz::Random random;

private:
	static const int padArr0[16];
	static const MapItemDef itemDefArr[NUM_ITEM_DEF];
	static const int padArr1[16];

	// 0,90,180,270 rotation
	static void XYRToWorld( int x, int y, int rotation, Matrix2I* mat );
	// 0,90,180,270 rotation
	static void WorldToXYR( const Matrix2I& mat, int *x, int *y, int* r, bool useRot0123 = false );

	static void WorldToModel( const Matrix2I& mat, bool billboard, grinliz::Vector3F* m );

	int InvertPathMask( U32 m ) {
		U32 m0 = (m<<2) | (m>>2);
		return m0 & 0xf;
	}

	class QuadTree
	{
	public:
		enum {
			QUAD_DEPTH = 5,
			NUM_QUAD_NODES = 1+4+16+64+256,
		};

		QuadTree();
		void Clear();

		void Add( MapItem* );

		MapItem* FindItems( const grinliz::Rectangle2I& bounds, int required, int excluded );
		MapItem* FindItems( int x, int y, int required, int excluded ) 
		{ 
			grinliz::Rectangle2I b; b.Set( x, y, x, y ); 
			return FindItems( b, required, excluded ); 
		}
		MapItem* FindItem( const Model* model );

		void UnlinkItem( MapItem* item );

	private:
		int WorldToNode( int x, int depth )					
		{ 
			GLRELASSERT( depth >=0 && depth < QUAD_DEPTH );
			GLRELASSERT( x>=0 && x < Map::SIZE );
			return x >> (LOG2_SIZE-depth); 
		}
		int NodeToWorld( int x0, int depth )
		{
			GLRELASSERT( x0>=0 && x0 < (1<<depth) );
			return x0 << (LOG2_SIZE-depth);			
		}
		int NodeOffset( int x0, int y0, int depth )	
		{	
			GLRELASSERT( x0>=0 && x0 < (1<<depth) );
			GLRELASSERT( y0>=0 && y0 < (1<<depth) );
			return y0 * (1<<depth) + x0; 
		}

		int CalcNode( const grinliz::Rectangle2<U8>& bounds, int* depth );

		int			depthUse[QUAD_DEPTH];
		int			depthBase[QUAD_DEPTH+1];
		MapItem*	tree[NUM_QUAD_NODES];
		const Model* filterModel;
	};

	// The background texture of the map.
	void SetTexture( const Surface* surface, int x, int y, int tileRotation );
	// The light map is a 64x64 texture of the lighting at each point. Without
	// a light map, full white (daytime) is used. The 'x,y,size' parameters support
	// setting the lightmap in parts.
	void SetLightMaps( const Surface* day, const Surface* night, int x, int y, int tileRotation );

	int GetPathMask( ConnectionType c, int x, int z );
	bool Connected4( ConnectionType c, 
					 const grinliz::Vector2<S16>& from,
					 const grinliz::Vector2<S16>& delta );
	bool Connected8( ConnectionType c, 
					 const grinliz::Vector2<S16>& from,
					 const grinliz::Vector2<S16>& delta );

	void StateToVec( const void* state, grinliz::Vector2<S16>* vec ) { *vec = *((grinliz::Vector2<S16>*)&state); }
	void* VecToState( const grinliz::Vector2<S16>& vec )			 { return (void*)(*(int*)&vec); }

	void ClearVisPathMap( grinliz::Rectangle2I& bounds );
	void CalcVisPathMap( grinliz::Rectangle2I& bounds );

	void DeleteItem( MapItem* item );
	void UpdateRenderBlock( int x, int y );

	bool dayTime;
	IPathBlocker* pathBlocker;
	int width, height;
	grinliz::Rectangle3F bounds;
	SpaceTree* tree;
	//float landerFlight;

	Texture* backgroundTexture;		// background texture
	Surface backgroundSurface;		// background surface

	Texture* greyTexture;			// version for previous seen terrain
	Surface greySurface;

	void QueryAllDoors();			// figure out where the doors are, and write the doorArray
	CDynArray< MapItem* >	doorArr;

	enum { MAX_IMAGE_DATA = 16 };
	struct ImageData {
		int x, y, size, tileRotation;
		grinliz::CStr<EL_FILE_STRING_LEN> name;
	};
	int nImageData;

	void GenerateLightMap();

	const Surface* lightMap;
	Surface dayMap, nightMap;
	bool lightMapValid;
	Texture* lightMapTex;

	Surface lightFogMap;
	Texture* lightFogMapTex;

	grinliz::BitArray<Map::SIZE, Map::SIZE, 1> fogOfWar;
	grinliz::BitArray<Map::SIZE, Map::SIZE, 1> cachedFogOfWar;
	grinliz::BitArray<Map::SIZE, Map::SIZE, 1> pastSeenFOW;
	Surface lightObject;

	U32 pathQueryID;
	U32 visibilityQueryID;

	micropather::MicroPather* microPather;

	struct Debris {
		Storage* storage;
		Model* crate;
	};
	CDynArray< Debris > debris;
	void SaveDebris( const Debris& d, FILE* fp, int depth );
	void LoadDebris( const TiXmlElement* mapNode, const ItemDefArr& );

	int PyroOn( int x, int y ) const		{ return pyro[y*SIZE+x]; }
	int PyroFire( int x, int y ) const		{ return pyro[y*SIZE+x] & 0x80; }
	bool PyroSmoke( int x, int y ) const	{ return PyroOn( x, y ) && !PyroFire( x, y ); }
	int PyroDuration( int x, int y ) const	{ return pyro[y*SIZE+x] & 0x7F; }

	void ChangeObscured( const grinliz::Rectangle2I& bounds, int delta );

	grinliz::BitArray<SIZE, SIZE, 1>			pathBlock;	// spaces the pather can't use (units are there)	

	MP_VECTOR<void*>							mapPath;
	MP_VECTOR< micropather::StateCost >			stateCostArr;

	CompositingShader							gamuiShader;
	gamui::TiledImage<MAX_TU*2+1, MAX_TU*2+1>	walkingMap;
	gamui::Image								border[4];

	grinliz::MemoryPool							itemPool;
	QuadTree									quadTree;
	CStringMap< const MapItemDef* >				itemDefMap;
	
	CDynArray< grinliz::Vector2I >				guardPos;
	CDynArray< grinliz::Vector2I >				scoutPos;
	CDynArray< grinliz::Vector2I >				civPos;

	int											nLanderPos;
	const MapItem*								lander;
	int											nSeenIndex, nUnseenIndex, nPastSeenIndex;

#ifdef USE_MAP_CACHE
	enum {
		RENDER_BLOCK_GRID_SIZE = 16,
		RENDER_BLOCK_SIZE = SIZE / RENDER_BLOCK_GRID_SIZE,
		NUM_RENDER_BLOCKS = RENDER_BLOCK_SIZE*RENDER_BLOCK_SIZE
	};
	CDynArray<MapRenderBlock> renderBlockArr[NUM_RENDER_BLOCKS];
	CDynArray<Vertex> vertexBuffer;
	CDynArray<U16> indexBuffer;
#endif
	ImageData imageData[ MAX_IMAGE_DATA ];

	// U8:
	// bits 0-6:	sub-turns remaining (0-127)		(0x7F)
	// bit    7:	set: fire, clear: smoke			(0x80)
	U8 pyro[SIZE*SIZE];
	// This is a count. As an object (that obscures) is added, this gets added too.
	// Subtracted back out when the object is removed.
	U8 obscured[SIZE*SIZE];

	U8									visMap[SIZE*SIZE];
	U8									pathMap[SIZE*SIZE];

	grinliz::Vector2F					mapVertex[(SIZE+1)*(SIZE+1)];		// in TEXTURE coordinates - need to scale up and swizzle for vertices.

	U16									seenIndex[SIZE*SIZE*6];		
	U16									unseenIndex[SIZE*SIZE*6];		
	U16									pastSeenIndex[SIZE*SIZE*6];		
};

#endif // UFOATTACK_MAP_INCLUDED
