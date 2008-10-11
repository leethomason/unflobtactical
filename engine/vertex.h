#ifndef UFOATTACK_VERTEX_INCLUDED
#define UFOATTACK_VERTEX_INCLUDED

#include <vector>
#include "../grinliz/glvector.h"
#include "../grinliz/glmatrix.h"

class Texture;

struct Vertex
{
	grinliz::Vector3F	pos;
	grinliz::Vector3F	normal;
	grinliz::Vector2F	tex;
};


#endif //  UFOATTACK_VERTEX_INCLUDED