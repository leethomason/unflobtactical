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

#ifndef UFOATTACK_TEXT_INCLUDED
#define UFOATTACK_TEXT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "screenport.h"


class UFOText
{
public:
	static void InitTexture( U32 textTextureID );
	static void InitScreen( const Screenport& sp );
	static void GlyphSize( const char* str, int* width, int* height );

	static void Begin();
	static void Stream( int x, int y, const char* format, ... );
	static void End();

	static void Draw( int x, int y, const char* format, ... );

private:
	static void TextOut( const char* str, int x, int y );

	static Screenport screenport;
	static U32 textureID;
};

#endif // UFOATTACK_TEXT_INCLUDED