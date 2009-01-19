#ifndef UFOATTACK_MODEL_INCLUDED
#define UFOATTACK_MODEL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/gltypes.h"
#include "vertex.h"
#include "enginelimits.h"

class Texture;
class SpaceTree;

class ModelResource
{
public:
	char name[16];			// loaded
	U32 nGroups;			// loaded
	Vector3X bounds[2];		// loaded
	SphereX boundSphere;	// computed

	const Texture* texture[EL_MAX_MODEL_GROUPS];

	U32 vertexID[EL_MAX_MODEL_GROUPS];
	U32 indexID[EL_MAX_MODEL_GROUPS];

	U32 nIndex[EL_MAX_MODEL_GROUPS];
	U32 nVertex[EL_MAX_MODEL_GROUPS];
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
	void Draw( bool useTexture = true );
	
	void SetPos( const grinliz::Vector3F& pos );
	void SetPos( float x, float y, float z )	{ grinliz::Vector3F vec = { x, y, z }; SetPos( vec ); }
	void SetYRotation( float rot )				{ this->rot = rot; }
	const float GetYRotation()					{ return rot; }

	void CalcBoundSphere( SphereX* spherex );
	ModelResource* GetResource()				{ return resource; }
	bool Sentinel()								{ return resource==0 && tree==0; }

	Model* next;

private:
	SpaceTree* tree;
	ModelResource* resource;
	grinliz::Vector3F pos;
	float rot;
};


#endif // UFOATTACK_MODEL_INCLUDED