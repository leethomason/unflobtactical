#ifndef UFOATTACK_MAP_INCLUDED
#define UFOATTACK_MAP_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

class Map
{
public:
	enum {
		SIZE = 64
	};

	Map()	{}
	~Map()	{}

	void Draw();

	int foo;
};

#endif // UFOATTACK_MAP_INCLUDED
