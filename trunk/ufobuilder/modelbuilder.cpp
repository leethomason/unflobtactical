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

#include "modelbuilder.h"
#include "../engine/vertex.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

void ModelBuilder::SetTexture( const char* textureName )
{
	GLASSERT( strlen( textureName ) < EL_FILE_STRING_LEN );
	current = 0;

	for( int i=0; i<nGroup; ++i ) {
		if ( strcmp( textureName, group[i].textureName ) == 0 ) {
			current = i;
			break;
		}
	}
	if ( !current ) {
		GLASSERT( nGroup < EL_MAX_MODEL_GROUPS );

		StrNCpy( group[nGroup].textureName, textureName, EL_FILE_STRING_LEN );
		current = nGroup++;
	}
}


void ModelBuilder::AddTri( const Vertex& v0, const Vertex& v1, const Vertex& v2 )
{
	GLASSERT( current<nGroup );

	Vertex vIn[3] = { v0, v1, v2 };
	Vertex vX[3];

	// Triangles are added to the VertexStream(s). They are later sorted
	// and processed to the VertexGroup(s).
	GLASSERT( stream[current].nVertex+3 <= VertexStream::MAX_VERTEX );

	// Transform the vertex into the current coordinate space.
	for( int i=0; i<3; ++i ) {
		// pos
		MultMatrix4( matrix, vIn[i].pos, &vX[i].pos, 1.0f );
		// normal
		vIn[i].normal.Normalize();
		MultMatrix4( matrix, vIn[i].normal, &vX[i].normal, 0.0f );
		// tex
		vX[i].tex = vIn[i].tex;

		// store for processing
		stream[current].vertex[ stream[current].nVertex+i ] = vX[i];
		stream[current].normalProcessed[ stream[current].nVertex+i ] = false;
	}
	stream[current].nVertex += 3;
}


void ModelBuilder::Flush()
{
	const float EPS_VERTEX = 0.005f;

	// Get rid of empty groups.
	for( int i=0; i<nGroup; /*nothing*/ ) {
		if ( stream[i].nVertex == 0 ) {
			for( int k=i; k<nGroup-1; ++k ) {
				stream[k] = stream[k+1];
				group[k] = group[k+1];
			}
			--nGroup;
		}
		else {
			++i;
		}
	}

	// Smoothing: average normals. If smoothing, everything with the same
	// position should have the same normal.
	if ( smooth ) {
		std::vector<int> match;

		for( int g=0; g<nGroup; ++g ) {
			for( int i=0; i<stream[g].nVertex; ++i ) {
				if ( stream[g].normalProcessed[i] )
					continue;	// already picked up by earlier pass.

				match.clear();
				match.push_back( i );
				Vector3F normal = stream[g].vertex[i].normal;

				for( int j=i+1; j<stream[g].nVertex; ++j ) {
					// Equal, in this case, is only the position.
					if ( !stream[g].normalProcessed[j] ) {
						if ( stream[g].vertex[i].pos.Equal( stream[g].vertex[j].pos, EPS_VERTEX ) ) {
							match.push_back( j );
							normal += stream[g].vertex[j].normal;
						}
					}
				}

				if ( normal.LengthSquared() < 0.001f ) {
					// crap.
					normal.Set( 0.0f, 1.0f, 0.0f );
				}
				normal.Normalize();
				for( unsigned j=0; j<match.size(); ++j ) {
					stream[g].vertex[match[j]].normal = normal;
					stream[g].normalProcessed[match[j]] = true;
				}
			}
		}
	}
	
	// Now we fetch vertices. This moves the vertex from the 'stream' to the 'group'
	for( int g=0; g<nGroup; ++g ) {
		for( int i=0; i<stream[g].nVertex; ++i ) {
			GLASSERT( stream[g].nVertex % 3 == 0 );
			
			// Is the vertex we need already in the group?
			bool added = false; 

			for( int j=0; j<group[g].nVertex; ++j ) {
				// All pos, normal, and tex must match...but if we were smoothed,
				// then the normals may have multiple possible matches.
				if ( group[g].vertex[j].Equal( stream[g].vertex[i], EPS_VERTEX ) ) {
					group[g].index[ group[g].nIndex++ ] = j;
					added = true;
					break;
				}
			}
			if ( !added ) {
				group[g].vertex[ group[g].nVertex ] = stream[g].vertex[i];
				group[g].index[ group[g].nIndex++ ] = group[g].nVertex++;
			}
			GLASSERT( group[g].nVertex < EL_MAX_VERTEX_IN_GROUP );
			GLASSERT( group[g].nIndex < EL_MAX_INDEX_IN_GROUP );
		}
		GLASSERT( group[g].nIndex % 3 == 0 );
	}

	bounds.min = bounds.max = group[0].vertex[0].pos;
	for( int i=0; i<nGroup; ++i ) {
		for( int j=0; j<group[i].nVertex; ++j ) {
			bounds.min.x = Min( bounds.min.x, group[i].vertex[j].pos.x );
			bounds.min.y = Min( bounds.min.y, group[i].vertex[j].pos.y );
			bounds.min.z = Min( bounds.min.z, group[i].vertex[j].pos.z );

			bounds.max.x = Max( bounds.max.x, group[i].vertex[j].pos.x );
			bounds.max.y = Max( bounds.max.y, group[i].vertex[j].pos.y );
			bounds.max.z = Max( bounds.max.z, group[i].vertex[j].pos.z );
		}
	}
}
