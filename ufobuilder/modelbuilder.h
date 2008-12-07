#ifndef UFOATTACK_MODELBUILDER_INCLUDED		
#define UFOATTACK_MODELBUILDER_INCLUDED

#include "../engine/vertex.h"
#include "../engine/enginelimits.h"

struct VertexGroup {
	enum {
		NAME_BYTES = 16,
	};
	VertexGroup() : nVertex( 0 ), nIndex( 0 ) { textureName[0] = 0; }

	char textureName[EL_FILE_STRING_LEN];
	VertexX	vertex[EL_MAX_VERTEX_IN_GROUP];
	grinliz::Vector3F normalSum[EL_MAX_VERTEX_IN_GROUP];
	int nVertex;
	U16 index[EL_MAX_INDEX_IN_GROUP];
	int nIndex;
};


class ModelBuilder {
public:
	ModelBuilder() : current( 0 ), nGroup( 0 )	{}

	void SetMatrix( const grinliz::Matrix4& mat )		{ matrix = mat; }

	// Set the current texture. Can be empty string. Must be called before AddTri.
	void SetTexture( const char* textureName );

	// Add the tri for the current texture.
	void AddTri( const Vertex& v0, const Vertex& v1, const Vertex& v2 );

	// Finishes adding triangle, runs the optimizer, calculates normals.
	void Flush();

	const VertexGroup* Groups()		{ return group; }
	int NumGroups()					{ return nGroup; }

private:
	float MemoryACMR( const U16* index, int nIndex );
	void ReOrderVertices( VertexGroup* group );

	VertexGroup* current;
	grinliz::Matrix4 matrix;
	VertexGroup group[EL_MAX_MODEL_GROUPS];

	/*
	VertexX	targetVertex[EL_MAX_VERTEX_IN_GROUP];
	U16 targetIndex[EL_MAX_INDEX_IN_GROUP];
	int indexMap[EL_MAX_INDEX_IN_GROUP];
	*/
	int nGroup;
};


#endif // UFOATTACK_MODELBUILDER_INCLUDED