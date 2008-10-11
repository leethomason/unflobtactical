#ifndef UFOATTACK_MODELBUILDER_INCLUDED		
#define UFOATTACK_MODELBUILDER_INCLUDED

#include <vector>
#include "../engine/vertex.h"

struct VertexGroup {
	enum {
		NAME_BYTES = 16
	};
	VertexGroup() { memset( textureName, 0, 16 ); }

	char textureName[NAME_BYTES];
	std::vector< Vertex > vertex;
	std::vector< int > index;
};


class ModelBuilder {
public:
	ModelBuilder() : group( 0 )	{}

	void SetMatrix( const grinliz::Matrix4& mat )		{ matrix = mat; }
	void SetTexture( const char* textureName );

	void AddTri( const grinliz::Vector3F* vertices, 
				 const grinliz::Vector2F* uv,
				 const grinliz::Vector3F& normal );
	void AddTri( const Vertex& v0, const Vertex& v1, const Vertex& v2 );

	void Optimize();
	const std::vector< VertexGroup >& GetGroups()	{ return group; }

private:
	VertexGroup* current;
	grinliz::Matrix4 matrix;
	std::vector< VertexGroup > group;
};


#endif // UFOATTACK_MODELBUILDER_INCLUDED