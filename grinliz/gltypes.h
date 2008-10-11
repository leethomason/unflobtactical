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



#ifndef GRINLIZ_TYPES_INCLUDED
#define GRINLIZ_TYPES_INCLUDED

#ifdef GL_NO_SDL
	// Typically, this isn't what you want. SDL provides great defaults.
	// But if there isn't SDL:
	
	typedef unsigned char	U8;
	typedef signed char	    S8;
	typedef unsigned short	U16;
	typedef signed short	S16;
	typedef unsigned long	U32;
	typedef long			S32;
	
	#ifdef _MSC_VER
		typedef unsigned __int64 	U64;
		typedef __int64				S64;
	#else
		typedef unsigned long long 	U64;
		typedef long long			S64;
	#endif
	
#else
	#include "SDL_types.h"
	//#include <SDL/SDL_types.h>
	
	typedef Uint8			U8;
	typedef Sint8		    S8;
	typedef Uint16			U16;
	typedef Sint16			S16;
	typedef Uint32			U32;
	typedef Sint32			S32;
	
	#ifdef SDL_HAS_64BIT_TYPE
		typedef Sint64				S64;
		typedef Uint64				U64;
	#else
		#error No 64-bit integer.
	#endif
#endif

// Set up for 64 bit pointers.
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
	#include <stdlib.h>
	typedef uintptr_t		UPTR;
	typedef intptr_t		SPTR;
#elif defined (__GNUC__) && (__GNUC__ >= 3 )
	#include <inttypes.h>
	typedef uintptr_t		UPTR;
	typedef intptr_t		SPTR;
#else
	// Assume not 64 bit pointers. Get a new compiler.
	typedef U32 UPTR;
	typedef S32 SPTR;
#endif

#endif
