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

#ifndef UFO_BUILDER_INCLUDED
#define UFO_BUILDER_INCLUDED

/*	MODEL

	name				char[16]
	nGroups				BE32
	nTotalVertices		BE32
	nTotalIndices		BE32
	bounds				Vector3X[2] min, max in BE32
	
	Group[]
		textureName		char[16]
		nVertex			BE32
		nIndex			BE32

	vertexData			VertexX[]
	indexData			BE16[]
*/

/*	TEXTURE

	name				char[16]
	format				BE32	(OpenGL constants)
	type				BE32	(OpenGL constants)
	width				BE32
	height				BE32

	pixel data			BE16
*/

#endif // UFO_BUILDER_INCLUDED