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

#include "../shared/glmap.h"

#include "../gamui/gamui.h"

#include "vertex.h"
#include "enginelimits.h"
#include "serialize.h"
#include "ufoutil.h"
#include "surface.h"
#include "texture.h"
#include "gpustatemanager.h"

class Model;
class ModelResource;
class SpaceTree;
class RenderQueue;
class ParticleSystem;
class TiXmlElement;
class Map;


class IPathBlocker
{
public:
	virtual void MakePathBlockCurrent( Map* map, const void* user ) = 0;
};


// some strange android bug - the size of the structure gets mangled
// by the compiler if all the fields weren't 32 bits
struct MapItemDef 
{
	enum {
		OBSCURES = 0x01,
		EXPLODES = 0x02,
		ROTATES  = 0x04,	// when destroyed, randomly rotate
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

	const char* Name() const { return resource; }
	const ModelResource* GetModelResource() const;
	const ModelResource* GetOpenResource() const;
	const ModelResource* GetDestroyedResource() const;

	U32 Pather( int x, int y ) const {
		GLRELASSERT( x >= 0 && y >= 0 && x < cx && y < cy );
		
		if ( !patherStr )
			return 0;
		
		GLASSERT( (int)strlen( patherStr ) == cx*cy );
		GLASSERT( (int)strlen( visibilityStr ) == cx*cy );

		const char c = *(patherStr + y*cx + x );
		return grinliz::HexLowerCharToInt( c );
	}

	U32 Visibility( int x, int y ) const 
	{
		GLRELASSERT( x < cx && y < cy );
		if ( !visibilityStr )
			return 0;

		GLASSERT( (int)strlen( patherStr ) == cx*cy );
		GLASSERT( (int)strlen( visibilityStr ) == cx*cy );

		const char c = *(visibilityStr + y*cx + x );
		return grinliz::HexLowerCharToInt( c );
	}

	// return true if the object can take damage
	bool CanDamage() const	{ return hp != 0xffff; }
	bool IsDoor() const		{ return resourceOpen != 0; }

