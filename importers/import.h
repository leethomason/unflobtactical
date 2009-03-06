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
