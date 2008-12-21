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
	U32 nGroups;
	Vector3X bounds[2];

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
	Model( ModelResource* resource=0 )		{	Init( resource ); }
	~Model()								{}

	void Init( ModelResource* resource ) 
	{
		this->resource = resource; 
		pos.Set( 0.0f, 0.0f, 0.0f ); 
		rot = 0.0f;
	}
	bool ShouldDraw()	{
		return resource && !next;
	}
	void Draw( bool useTexture = true );
	
	void SetPos( const grinliz::Vector3F& pos )	{ this->pos = pos; }
	void SetPos( float x, float y, float z )	{ grinliz::Vector3F vec = { x, y, z }; SetPos( vec ); }
	void SetYRotation( float rot )				{ this->rot = rot; }
	const float GetYRotation()					{ return rot; }

	void Link( Model* afterThis ) {
		prev = afterThis;
		next = afterThis->next;
		afterThis->next->prev = this;
		afterThis->next = this;
	}
	void UnLink() {
		GLASSERT( next );
		GLASSERT( prev );
		prev->next = next;
		next->prev = prev;
		prev = next = 0;
	}

public:
	Model* next;
	Model* prev;
private:
	ModelResource* resource;
	grinliz::Vector3F pos;
	float rot;
};


#endif // UFOATTACK_MODEL_INCLUDED