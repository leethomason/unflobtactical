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
#include <CoreFoundation/CoreFoundation.h>
# endif

#include "gldebug.h"
#include "gltypes.h"

namespace grinliz {

#if defined(_MSC_VER)
	typedef U64 timeUnit;

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
	#define LILITH_CONVERT_TO_MSEC (10000)

#elif defined(__GNUC__) && !defined(__APPLE__) 
	typedef U64 timeUnit;

	inline U64 FastTime()
	{
		U64 val;
		 __asm__ __volatile__ ("rdtsc" : "=A" (val));
		 return val;
	}
#elif defined (__APPLE__)
	typedef double timeUnit;
	
	inline double FastTime() {
		return CFAbsoluteTimeGetCurrent();
	}
#else
#	error Platform not supported for profiling.
#endif


const int GL_MAX_PROFILE_ITEM = 32;	// Max functions that can be profiled.

// Static - there is only one of these for a given function.
struct PerformanceData
{
	PerformanceData( const char* name, const char* subName );

	const char* name;
	const char* subName;
	int id;
	U32 functionCallsTop;
	timeUnit functionTimeTop;
	U32 functionCallsSub;
	timeUnit functionTimeSub;

	void Clear();
};


struct ProfileItem
{
	const char* name;
	U32 functionCalls;	// # of calls
	timeUnit functionTime;	// total time - in no particular unit (multiple of clock cycle)
};

struct ProfileData
{
	timeUnit totalTime;		// total time of all items (in CPU clocks)
	U32 count;			// number of items
	grinliz::ProfileItem item[ GL_MAX_PROFILE_ITEM*2 ];

	float NormalTime( timeUnit t ) const {
		if ( totalTime == 0 )
			return 0.0f;
		else
			return (float)( (double)t / (double)totalTime );
	}
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

		if ( inUse )
			++data->functionCallsSub;
		else
			++data->functionCallsTop;

		start = FastTime();
		++inUse;
	}

	~Performance()
	{
		--inUse;
		U64 end = FastTime();
		GLASSERT( end >= start );
		if ( inUse )
			data->functionTimeSub += ( end - start );
		else	
			data->functionTimeTop += ( end - start );
	}

	/// Write the results of performance testing to a file.
    static void Dump( FILE* fp, const char* desc );
	/// Get the profiling data in a useable format, optionally sorted on total time.
	static const grinliz::ProfileData& GetData( bool sortOnTime );
	/// Reset the profiling data.
	static void Clear();

  protected:

	static PerformanceData* map[ GL_MAX_PROFILE_ITEM ];
	static ProfileData	  profile;
	static int numMap;
	static int inUse;

	PerformanceData* data;
	U64 start;
};

#ifdef GRINLIZ_PROFILE
	#define GRINLIZ_PERFTRACK static PerformanceData data( __FUNCTION__, __FUNCTION__ ); Performance perf( &data );
#else
	#define GRINLIZ_PERFTRACK {}
#endif


};		

#endif
