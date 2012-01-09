/*
Copyright (c) 2000-2007 Lee Thomason (www.grinninglizard.com)
Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#ifndef __APPLE__
	#include "SDL_loadso.h"
	#include "SDL_error.h"
#else
	#include <dlfcn.h>
#endif 

#include <string>

#include "gldebug.h"
#include "gldynamic.h"


using namespace std;

void* grinliz::grinlizLoadLibrary( const char* name )
{
	string libraryName;
	void* handle = 0;

	#if defined( _WIN32 )
		libraryName = name;
		handle = SDL_LoadObject( libraryName.c_str() );
		GLRELASSERT( handle );
		
		//if ( !handle ) {
		//	GLLOG(( "ERROR: could not load %s. Reason: %s\n", libraryName.c_str(), SDL_GetError() ));
		//}
	#elif defined(__APPLE__)
		libraryName = name;
		handle = dlopen ( libraryName.c_str(), RTLD_LAZY);
		if (!handle) {
			GLLOG(( "ERROR: could not load %s.\n", libraryName.c_str() ));
		}
	#endif
	return handle;
}


void* grinliz::grinlizLoadFunction( void* handle, const char* functionName )
{
	void* func = 0;

	#ifdef __APPLE__
		func = dlsym( handle, functionName );
	#else
		func = SDL_LoadFunction( handle, functionName );
	#endif

	return func;
}
