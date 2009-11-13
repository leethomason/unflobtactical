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

#pragma warning ( disable : 4530 )		// Don't warn about unused exceptions.

#include "glew.h"
#include "framebuffer.h"
#include "../grinliz/glutil.h"

//#define NEED_POWER_OF_2

FrameBuffer::FrameBuffer( int width, int height )
{
#ifdef NEED_POWER_OF_2
	w2 = grinliz::CeilPowerOf2( width );
	h2 = grinliz::CeilPowerOf2( height );
#else
	w2 = width;
	h2 = height;
#endif
/*
	glGenTextures( 1, &textureID );
	glBindTexture( GL_TEXTURE_2D, textureID );

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
*/

	// Create the FBO and Bind it.
	glGenFramebuffersEXT( 1, &frameBufferObject );
	GLASSERT( glGetError() == GL_NO_ERROR );	
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, frameBufferObject );
	GLASSERT( glGetError() == GL_NO_ERROR );	


	// Create the color buffer.
	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w2, h2, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );

	glFramebufferTexture2DEXT(	GL_FRAMEBUFFER_EXT, 
								GL_COLOR_ATTACHMENT0_EXT, 
								GL_TEXTURE_2D, 
								colorBuffer, 0);
	GLASSERT( glGetError() == GL_NO_ERROR );	

	// Attach a depth buffer:
	glGenRenderbuffersEXT(1, &depthbuffer);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthbuffer);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, w2, h2 );
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthbuffer);
	GLASSERT( glGetError() == GL_NO_ERROR );	

	// Confirm the status.
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	GLASSERT( status == GL_FRAMEBUFFER_COMPLETE_EXT );	

	// Clean up.
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
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
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	GLASSERT( status == GL_FRAMEBUFFER_COMPLETE_EXT );	

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
