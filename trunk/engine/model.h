#ifndef UFOATTACK_MODEL_INCLUDED
#define UFOATTACK_MODEL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/gltypes.h"
#include "vertex.h"
#include "enginelimits.h"

class Texture;
class SpaceTree;
class RenderQueue;

struct ModelAtom 
{
	const Texture* texture;
	U32 vertexID;
	U32 indexID;
	U32 nVertex;
	U32 nIndex;

	void Bind() const;
	void Draw() const;
};


class ModelResource
{
public:
	char name[16];			// loaded
	U32 nGroups;			// loaded
	Vector3X bounds[2];		// loaded

	SphereX boundSphere;	// computed
	Rectangle3X hitBounds;	// for picking - a bounds approximation

	ModelAtom atom[EL_MAX_MODEL_GROUPS];
};

class ModelLoader
{
public:
	ModelLoader( const Texture* texture, int nTextures ) 	{
		this->texture = texture;
		this->nTextures = nTextures;
	}
	~ModelLoader()		{}

	void Load( FILE* fp, ModelResource* res );

private:
	const Texture* texture;
	int nTextures;

	U16		index[EL_MAX_INDEX_IN_MODEL];
	VertexX	vertex[EL_MAX_VERTEX_IN_MODEL];
};


class Model
{
public:
	Model()		{	Init( 0, 0 ); }
	~Model()	{}

	void Init( ModelResource* resource, SpaceTree* tree );
	// Draws the model, and sets texture
	void Draw( bool useTexture = true );
	// Queued rendering
	void Queue( RenderQueue* queue, bool useTexture );

	// Used by the queued rendering system:
	void PushMatrix() const;
	void PopMatrix() const;

	bool IsDraggable()	{ return isDraggable; }
	void SetDraggable( bool drag )	{ isDraggable = drag; }
	bool IsHiddenFromTree()			{ return hiddenFromTree; }
	void HideFromTree( bool hide )  { hiddenFromTree = hide; }
	
	const Vector3X& Pos()						{ return pos; }
	void SetPos( const Vector3X& pos );
	void SetPos( float x, float y, float z )	{ Vector3X vec = { grinliz::Fixed(x), grinliz::Fixed(y), grinliz::Fixed(z) }; SetPos( vec ); }
	void SetPos( grinliz::Fixed x, grinliz::Fixed y, grinliz::Fixed z )	{ Vector3X vec = { grinliz::Fixed(x), grinliz::Fixed(y), grinliz::Fixed(z) }; SetPos( vec ); }

	void SetYRotation( grinliz::Fixed rot )		{ this->rot = rot; }
	void SetYRotation( float rot )				{ this->rot = rot; }
	const grinliz::Fixed GetYRotation()			{ return rot; }

	void SetSkin( int armor, int skin, int hair );

	void CalcBoundSphere( SphereX* spherex );
	void CalcHitAABB( Rectangle3X* aabb );

	ModelResource* GetResource()				{ return resource; }
	bool Sentinel()								{ return resource==0 && tree==0; }

	Model* next;

private:
	SpaceTree* tree;
	ModelResource* resource;
	Vector3X pos;
	grinliz::Fixed rot;
	grinliz::Fixed textureOffsetX;

	bool isDraggable;
	bool hiddenFromTree;
};


#endif // UFOATTACK_MODEL_INCLUDED