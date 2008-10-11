#include "modelbuilder.h"

void ModelBuilder::SetTexture( const char* textureName )
{
	GLASSERT( strlen( textureName ) < 16 );
	current = 0;

	for( unsigned i=0; i<group.size(); ++i ) {
		if ( strcmp( textureName, group[i].textureName ) == 0 ) {
			current = &group[i];
			break;
		}
	}
	if ( !current ) {
		VertexGroup vg;
		group.push_back( vg );
		current = &group[group.size()-1];
		strcpy( current->textureName, textureName );
	}
}
