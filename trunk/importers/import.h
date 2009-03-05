#ifndef ALTERAORBIS_IMPORT_INCLUDED
#define ALTERAORBIS_IMPORT_INCLUDED

#include <string>
#include <vector>

#include "../engine/vertex.h"
#include "../ufobuilder/modelbuilder.h"

bool ImportAC3D(	const std::string& filename, 
					ModelBuilder* builder );

bool ImportOFF(		const std::string& filename, 
					ModelBuilder* builder );


#endif // ALTERAORBIS_IMPORT_INCLUDED