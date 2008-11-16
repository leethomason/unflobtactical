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

	U32 TextureID()	{ return textureID; }

private:
	U32 textureID;
	U32 frameBufferObject;
	U32 w2, h2;
};

#endif // UFOATTACK_FRAMEBUFFER_INCLUDED