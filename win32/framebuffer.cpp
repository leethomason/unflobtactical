#pragma warning ( disable : 4530 )		// Don't warn about unused exceptions.

#include "glew.h"
#include "framebuffer.h"
#include "../grinliz/glutil.h"

FrameBuffer::FrameBuffer( int width, int height )
{
	glGenTextures( 1, &textureID );
	glBindTexture( GL_TEXTURE_2D, textureID );

	w2 = grinliz::CeilPowerOf2( width );
	h2 = grinliz::CeilPowerOf2( height );

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexImage2D(	GL_TEXTURE_2D, 
					0, 
					GL_RGBA8, 
					w2, 
					h2, 
					0, 
					GL_BGRA, 
					GL_UNSIGNED_INT_8_8_8_8_REV, 
					0 );

	glGenFramebuffersEXT( 1, &frameBufferObject );
	GLASSERT( glGetError() == GL_NO_ERROR );	

	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, frameBufferObject );
	GLASSERT( glGetError() == GL_NO_ERROR );	

	glGenRenderbuffersEXT(1, &depthbuffer);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthbuffer);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, w2, h2 );
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthbuffer);

	glBindTexture(GL_TEXTURE_2D, textureID );
	GLASSERT( glGetError() == GL_NO_ERROR );	

	glFramebufferTexture2DEXT(	GL_FRAMEBUFFER_EXT, 
								GL_COLOR_ATTACHMENT0_EXT, 
								GL_TEXTURE_2D, 
								textureID, 0);

	GLASSERT( glGetError() == GL_NO_ERROR );	

	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	GLASSERT( status == GL_FRAMEBUFFER_COMPLETE_EXT );	

	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	GLASSERT( glGetError() == GL_NO_ERROR );	
}

FrameBuffer::~FrameBuffer()
{
	glDeleteFramebuffersEXT(1, &frameBufferObject);
	glDeleteRenderbuffersEXT( 1, &depthbuffer );
}


void FrameBuffer::Bind() 
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBufferObject);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,w2,h2);
}


void FrameBuffer::UnBind() 
{
	glPopAttrib();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	GLASSERT( glGetError() == GL_NO_ERROR );	
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable( GL_TEXTURE_2D );

	GLASSERT( glGetError() == GL_NO_ERROR );
}
