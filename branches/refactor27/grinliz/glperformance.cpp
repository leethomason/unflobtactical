/*
Copyright (c) 2000-2010 Lee Thomason (www.grinninglizard.com)
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


#ifdef _WIN32
	#include <windows.h>
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <stdio.h>

#include "gldebug.h"
#include "glperformance.h"
#include "glutil.h"

using namespace grinliz;
using namespace std;

PerformanceData* Performance::map[ GL_MAX_PROFILE_ITEM ];
PerformanceData Performance::sample[ GL_MAX_PROFILE_ITEM ];
int Performance::numMap = 0;
int Performance::callDepth = 0;


PerformanceData::PerformanceData( const char* _name ) : name( _name )
{ 
	Clear();
	Performance::map[ Performance::numMap ] = this;
	id = Performance::numMap;
	++Performance::numMap;
}


void PerformanceData::Clear()
{
	functionCalls= 0;
	functionTime = 0;
	functionTopTime = 0;
	normalTime = 0.f;
}


///////////////////////////////////////////////////////


/* static */ void Performance::Clear()
{
	for( int i=0; i<numMap; ++i ) {
		map[i]->Clear();
		sample[i].Clear();
	}
}


/*static*/ void Performance::SampleData()
{
	TimeUnit total = 0;
	for( int i=0; i<numMap; ++i ) {
		total += map[i]->functionTopTime;
	}

	for( int i=0; i<numMap; ++i ) {
		sample[i] = *map[i];
		map[i]->Clear();

		sample[i].normalTime = 0.f;
		if ( total ) {
			sample[i].normalTime = (float)((double)sample[i].functionTime / (double)total);
		}
	}
}




void Performance::Dump( FILE* fp, const char* desc )
{
	fprintf( fp, "                                                    calls     time\n" );
	for( int i=0; i<numMap; ++i )
	{
		fprintf( fp, "%48s %10d %.1f%%\n",
				sample[i].name,
				(int)sample[i].functionCalls,
				sample[i].normalTime );
	}

}

