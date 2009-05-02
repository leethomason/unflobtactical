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


#ifdef _WIN32
	#include <windows.h>
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <stdio.h>
#include <vector>
#include <algorithm>

#include "gldebug.h"
#include "glperformance.h"
#include "glutil.h"

using namespace grinliz;
using namespace std;

PerformanceData* Performance::map[ GL_MAX_PROFILE_ITEM ];
ProfileData Performance::profile;
int Performance::numMap = 0;
int Performance::inUse = 0;

PerformanceData::PerformanceData( const char* _name, const char* _subName ) : name( _name ), subName( _subName )
{ 
	Clear();
	Performance::map[ Performance::numMap ] = this;
	id = Performance::numMap;
	++Performance::numMap;
}


void PerformanceData::Clear()
{
	functionCallsTop = 0;
	functionTimeTop = 0;
	functionCallsSub = 0;
	functionTimeSub = 0;
}

///////////////////////////////////////////////////////


void Performance::Clear()
{
	for( int i=0; i<numMap; ++i ) {
		map[i]->Clear();
	}
}

struct PDIgreater {
	bool operator()(const ProfileItem& _X, const ProfileItem& _Y) const
		{return (_X.functionTime > _Y.functionTime ); }
};


/*static*/ const ProfileData& Performance::GetData( bool sortOnTime )
{
	// Compute the total time, and the highest bit.
	profile.totalTime = 0;
	//profile.totalTimeMSec = 0;

	profile.count = 0;
	for( int i=0; i<numMap; ++i )
	{
		if ( map[i]->functionCallsTop ) 
		{
			profile.item[profile.count].name = map[i]->name;

			profile.item[profile.count].functionCalls = map[i]->functionCallsTop;
			profile.item[profile.count].functionTime = map[i]->functionTimeTop;
			profile.totalTime += profile.item[profile.count].functionTime;

			++profile.count;
		}
		if ( map[i]->functionCallsSub ) 
		{
			profile.item[profile.count].name = map[i]->subName;

			profile.item[profile.count].functionCalls = map[i]->functionCallsSub;
			profile.item[profile.count].functionTime = map[i]->functionTimeSub;
			// does NOT impact total time
			++profile.count;
		}
	}
	//for( unsigned i=0; i<profile.count; ++i ) {
	//	profile.item[i].functionTimeMSec = (U32)(profile.item[i].functionTime / U64(LILITH_CONVERT_TO_MSEC));
	//}
	//profile.totalTimeMSec = (U32)(profile.totalTime / (U64)LILITH_CONVERT_TO_MSEC);
	//if ( profile.totalTimeMSec == 0 ) {
		//profile.totalTimeMSec = 1;
	//}

	if ( sortOnTime ) {
		sort( &profile.item[0], 
			  &profile.item[ profile.count ], 
			  PDIgreater() );
	}
	return profile;
}


void Performance::Dump( FILE* fp, const char* desc )
{
	const ProfileData& pd = GetData( true );

	fprintf( fp, "                                                    calls     time    time/call percent\n" );
	for( U32 i=0; i<pd.count; ++i )
	{
		U32 functionCalls = (pd.item[i].functionCalls>0) ? pd.item[i].functionCalls : 1;

		//fprintf( fp, "%48s %8d %10d %10d %.1f%%\n",
		fprintf( fp, "%48s %10d %.1f%%\n",
				pd.item[i].name,
				//pd.item[i].id,
				pd.item[i].functionCalls,
				//pd.item[i].functionTimeMSec,
				//(U32)( pd.item[i].functionTime / functionCalls / (U64)(LILITH_CONVERT_TO_MSEC) ),
				100.0 * double( pd.item[i].functionTime ) / double( pd.totalTime ) );
	}
	//fprintf( fp, "Total time: %d\n", pd.totalTimeMSec );

	//int maxFrame = grinliz::Min( (nFrames+1), GL_MAX_PROFILE_FRAMES );
	//fprintf( fp, "    " );

	//for( unsigned i=0; i<pd.count; ++i )
	//	fprintf( fp, "  %3d ", i );
	//fprintf( fp, "\n" );

	//for( int i=0; i<maxFrame; ++i ) {
	//	fprintf( fp, "%3d. ", i );
	//	for( int j=0; j<numMap; ++j ) {
	//		U64 time = 0;
	//		if ( map[j]->functionCallsPF[i] > 0 )
	//			time = map[j]->functionTimePF[i] / map[j]->functionCallsPF[i];
	//		time /= (U64)(LILITH_CONVERT_TO_MSEC);
	//		fprintf( fp, "%5d ", (U32)(time)  );
	//	}
	//	fprintf( fp, "\n" );
	//}
}

