#ifndef UFO_BUILDER_INCLUDED
#define UFO_BUILDER_INCLUDED

/*	MODEL

	name				char[16]
	nGroups				BE32
	nTotalIndices		BE32
	nTotalVertices		BE32
	
	Group[]
		textureName		char[16]
		startVertex		BE32
		nVertex			BE32
		startIndex		BE32
		nIndex			BE32

	vertexData			Vertex[]
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