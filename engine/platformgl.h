#ifndef UFOATTACK_PLATFORMGL_INCLUDED
#define UFOATTACK_PLATFORMGL_INCLUDED

#define TESTGLERR()	{	GLenum err = glGetError();				\
						if ( err != GL_NO_ERROR ) {				\
							GLOUTPUT(( "GL ERR=0x%x\n", err )); \
							GLASSERT( 0 );						\
						}										\
					}

#include "../win32/glew.h"


#endif // UFOATTACK_PLATFORMGL_INCLUDED