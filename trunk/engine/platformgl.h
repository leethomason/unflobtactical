#ifndef UFOATTACK_PLATFORMGL_INCLUDED
#define UFOATTACK_PLATFORMGL_INCLUDED

#define TESTGLERR()	{	GLenum err = glGetError();				\
						if ( err != GL_NO_ERROR ) {				\
							GLOUTPUT(( "GL ERR=0x%x\n", err )); \
							GLASSERT( 0 );						\
						}										\
					}

#ifdef __APPLE__
	#import <OpenGLES/ES1/gl.h>
	#import <OpenGLES/ES1/glext.h>
#else
	#include "../win32/glew.h"
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