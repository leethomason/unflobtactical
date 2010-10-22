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



#ifndef GRINLIZ_PERFORMANCE_MEASURE
#define GRINLIZ_PERFORMANCE_MEASURE

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif
#if defined(__APPLE__)
//#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_time.h>
# endif

#include "gldebug.h"
#include "gltypes.h"
#include <stdio.h>


#if defined(_MSC_VER) && defined(_M_IX86)
namespace grinliz {
	typedef U64 TimeUnit;

	inline U64 FastTime()
	{
		union 
		{
			U64 result;
			struct
			{
				U32 lo;
				U32 hi;
			} split;
		} u;
		u.result = 0;

		_asm {
			//pushad;	// don't need - aren't using "emit"
			cpuid;		// force all previous instructions to complete - else out of order execution can confuse things
			rdtsc;
			mov u.split.hi, edx;
			mov u.split.lo, eax;
			//popad;
		}				
		return u.result;
	}
}

#elif defined(__GNUC__) && defined(__i386__) 
namespace grinliz {
	typedef U64 TimeUnit;

	inline U64 FastTime()
	{
		U64 val;
		 __asm__ __volatile__ ("rdtsc" : "=A" (val));
		 return val;
	}
}
#elif defined (__APPLE__) && defined(__arm__)
namespace grinliz {
	typedef U64 TimeUnit;
	inline U64 FastTime() {
		return mach_absolute_time();
	}
}
#else
#include <time.h>
namespace grinliz {
	typedef clock_t TimeUnit;
	inline TimeUnit FastTime() { return clock(); }
}
#endif

namespace grinliz {
class QuickProfile
{
public:
	QuickProfile( const char* name)		{ 
		startTime = FastTime(); 
		this->name = name;
	}
	~QuickProfile()		{ 
		U64 endTime = FastTime();	
		GLOUTPUT(( "%s %d MClocks\n", name, (int)((endTime-startTime)/(U64)(1000*1000)) ));
	}

private:
	TimeUnit startTime;
	const char* name;
};

const int GL_MAX_PROFILE_ITEM = 60;	// Max functions that can be profiled.

// Static - there is only one of these for a given function.
struct PerformanceData
{
	PerformanceData( const char* name );
	PerformanceData() : name( 0 ), id( 0 ), functionCalls( 0 ), functionTime( 0 ), functionTopTime( 0 ), normalTime( 0 ) {}

	const char* name;
	int id;
	U32			functionCalls;
	TimeUnit	functionTime;
	TimeUnit	functionTopTime;
	float normalTime;

	void Clear();
};


/**
	Used to automatically track performance of blocks of code. Should
	be used to measure code that is blocked out like this:

	@verbatim
	#ifdef L3PERF
	static PerformanceData data( "L3TerrainMesh_Stream" );
	Performance perf( &data );
	#endif
	@endverbatim
*/
class Performance
{
	friend struct PerformanceData;
  public:

	Performance( PerformanceData* _data )	{
		this->data = _data;

		++data->functionCalls;
		start = FastTime();
		++callDepth;
	}

	~Performance()
	{
		TimeUnit end = FastTime();
		GLASSERT( end >= start );
		data->functionTime += ( end - start );
		--callDepth;
		if ( callDepth == 0 ) {
			data->functionTopTime += (end - start);
		}
	}

	/// Write the results of performance testing to a file.
    static void Dump( FILE* fp, const char* desc );
	/// Reset the profiling data.
	static void Clear();

	static void SampleData();
	static int NumData()								{ return numMap; }
	static const PerformanceData& GetData( int i )		{ GLASSERT( i >= 0 && i < numMap ); return sample[i]; }

  protected:
	static PerformanceData* map[ GL_MAX_PROFILE_ITEM ];
	static PerformanceData  sample[ GL_MAX_PROFILE_ITEM ];
	static int numMap;
	static int callDepth;
	static TimeUnit totalTime;

	PerformanceData* data;
	TimeUnit start;
};

#ifdef GRINLIZ_PROFILE
	#define GRINLIZ_PERFTRACK static PerformanceData data( __FUNCTION__ ); Performance perf( &data );
	#define GRINLIZ_PERFTRACK_NAME( x ) static PerformanceData data( x ); Performance perf( &data );
#else
	#define GRINLIZ_PERFTRACK			{}
	#define GRINLIZ_PERFTRACK_NAME( x )	{}
#endif


};		

#endif
