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
#include "../micropather/micropather.h"
#include "vertex.h"
#include "enginelimits.h"
#include "serialize.h"
#include "ufoutil.h"
#include "surface.h"
#include "texture.h"
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


class IPathBlocker
{
public:
	virtual void MakePathBlockCurrent( Map* map, const void* user ) = 0;
};


class Map : public micropather::Graph,
			public ITextureCreator,
			public gamui::IGamuiRenderer
{
public:
	enum {
		SIZE = 64,					// maximum size. FIXME: duplicated in gamelimits.h
		LOG2_SIZE = 6,

		MAX_ITEM_DEF = 256,
		LIGHT_START = 0xD0			// where the lights start in the itemDef array
	};

	struct LightItemDef
	{
		int x, y;	// image space
		int w, h;	// image space
	};

	struct Mat2I
	{
		int a, b, c, d, tx, ty;
	};

	struct MapItemDef 
	{
		enum {	MAX_CX = 6, 
				MAX_CY = 6,
			 };


		void Init() {	cx = 1; 
						cy = 1; 
						hp = 0xffff; 
						flammable = 0;

						modelResource			= 0;
						modelResourceOpen		= 0;
						modelResourceDestroyed	= 0;

						memset( pather, 0, MAX_CX*MAX_CY );
						memset( visibility, 0, MAX_CX*MAX_CY );

						lightDef = 0;
						lightOffsetX = 0;
						lightOffsetY = 0;
						lightTX = 0;
						lightTY = 0;
					}

		U16		cx, cy;
		U16		hp;					// 0xffff infinite, 0 destroyed
		U8		flammable;			// 0 - 255
		U8		lightDef;			// itemdef index of the light associated with this (is auto-created)
		S8		lightOffsetX;		// if light, offset of light origin from model origin
		S8		lightOffsetY;
		U8		lightTX;			// if a light, x location in texture
		U8		lightTY;			// if a light, y location in texture

		const ModelResource* modelResource;
		const ModelResource* modelResourceOpen;
		const ModelResource* modelResourceDestroyed;

		grinliz::CStr< EL_FILE_STRING_LEN > name;
		U8		pather[MAX_CX][MAX_CY];
		U8		visibility[MAX_CX][MAX_CY];

		// return true if the object can take damage
		bool CanDamage() const	{ return hp != 0xffff; }
		int HasLight() const	{ return lightDef; }
		bool IsLight() const	{ return lightTX || lightTY; }
		bool IsDoor() const		{ return modelResourceOpen != 0; }
		grinliz::Rectangle2I Bounds() const 
		{
			grinliz::Rectangle2I b;
			b.Set( 0, 0, cx-1, cy-1 );
			return b;
		}
	};

	struct MapItem
	{
		enum {
			MI_IS_LIGHT				= 0x01,
			MI_NOT_IN_DATABASE		= 0x02,		// lights are usually generated, and are not stored in the DB
			MI_DOOR					= 0x04,
		};

		struct MatPacked {
			S8	a, b, c, d;
			S16	x, y;
		};

		U8			open;
		U8			itemDefIndex;	// 0: not in use, >0 is the index
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

		bool InUse() const			{ return itemDefIndex > 0; }
		bool IsLight() const		{ return flags & MI_IS_LIGHT; }

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
	void CalcBounds( grinliz::Rectangle2I* b ) const	{ b->Set( 0, 0, width-1, height-1 ); }

	void SetSize( int w, int h );
	const Model* GetLanderModel()					{ return lander ? lander->model : 0; }

	bool DayTime() const { return dayTime; }
	void SetDayTime( bool day );

	const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>&	GetFogOfWar()		{ return fogOfWar; }
	grinliz::BitArray<Map::SIZE, Map::SIZE, 1>*			LockFogOfWar();
	void												ReleaseFogOfWar();

	// Light Map
	// 0: Light map that was set in "SetLightMap", or white if none set
	// 1: Light map 0 + lights
	// Not currently used: 2: Light map 0 + lights + FogOfWar
	const Surface* GetLightMap( int i)	{ GLASSERT( i>=0 && i<2 ); GenerateLightMap(); return &lightMap[i]; }

	// Rendering.
	void BindTextureUnits();
	void UnBindTextureUnits();

	void Draw();			//< draw the map, without the FOW
	void DrawPath();		//< debugging
	void DrawOverlay( int layer );		//< draw the "where can I walk" alpha overlay. Set up by ShowNearPath().
	void DrawFOW();			//< black out the regions where the FOW is.

	// Do damage to a singe map object.
	void DoDamage( Model* m, const DamageDesc& damage, grinliz::Rectangle2I* destroyedBounds );
	// Do damage to an entire map tile.
	void DoDamage( int x, int y, const DamageDesc& damage, grinliz::Rectangle2I* destroyedBounds );
	
	// Process a sub-turn: fire moves, smoke goes away, etc.
	void DoSubTurn( grinliz::Rectangle2I* changeBounds );

	// Smoke from weapons, explosions, effects, etc.
	void AddSmoke( int x, int y, int subturns );
	// Returns true if view obscured by smoke, fire, etc.
	bool Obscured( int x, int y ) const		{ return PyroOn( x, y ) ? true : false; }
	void EmitParticles( U32 deltaTime );

	// Set the path block (does nothing if they are equal.)
	// Generally called by MakePathBlockCurrent
	void SetPathBlocks( const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>& block );

	Storage* LockStorage( int x, int y );					//< can return 0 if none there
	void ReleaseStorage( int x, int y, Storage* storage, ItemDef* const* arr );	//< sets the storage

	const Storage* GetStorage( int x, int y ) const;		//< take a peek
	void FindStorage( const ItemDef* itemDef, int maxLoc, grinliz::Vector2I* loc, int* numLoc );

	MapItemDef* InitItemDef( int i );
	const char* GetItemDefName( int i );
	const MapItemDef* GetItemDef( const char* name );

#ifdef MAPMAKER
	Model* CreatePreview( int x, int z, int itemDefIndex, int rotation );
#endif
	// hp = -1 default
	//       0 destroyed
	//		1+ hp remaining
	// Storage is owned by the map after this call.
	MapItem* AddItem( int x, int z, int rotation, int itemDefIndex, int hp, int flags );
	// Utility call to AddItem() that preprocesses the x,z params.
	MapItem* AddLightItem( int x, int z, int rotation, int itemDefIndex, int hp, int flags );

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
	virtual void BeginRender();
	virtual void EndRender();
	virtual void BeginRenderState( const void* renderState );
	virtual void BeginTexture( const void* textureHandle );
	virtual void Render( const void* renderState, const void* textureHandle, int nIndex, const int16_t* index, int nVertex, const gamui::Gamui::Vertex* vertex );

	enum ConnectionType {
		PATH_TYPE,
		VISIBILITY_TYPE
	};
	// visibility (but similar to AdjacentCost conceptually). If PATH_TYPE is
	// passed in for the connection, it becomes CanWalk
	bool CanSee( const grinliz::Vector2I& p, const grinliz::Vector2I& q, ConnectionType connection=VISIBILITY_TYPE );

	bool OpenDoor( int x, int y, bool open );
	void QueryAllDoors( CDynArray< grinliz::Vector2I >* doors );

	void Save( TiXmlElement* parent );
	void Load( const TiXmlElement* mapNode, ItemDef* const* arr );

	// Gets a starting location for a unit on the map.
	// TERRAN_TEAM - from the lander
	// ALIEN_TEAM, guard or scout - on the map metadata
	//
	void PopLocation( int team, bool guard, grinliz::Vector2I* pos, float* rotation );

	static void MapObjectToWorld( int x, int y, int rotation, Matrix2I* mat, grinliz::Vector3F* model );
	static void MapImageToWorld( int x, int y, int w, int h, int tileRotation, Matrix2I* mat );

	gamui::Gamui	overlay0;
	gamui::Gamui	overlay1;

private:

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
			GLASSERT( depth >=0 && depth < QUAD_DEPTH );
			GLASSERT( x>=0 && x < Map::SIZE );
			return x >> (LOG2_SIZE-depth); 
		}
		int NodeToWorld( int x0, int depth )
		{
			GLASSERT( x0>=0 && x0 < (1<<depth) );
			return x0 << (LOG2_SIZE-depth);			
		}
		int NodeOffset( int x0, int y0, int depth )	
		{	
			GLASSERT( x0>=0 && x0 < (1<<depth) );
			GLASSERT( y0>=0 && y0 < (1<<depth) );
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
	bool Connected( ConnectionType c, int x, int y, int dir );

	void StateToVec( const void* state, grinliz::Vector2<S16>* vec ) { *vec = *((grinliz::Vector2<S16>*)&state); }
	void* VecToState( const grinliz::Vector2<S16>& vec )			 { return (void*)(*(int*)&vec); }

	void ClearVisPathMap( grinliz::Rectangle2I& bounds );
	void CalcVisPathMap( grinliz::Rectangle2I& bounds );

	void DeleteItem( MapItem* item );

	grinliz::Random random;
	bool dayTime;
	IPathBlocker* pathBlocker;
	int width, height;
	grinliz::Rectangle3F bounds;
	SpaceTree* tree;

	Texture* backgroundTexture;		// background texture
	Surface backgroundSurface;		// background surface
	Vertex vertex[4];
	grinliz::Vector2F texture1[4];

	enum { MAX_IMAGE_DATA = 16 };
	struct ImageData {
		int x, y, size, tileRotation;
		grinliz::CStr<EL_FILE_STRING_LEN> name;
	};
	int nImageData;
	ImageData imageData[ MAX_IMAGE_DATA ];

	void GenerateLightMap();

	Surface lightMap[2];
	Texture* lightMapTex;

	Surface fowSurface;
	Texture* fowTex;

	Surface dayMap, nightMap;
	grinliz::Rectangle2I invalidLightMap;
	grinliz::BitArray<Map::SIZE, Map::SIZE, 1> fogOfWar;
	Surface lightObject;

	U32 pathQueryID;
	U32 visibilityQueryID;

	micropather::MicroPather* microPather;

	struct Debris {
		int x, y;
		Storage* storage;
		Model* crate;
	};
	CDynArray< Debris > debris;
	void SaveDebris( const Debris& d, TiXmlElement* parent );
	void LoadDebris( const TiXmlElement* mapNode, ItemDef* const* arr );

	// U8:
	// bits 0-6:	sub-turns remaining (0-127)		(0x7F)
	// bit    7:	set: fire, clear: smoke			(0x80)
	U8 pyro[SIZE*SIZE];

	int PyroOn( int x, int y ) const		{ return pyro[y*SIZE+x]; }
	int PyroFire( int x, int y ) const		{ return pyro[y*SIZE+x] & 0x80; }
	bool PyroSmoke( int x, int y ) const	{ return PyroOn( x, y ) && !PyroFire( x, y ); }
	int PyroDuration( int x, int y ) const	{ return pyro[y*SIZE+x] & 0x7F; }
	void SetPyro( int x, int y, int duration, int fire );

	grinliz::BitArray<SIZE, SIZE, 1>	pathBlock;	// spaces the pather can't use (units are there)	

	MP_VECTOR<void*>					mapPath;
	MP_VECTOR< micropather::StateCost > stateCostArr;

	gamui::TiledImage<MAX_TU*2+1, MAX_TU*2+1>	walkingMap;
	gamui::Image								border[	4];

	U8									visMap[SIZE*SIZE];
	U8									pathMap[SIZE*SIZE];

	grinliz::MemoryPool					itemPool;
	QuadTree							quadTree;
	MapItemDef							itemDefArr[MAX_ITEM_DEF];
	CStringMap< MapItemDef* >			itemDefMap;

	enum { MAX_GUARD_SCOUT = 24 };
	int nGuardPos;
	int nScoutPos;
	int nLanderPos;
	const MapItem* lander;
	grinliz::Vector2I					guardPos[MAX_GUARD_SCOUT];
	grinliz::Vector2I					scoutPos[MAX_GUARD_SCOUT];
};

#endif // UFOATTACK_MAP_INCLUDED
