#ifndef UFOATTACK_VERTEX_INCLUDED
#define UFOATTACK_VERTEX_INCLUDED

#include <vector>
#include "../grinliz/glvector.h"
#include "../grinliz/glmatrix.h"

class Texture;


struct Vertex
{
	enum {
		POS_OFFSET = 0,
		NORMAL_OFFSET = 12,
		TEXTURE_OFFSET = 24
	};

	grinliz::Vector3F	pos;
	grinliz::Vector3F	normal;
	grinliz::Vector2F	tex;

	bool Equal( const Vertex& v ) const {
		if (    pos == v.pos 
			 && normal == v.normal
			 && tex == v.tex )
		{
			return true;
		}
		return false;
	}
};

typedef S32 FIXED;
typedef grinliz::Vector3< FIXED > Vector3X;
typedef grinliz::Vector2< FIXED > Vector2X;

const FIXED FIXED_1 = 0x10000;

inline FIXED FloatToFixed( float f ) {
	return grinliz::LRintf( f * 65536.0f );
}

inline FIXED FloatToFixed( double f ) {
	return grinliz::LRint( f * 65536.0 );
}

inline float FixedToFloat( S32 v ) {
	return (float)v / 65536.0f;
}

struct VertexX
{
	enum {
		POS_OFFSET = 0,
		NORMAL_OFFSET = 12,
		TEXTURE_OFFSET = 24
	};

	void From( const Vertex& v )
	{
		pos.x = FloatToFixed( v.pos.x );
		pos.y = FloatToFixed( v.pos.y );
		pos.z = FloatToFixed( v.pos.z );

		normal.x = FloatToFixed( v.normal.x );
		normal.y = FloatToFixed( v.normal.y );
		normal.z = FloatToFixed( v.normal.z );

		tex.x = FloatToFixed( v.tex.x );
		tex.y = FloatToFixed( v.tex.y );
	}

	Vector3X	pos;
	Vector3X	normal;
	Vector2X	tex;

	bool Equal( const VertexX& v ) const {
		if (    pos == v.pos 
			 && normal == v.normal
			 && tex == v.tex )
		{
			return true;
		}
		return false;
	}
};

#endif //  UFOATTACK_VERTEX_INCLUDED