	grinliz::Rectangle2I Bounds() const 
	{
		return grinliz::Rectangle2I( 0, 0, cx, cy );
	}
};


struct MapDamageDesc
{
	float damage;		// damage done, in hp
	float incendiary;	// damage done by flame ( <= hp )
};


// Map is crazy, crazy heavy weight. Also possible to use
// just the IMap for minimal function. Yes, this is a 
// factoring problem.
class IMap
{
public:
	virtual Texture* LightFogMapTexture() = 0;
	virtual void LightFogMapParam( float* w, float* h, float* dx, float* dy ) = 0;
};


class Map : public IMap,
			public micropather::Graph,
			public ITextureCreator,
			public gamui::IGamuiRenderer
{
public:
	enum {
		SIZE = EL_MAP_SIZE,
		LOG2_SIZE = 6,
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
		U8			modelRotation;
		U8			amountObscuring;		// if !0, amount to decrement 
		U16			hp;
		U16			flags;
		MatPacked	xform;			// xform in map coordinates
		grinliz::Rectangle2<U8> mapBounds8;
		float		modelX, modelZ;
		
		const MapItemDef* def;
		Model*	model;
		
		MapItem* next;			// the 'next' after a query
		MapItem* nextQuad;		// next pointer in the quadTree

		grinliz::Rectangle2I MapBounds() const 
		{	
			return grinliz::Rectangle2I( mapBounds8.pos.x, mapBounds8.pos.y, 
										 mapBounds8.size.x, mapBounds8.size.y );
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
	grinliz::Rectangle2I Bounds() const	{ return grinliz::Rectangle2I( 0, 0, width, height ); }

/*	void ClipToMap( grinliz::Rectangle2I* b ) const		
	{
		grinliz::Rectangle2I cpy = *b;
		cpy.DoIntersection( Bounds() );
		*b = cpy;
	}*/

	virtual void SetSize( int w, int h );
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
	void DrawPastSeen( const grinliz::Color4F& );		//< draw the map that currently can't be seen

	void DrawPath( int mode );		//< debugging
	void DrawOverlay( int layer );		//< draw the "where can I walk" alpha overlay. Set up by ShowNearPath().

	// Do damage to a singe map object.
	void DoDamage( Model* m, const MapDamageDesc& damage, grinliz::Rectangle2I* destroyedBounds, grinliz::Vector2I* explosion  );
	// Do damage to an entire map tile.
	void DoDamage( int x, int y, const MapDamageDesc& damage, grinliz::Rectangle2I* destroyedBounds, grinliz::Vector2I* explosion );
	
	// Process a sub-turn: fire moves, smoke goes away, etc.
	void DoSubTurn( grinliz::Rectangle2I* changeBounds, float fireDamagePerSubTurn );

	// Smoke from weapons, explosions, effects, etc.
	void AddSmoke( int x, int y, int subturns );
	void AddFlare( int x, int y, int subturns );

	// Returns true if view obscured by smoke, fire, etc.
	bool Obscured( int x, int y ) const		{ return ( obscured[y*SIZE+x] || PyroSmoke( x, y ) ); }
	int  Flared( int x, int y ) const		{ return PyroFlare( x, y ); }
	void EmitParticles( U32 deltaTime );

	// Set the path block (does nothing if they are equal.)
	// Generally called by MakePathBlockCurrent
	void SetPathBlocks( const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>& block );

	virtual int GetNumItemDef() = 0;
	virtual const char* GetItemDefName( int i ) = 0;
	virtual const MapItemDef* GetItemDef( const char* name ) = 0;

	// MapMaker method to create a translucent preview.
	Model* CreatePreview( int x, int z, const MapItemDef* def, int rotation );

	// hp = -1 default
	//       0 destroyed
	//		1+ hp remaining
	MapItem* AddItem( int x, int z, int rotation, const MapItemDef* def, int hp, int flags );

	void DeleteAt( int x, int z );
	void MapBoundsOfModel( const Model* m, grinliz::Rectangle2I* mapBounds );

	void ResetPath();	// normally called automatically
	//void Clear();

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
						const grinliz::Vector2F* range,				// array of range colorings
						const grinliz::Vector2<S16>* dest );		// if not null, use a single destination, not all destinations
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
		RENDERSTATE_MAP_TRANSLUCENT,
		RENDERSTATE_MAP_MORE_TRANSLUCENT
	};
	virtual void BeginRender();
	virtual void EndRender();
	virtual void BeginRenderState( const void* renderState );
	virtual void BeginTexture( const void* textureHandle );
	virtual void Render( const void* renderState, const void* textureHandle, int nIndex, const uint16_t* index, int nVertex, const gamui::Gamui::Vertex* vertex );

	Texture* BackgroundTexture()	{ return backgroundTexture; }
	Texture* LightMapTexture()		{ return lightMapTex; }
	// IMap
	Texture* LightFogMapTexture()	{ return lightFogMapTex; }
	virtual void LightFogMapParam( float* w, float* h, float* dx, float* dy )	{ *w = (float)EL_MAP_SIZE; *h = (float)EL_MAP_SIZE; *dx = 0; *dy = 0; };

	enum ConnectionType {
		PATH_TYPE,
		VISIBILITY_TYPE
	};
	// visibility (but similar to AdjacentCost conceptually). If PATH_TYPE is
	// passed in for the connection, it becomes CanWalk
	bool CanSee( const grinliz::Vector2I& p, const grinliz::Vector2I& q, ConnectionType connection=VISIBILITY_TYPE );

	bool ProcessDoors( const grinliz::Vector2I* openers, int nOpeners );
	void SetPyro( int x, int y, int duration, bool fire, bool flare );

	void Save( FILE* fp, int depth );
	void Load( const TiXmlElement* mapNode );


	static void MapImageToWorld( int x, int y, int w, int h, int tileRotation, Matrix2I* mat );

	enum {
		LAYER_UNDER_LOW,		// obscurred by unseen and past-seen
		LAYER_UNDER_HIGH,		// covers unseen and past-seen
		LAYER_OVER,				// on top of models
		NUM_LAYERS
	};
	gamui::Gamui	overlay[NUM_LAYERS];
	grinliz::Random random;

protected:
	virtual void SubSave( FILE* fp, int depth ) = 0;
	virtual void SubLoad( const TiXmlElement* mapNode ) = 0;
	virtual void InitWalkingMapAtoms( gamui::RenderAtom* atoms, int nWalkingMaps ) = 0;	// 3 colors per walking map

	// 0,90,180,270 rotation
	static void XYRToWorld( int x, int y, int rotation, Matrix2I* mat );
	// 0,90,180,270 rotation
	static void WorldToXYR( const Matrix2I& mat, int *x, int *y, int* r, bool useRot0123 = false );
	static void WorldToModel( const Matrix2I& mat, bool billboard, grinliz::Vector3F* m );

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
			grinliz::Rectangle2I b( x, y, 1, 1 ); 
			return FindItems( b, required, excluded ); 
		}
		MapItem* FindItem( const Model* model );

		void UnlinkItem( MapItem* item );

		void MarkVisible( const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>& fogOfWar );

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

	SpaceTree*	tree;
	QuadTree	quadTree;

	CDynArray< grinliz::Vector2I >				guardPos;
	CDynArray< grinliz::Vector2I >				scoutPos;
	CDynArray< grinliz::Vector2I >				civPos;

private:


	int InvertPathMask( U32 m ) {
		U32 m0 = (m<<2) | (m>>2);
		return m0 & 0xf;
	}

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
	micropather::MPVector<void*> mpVector;

	// 0x80 fire bit		(128)
	// 0x40 flare bit		(64)
	// duration: 1->64
	int PyroOn( int x, int y ) const		{ return pyro[y*SIZE+x]; }
	int PyroFire( int x, int y ) const		{ return pyro[y*SIZE+x] & 0x80; }
	int PyroFlare( int x, int y ) const		{ return pyro[y*SIZE+x] & 0x40; }
	bool PyroSmoke( int x, int y ) const	{ int p = pyro[y*SIZE+x]; return ((p & 0xC0) == 0) && (p>0); }
	int PyroDuration( int x, int y ) const	{ return pyro[y*SIZE+x] & 0x3F; }

	void ChangeObscured( const grinliz::Rectangle2I& bounds, int delta );

	grinliz::BitArray<SIZE, SIZE, 1>			pathBlock;	// spaces the pather can't use (units are there)	

	MP_VECTOR<void*>							mapPath;
	MP_VECTOR< micropather::StateCost >			stateCostArr;

	CompositingShader							gamuiShader;
	enum {
		MAX_WALKING_MAPS = 2		// 1 or 2
	};
	gamui::TiledImage<EL_MAP_MAX_PATH*2+1, EL_MAP_MAX_PATH*2+1>	walkingMap[MAX_WALKING_MAPS];

	grinliz::MemoryPool							itemPool;
	int											nSeenIndex, nUnseenIndex, nPastSeenIndex;

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
