#ifndef UFOATTACK_MAP_INCLUDED
#define UFOATTACK_MAP_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

class Model;

class Map
{
public:
	enum {
		SIZE = 64					// maximum size
	};

	Map()	{}
	~Map()	{}

	void SetModel( Model* m );
	void Draw();

	int Width()		{ return width; }	// the size in use
	int Height()	{ return height; }	

private:
	Model* model;
	int width, height;
};

#endif // UFOATTACK_MAP_INCLUDED
