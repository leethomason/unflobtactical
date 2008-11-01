#ifndef UFOATTACK_MODEL_INCLUDED
#define UFOATTACK_MODEL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/gltypes.h"
#include "vertex.h"
#include "enginelimits.h"

class Texture;

class ModelResource
{
public:
	char name[16];
	U32 dataID;
	U32 indexID;

	U32 nGroups;
	const Texture* texture[EL_MAX_MODEL_GROUPS];
	U32 startIndex[EL_MAX_MODEL_GROUPS];
	U32 nIndex[EL_MAX_MODEL_GROUPS];
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
	Vertex	vertex[EL_MAX_VERTEX_IN_MODEL];
};


class Model
{
public:
	Model( ModelResource* resource=0 )		{	Init( resource ); }
	~Model()								{}

	void Init( ModelResource* resource ) 
	{
		this->resource = resource; 
		pos.Set( 0.0f, 0.0f, 0.0f ); 
		rot = 0.0f;
	}
	void Draw();
	
	void SetPos( const grinliz::Vector3F& pos )	{ this->pos = pos; }
	void SetRotationY( float rot )				{ this->rot = rot; }

public:
	Model* next;
	Model* prev;
private:
	ModelResource* resource;
	grinliz::Vector3F pos;
	float rot;
};


#endif // UFOATTACK_MODEL_INCLUDED