#ifndef UFOATTACK_MODELBUILDER_INCLUDED		
#define UFOATTACK_MODELBUILDER_INCLUDED

#include "../engine/vertex.h"


struct VertexGroup {
	enum {
		NAME_BYTES = 16,
		MAX_VERTEX = 4096,
		MAX_INDEX  = 4096
	};
	VertexGroup() : nVertex( 0 ), nIndex( 0 ) { textureName[0] = 0; }

	char textureName[NAME_BYTES];
	Vertex vertex[MAX_VERTEX];
	int nVertex;
	U16 index[MAX_INDEX];
	int nIndex;
};


class ModelBuilder {
public:
	enum {
		MAX_GROUP = 4,
		SCAN_BACK = 64
	};
	ModelBuilder() : current( 0 ), nGroup( 0 )	{}

	void SetMatrix( const grinliz::Matrix4& mat )		{ matrix = mat; }
	void SetTexture( const char* textureName );

	void AddTri( const Vertex& v0, const Vertex& v1, const Vertex& v2 );

	void Optimize();
	const VertexGroup* Groups()		{ return group; }
	int NumGroups()					{ return nGroup; }

private:
	VertexGroup* current;
	grinliz::Matrix4 matrix;
	VertexGroup group[MAX_GROUP];
	int nGroup;
};


#endif // UFOATTACK_MODELBUILDER_INCLUDED