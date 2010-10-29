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

#ifndef UFOATTACK_VERTEX_INCLUDED
#define UFOATTACK_VERTEX_INCLUDED

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

	bool Equal( const Vertex& v, float EPS ) const
	{
		if (	pos.Equal( v.pos, EPS ) 
			 && normal.Equal( v.normal, EPS ) 
			 && tex.Equal( v.tex, EPS ) )
		{
			return true;
		}
		return false;
	}
};


struct PTVertex
{
	enum {
		POS_OFFSET = 0,
		TEXTURE_OFFSET = 12
	};

	grinliz::Vector3F	pos;
	grinliz::Vector2F	tex;
};

struct PTVertex2
{
	enum {
		POS_OFFSET		= 0,
		TEXTURE_OFFSET	= 8
	};

	grinliz::Vector2F	pos;
	grinliz::Vector2F	tex;
};

typedef grinliz::Vector3< U8 > Color3U;
typedef grinliz::Vector4< U8 > Color4U;
typedef grinliz::Vector3< float > Color3F;
typedef grinliz::Vector4< float > Color4F;


#endif //  UFOATTACK_VERTEX_INCLUDED