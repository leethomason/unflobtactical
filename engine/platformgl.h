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

#ifndef UFOATTACK_PLATFORMGL_INCLUDED
#define UFOATTACK_PLATFORMGL_INCLUDED

#include "../grinliz/gldebug.h"

#define TESTGLERR()	{	GLenum err = glGetError();				\
						if ( err != GL_NO_ERROR ) {				\
							GLOUTPUT(( "GL ERR=0x%x\n", err )); \
							GLASSERT( 0 );						\
						}										\
					}

#if defined(UFO_IPHONE)
	// Really iPhone, not apple...
	#import <OpenGLES/ES1/gl.h>
	#import <OpenGLES/ES1/glext.h>

	#define glFrustumfX		glFrustumf
	#define glOrthofX		glOrthof
	#define USING_ES
#elif defined (ANDROID_NDK)
	#include <GLES/gl.h>
	#define glFrustumfX		glFrustumf
	#define glOrthofX		glOrthof
	#define USING_ES
	// Of all the stupid, stupid driver bugs. (This one on my netbook, again.
	// The ARB form resolves but not the normal form.)
	#define glGenBuffersX	glGenBuffers
	#define glBindBufferX	glBindBuffer
	#define glBufferDataX	glBufferData
	#define glBindBufferX	glBindBuffer
	#define glDeleteBuffersX	glDeleteBuffers
#elif defined( UFO_WIN32_SDL )
	#include "../win32/glew.h"

	#define glFrustumfX		glFrustum
	#define glOrthofX		glOrtho
	#define USING_GL
	#define glGenBuffersX	glGenBuffersARB
	#define glBindBufferX	glBindBufferARB
	#define glBufferDataX	glBufferDataARB
	#define glBindBufferX	glBindBufferARB
	#define glDeleteBuffersX	glDeleteBuffersARB
#else
	#error Undefined platform
#endif


#ifdef DEBUG
#define CHECK_GL_ERROR	{	GLenum error = glGetError();				\
							if ( error  != GL_NO_ERROR ) {				\
								GLOUTPUT(( "GL Error: %x\n", error ));	\
								GLASSERT( 0 );							\
							}											\
						}
#else
#define CHECK_GL_ERROR	{}
#endif



#endif // UFOATTACK_PLATFORMGL_INCLUDED