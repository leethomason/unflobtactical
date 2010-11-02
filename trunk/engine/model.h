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

#ifndef UFOATTACK_MODEL_INCLUDED
#define UFOATTACK_MODEL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glgeometry.h"
#include "../shared/glmap.h"
#include "../shared/gamedbreader.h"
#include "vertex.h"
#include "enginelimits.h"
#include "serialize.h"
#include "ufoutil.h"
#include "gpustatemanager.h"

class Texture;
class SpaceTree;
class RenderQueue;
class GPUShader;

// The smallest draw unit: texture, vertex, index.
struct ModelAtom 
{
	Texture* texture;
#ifdef EL_USE_VBO
	mutable GPUVertexBuffer vertexBuffer;		// created on demand, hence 'mutable'
	mutable GPUIndexBuffer  indexBuffer;
#endif
	U32 nVertex;
	U32 nIndex;

	//enum {
	//	BIND_VERTEX		= 0x01,
	//	BIND_NORMAL		= 0x02,
	//	BIND_TEXTURE_0	= 0x04
	//};
	void Bind( GPUShader* shader ) const;
	void BindPlanarShadow( GPUShader* shader ) const;	// I gave up trying to make this general.
	void LowerBind( GPUShader* shader, const GPUShader::Stream& stream ) const;

	// A note on the memory model: the index and vertices are stored
	// in continuous memory to cut down on allocation overhead. But
	// that is an allocation trick. The 'index' and 'vertex' pointers
	// work as if the buffers we allocated normally.
	//
	const U16* index;		// points back to ModelResource memory.
	const Vertex* vertex;	// points back to ModelResource memory.
};


class ModelResource
{
public:
	ModelResource()		{ memset( &header, 0, sizeof( header ) ); }
	~ModelResource()	{ Free(); }

	void Free();
	void DeviceLoss();

	// In the coordinate space of the resource! (Object space.)
	int Intersect(	const grinliz::Vector3F& point,
					const grinliz::Vector3F& dir,
					grinliz::Vector3F* intersect ) const;


	ModelHeader header;						// loaded

	grinliz::Rectangle3F	hitBounds;		// for picking - a bounds approximation
	U16*					allIndex;		// memory store for vertices and indices. Used for hit-testing.
	Vertex*					allVertex;

	const grinliz::Rectangle3F& AABB() const	{ return header.bounds; }

	ModelAtom atom[EL_MAX_MODEL_GROUPS];
};


class ModelResourceManager
{
public:
	static ModelResourceManager* Instance()	{ GLASSERT( instance ); return instance; }
	
	void  AddModelResource( ModelResource* res );
	const ModelResource* GetModelResource( const char* name, bool errorIfNotFound=true );
	void DeviceLoss();

	static void Create();
	static void Destroy();
private:
	enum { 
		MAX_MODELS = 100	// just pointers
	};
	ModelResourceManager();
	~ModelResourceManager();

	static ModelResourceManager* instance;
	CArray< ModelResource*, MAX_MODELS > modelResArr;
	CStringMap<	ModelResource* > map;
};


class ModelLoader
{
public:
	ModelLoader() 	{}
	~ModelLoader()	{}

	void Load( const gamedb::Item*, ModelResource* res );

private:
};


class Model
{
public:
	Model();
	~Model();

	void Init( const ModelResource* resource, SpaceTree* tree );
	void Free();

	void Queue( RenderQueue* queue, GPUShader* opaque, GPUShader* transparent );

	enum {
		MODEL_SELECTABLE			= 0x01,
		MODEL_HIDDEN_FROM_TREE		= 0x02,
		MODEL_OWNED_BY_MAP			= 0x04,
		MODEL_NO_SHADOW				= 0x08,
		MODEL_INVISIBLE				= 0x10,
		MODEL_MAP_TRANSPARENT		= 0x20,
		MODEL_METADATA				= 0x80,		// mapmaker data that isn't displayed in-game

		// RESERVED!
		// MODEL_local				= 0x00010000 and higher.
	};

	int IsFlagSet( int f ) const	{ return flags & f; }
	void SetFlag( int f )			{ flags |= f; }
	void ClearFlag( int f )			{ flags &= (~f); }
	int Flags()	const				{ return flags; }

	const grinliz::Vector3F& Pos() const			{ return pos; }
	void SetPos( const grinliz::Vector3F& pos );
	void SetPos( float x, float y, float z )		{ grinliz::Vector3F vec = { x, y, z }; SetPos( vec ); }
	float X() const { return pos.x; }
	float Y() const { return pos.y; }
	float Z() const { return pos.z; }

	void SetRotation( float rot, int axis=1 );
	float GetRotation( int axis=1 ) const			{ return rot[axis]; }

	int IsBillboard() const 		{ return resource->header.flags & ModelHeader::BILLBOARD; }
	int IsShadowRotated() const		{ return resource->header.flags & ModelHeader::ROTATE_SHADOWS; }
	
	// Set the skin texture (which is a special texture xform)
	void SetSkin( int armor, int skin, int hair );
	// Set the texture xform for rendering tricks
	void SetTexXForm( float a=1.0f, float d=1.0f, float x=0.0f, float y=0.0f );

	// Set the texture - overrides all textures
	void SetTexture( Texture* t );

	// AABB for user selection (bigger than the true AABB)
	void CalcHitAABB( grinliz::Rectangle3F* aabb ) const;

	// The bounding box
	const grinliz::Rectangle3F& AABB() const;

	void CalcTrigger( grinliz::Vector3F* trigger ) const;
	void CalcTarget( grinliz::Vector3F* target ) const;

	// Returns grinliz::INTERSECT or grinliz::REJECT
	int IntersectRay(	const grinliz::Vector3F& origin, 
						const grinliz::Vector3F& dir,
						grinliz::Vector3F* intersect ) const;

	const ModelResource* GetResource()			{ return resource; }
	bool Sentinel()	const						{ return resource==0 && tree==0; }

	Model* next;			// used by the SpaceTree query
	Model* next0;			// used by the Engine sub-sorting
	
	// Set by the engine. Any xform change will set this
	// to (-1,-1)-(-1,-1) to then be set by the engine.
	grinliz::Rectangle2I mapBoundsCache;

	const grinliz::Matrix4& XForm() const;
	bool HasTextureXForm() const					{ return texMatSet; }
	const grinliz::Matrix4& TextureXForm() const	{ return texMat; }

private:
	void Modify() { xformValid = false; invValid = false; mapBoundsCache.Set( -1, -1, -1, -1 ); }
	const grinliz::Matrix4& InvXForm() const;

	SpaceTree* tree;
	const ModelResource* resource;
	grinliz::Vector3F pos;
	float rot[3];
	bool texMatSet;
	grinliz::Matrix4 texMat;

	Texture* setTexture;	// overrides the default texture
	ModelAtom* textureAtoms;		// if the texture is set, a copy of the atoms (that use the texture instead)

	int flags;
	
	mutable bool xformValid;
	mutable bool invValid;

	mutable grinliz::Rectangle3F aabb;
	mutable grinliz::Matrix4 _xform;
	mutable grinliz::Matrix4 _invXForm;
};


#endif // UFOATTACK_MODEL_INCLUDED