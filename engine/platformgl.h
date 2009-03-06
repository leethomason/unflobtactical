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

#ifdef __APPLE__
	// Really iPhone, not apple...
	#import <OpenGLES/ES1/gl.h>
	#import <OpenGLES/ES1/glext.h>

	#define glFrustumfX		glFrustumf
#else
	#include "../win32/glew.h"

	#define glFrustumfX		glFrustum

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