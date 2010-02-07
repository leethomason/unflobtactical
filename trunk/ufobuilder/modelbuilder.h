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

#ifndef UFOATTACK_MODELBUILDER_INCLUDED		
#define UFOATTACK_MODELBUILDER_INCLUDED

#include "../engine/vertex.h"
#include "../engine/enginelimits.h"
#include "../grinliz/glrectangle.h"

struct VertexGroup {
	VertexGroup() : nVertex( 0 ), nIndex( 0 ) { textureName[0] = 0; }

	char textureName[EL_FILE_STRING_LEN];

	int nVertex;
	Vertex	vertex[EL_MAX_VERTEX_IN_GROUP];
	int nIndex;
	U16 index[EL_MAX_INDEX_IN_GROUP];
};


struct VertexStream {
	VertexStream() : nVertex( 0 )	{}

	enum { MAX_VERTEX = EL_MAX_VERTEX_IN_GROUP*4 };
	int				nVertex;
	Vertex			vertex[ MAX_VERTEX ];
	bool			normalProcessed[ MAX_VERTEX ];
};


class ModelBuilder {
public:
	ModelBuilder() : current( 0 ), nGroup( 0 ), smooth( false )	{}

	void SetMatrix( const grinliz::Matrix4& mat )		{ matrix = mat; }

	// Set the current texture. Can be empty string. Must be called before AddTri.
	void SetTexture( const char* textureName );

	// Set smooth shading, generally true for characters and false for buildings.
	void SetShading( bool smooth )	{ this->smooth = smooth; }

	// Add the tri for the current texture.
	void AddTri( const Vertex& v0, const Vertex& v1, const Vertex& v2 );

	// Finishes adding triangle, runs the optimizer, calculates normals.
	void Flush();

	const VertexGroup* Groups()		{ return group; }
	int NumGroups()					{ return nGroup; }

	const grinliz::Rectangle3F& Bounds()	{ return bounds; }

private:
	int current;

	grinliz::Matrix4 matrix;
	grinliz::Rectangle3F bounds;

	VertexGroup group[EL_MAX_MODEL_GROUPS];
	VertexStream stream[EL_MAX_MODEL_GROUPS];

	int nGroup;
	bool smooth;
};


#endif // UFOATTACK_MODELBUILDER_INCLUDED