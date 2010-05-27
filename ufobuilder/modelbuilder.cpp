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

void ModelBuilder::SetTexture( const char* _textureName )
{
	CStr< EL_FILE_STRING_LEN >  textureName( _textureName );
	current = 0;

	for( int i=0; i<nGroup; ++i ) {
		if (    ( textureName.empty() && group[i].textureName.empty() )
			 || ( textureName == group[i].textureName ) )
		{
			current = i;
			break;
		}
	}
	if ( !current ) {
		group[nGroup].textureName = textureName;
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
	polyCulled = 0;

	if ( polyRemoval == POLY_PRE ) {
		const float GROUND = 0.01f;
		const Vector3F DOWN = { 0.0f, -1.0f, 0.0f };
		const float EPS = 0.95f;

		for( int g=0; g<nGroup; ++g ) {
			int w = 0;	//write
			GLASSERT( stream[g].nVertex % 3 == 0 );
			for( int r=0; r<stream[g].nVertex; r+=3 ) {
				if (    stream[g].vertex[r+0].pos.y <= GROUND
					 && stream[g].vertex[r+1].pos.y <= GROUND
					 && stream[g].vertex[r+2].pos.y <= GROUND ) 
				{
					Vector3F a = stream[g].vertex[r+1].pos - stream[g].vertex[r+0].pos;
					Vector3F b = stream[g].vertex[r+2].pos - stream[g].vertex[r+0].pos;
					Vector3F c;
					CrossProduct( a, b, &c );
					c.Normalize();
					if ( DotProduct( c, DOWN ) > EPS ) {
						// Thow away this down facing polygon on the ground.
						++polyCulled;
						continue;	
					}
				}
				stream[g].vertex[w+0] = stream[g].vertex[r+0];
				stream[g].vertex[w+1] = stream[g].vertex[r+1];
				stream[g].vertex[w+2] = stream[g].vertex[r+2];
				w += 3;
			}
			stream[g].nVertex = w;
		}
	}

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

	// Smoothing: average normals.
	// SMOOTH: everything with the same position should have the same normal.
	// CREASE: all the normals that are "close" are averaged together
	if ( smooth ) {
		std::vector<int> match;

		for( int g=0; g<nGroup; ++g ) {
			for( int i=0; i<stream[g].nVertex; ++i ) {
				if ( stream[g].normalProcessed[i] )
					continue;	// already picked up by earlier pass.

				match.clear();
				match.push_back( i );
				Vector3F normalSum = stream[g].vertex[i].normal;
				Vector3F normal = stream[g].vertex[i].normal;

				for( int j=i+1; j<stream[g].nVertex; ++j ) {
					// Equal, in this case, is only the position.
					if ( !stream[g].normalProcessed[j] ) {

						if ( smooth == SMOOTH ) {
							if ( stream[g].vertex[i].pos.Equal( stream[g].vertex[j].pos, EPS_VERTEX ) ) {
								match.push_back( j );
								normalSum += stream[g].vertex[j].normal;
							}
						}
						else {	// CREASE
							if (    stream[g].vertex[i].pos.Equal( stream[g].vertex[j].pos, EPS_VERTEX )
								 && DotProduct( normal, stream[g].vertex[j].normal ) > 0.7f )
							{
								match.push_back( j );
								normalSum += stream[g].vertex[j].normal;
							}
						}
					}
				}

				if ( normalSum.LengthSquared() < 0.001f ) {
					// crap.
					normalSum.Set( 0.0f, 1.0f, 0.0f );
				}
				normalSum.Normalize();
				for( unsigned j=0; j<match.size(); ++j ) {
					stream[g].vertex[match[j]].normal = normalSum;
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
