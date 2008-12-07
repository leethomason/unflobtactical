#include "import.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glgeometry.h"

using namespace grinliz;

bool ImportOFF( const std::string& filename, ModelBuilder* builder )
{
	FILE* fp = fopen( filename.c_str(), "r" );
	if ( !fp ) {
		return false;
	}

	builder->SetTexture( "" );

	char buf[256];
	std::vector< StrToken > tokens;
	
	int vertexCount = 0;
	int faceCount = 0;
	int edgeCount = 0;

	// Header.
	while( fgets( buf, 256, fp ) ) {
		if ( *buf == '#' || *buf == 0 ) {
			// comment, emtpy line
			continue;
		}
		std::string str( buf );
		StrTokenize( str, &tokens );

		if ( !tokens.size() ) {
			continue;
		}

		if ( tokens[0].type == StrToken::STRING && tokens[0].str == "OFF" ) {
			continue;
		}
		if ( vertexCount == 0 ) {
			GLASSERT( tokens.size() == 3 );
			GLASSERT( tokens[0].type == StrToken::NUMBER );
			GLASSERT( tokens[1].type == StrToken::NUMBER );
			GLASSERT( tokens[2].type == StrToken::NUMBER );
			vertexCount = (int) tokens[0].number;
			faceCount   = (int) tokens[1].number;
			edgeCount   = (int) tokens[2].number;
			break;
		}
	}

	std::vector< Vector3F > pos( vertexCount );

	// Vertices
	int i=0;
	while( fgets( buf, 256, fp ) && i<vertexCount ) {
		if ( *buf == '#' || *buf == 0 ) {
			// comment, emtpy line
			continue;
		}
		std::string str( buf );
		StrTokenize( str, &tokens );
		if ( !tokens.size() ) {
			continue;
		}
		GLASSERT( tokens.size() == 3 );
		GLASSERT( tokens[0].type == StrToken::NUMBER );
		GLASSERT( tokens[1].type == StrToken::NUMBER );
		GLASSERT( tokens[2].type == StrToken::NUMBER );

		// Note axis swizzle.
		pos[i].x = float( tokens[0].number );
		pos[i].y = float( tokens[2].number );
		pos[i].z = float( -tokens[1].number );
		++i;
	}
	GLASSERT( i == vertexCount );

	Vertex vertex[16];
	int totalTri = 0;

	// Indices
	while( fgets( buf, 256, fp ) ) {
		if ( *buf == '#' || *buf == 0 ) {
			// comment, emtpy line
			continue;
		}
		std::string str( buf );
		StrTokenize( str, &tokens );
		if ( !tokens.size() ) {
			continue;
		}

		int nIndex = (int) tokens[0].number;
		GLASSERT( nIndex < 16 );

		for( int j=0; j<nIndex; j++ ) {
			int index = (int) tokens[j+1].number;
			GLASSERT( index < vertexCount );
			vertex[j].pos = pos[index];
			vertex[j].tex.Set( 0.0f, 0.0f );	// not supported.
			// Normal computed below.
		}

		// Now the normals (noting the opposite winding)
		Vector3F a = vertex[1].pos - vertex[2].pos;
		Vector3F b = vertex[0].pos - vertex[2].pos;
		Vector3F c = CrossProduct( a, b );

		if ( c.Length() > 0.0f ) {	// there are 0 sized tris...
			c.Normalize();

			for( int j=0; j<nIndex; ++j ) {
				vertex[j].normal = c;
			}

			int nTri = nIndex - 2;
			for( int j=0; j<nTri; ++j ) {
				++totalTri;
				//builder->AddTri( vertex[0], vertex[j+1], vertex[j+2] );
				// Winding of OFF is opposite?
				builder->AddTri( vertex[j+2], vertex[j+1], vertex[0] );
			}
		}
	}
	fclose( fp );
	builder->Flush();
	return true;
}
