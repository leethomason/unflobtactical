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

#ifndef UFOATTACK_FRAMEBUFFER_INCLUDED
#define UFOATTACK_FRAMEBUFFER_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

class FrameBuffer
{
public:
	FrameBuffer( int width, int height );
	~FrameBuffer();

	void Bind();
	void UnBind();

	U32 TextureID()	{ return colorBuffer; }

private:
	U32 frameBufferObject;
	U32 colorBuffer;
	U32 depthbuffer;
	U32 w2, h2;
};

#endif // UFOATTACK_FRAMEBUFFER_INCLUDED