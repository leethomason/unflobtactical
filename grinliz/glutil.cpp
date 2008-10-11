/*
Copyright (c) 2000-2005 Lee Thomason (www.grinninglizard.com)
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

#include "glutil.h"


float grinliz::gU8ToFloat[256];

namespace grinliz
{
	class U8ToFloatInit
	{
	  public:
		U8ToFloatInit() {
			for ( int i=0; i<256; ++i )
				gU8ToFloat[i] = (float)i / 255.f;
		}
	};
};

static grinliz::U8ToFloatInit u8ToFloatInit;
