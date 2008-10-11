/*
Copyright (c) 2000-2006 Lee Thomason (www.grinninglizard.com)
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



#ifndef GRINLIZ_STRINGUTIL_INCLUDED
#define GRINLIZ_STRINGUTIL_INCLUDED

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <string>
#include "gldebug.h"
#include "gltypes.h"

namespace grinliz 
{
	
/** Loads a text file to the given string. Returns true on success, false
    if the file could not be found.
*/
bool LoadTextFile( const char* filename, std::string* str );

};	// namespace grinliz

#endif
