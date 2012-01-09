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



#ifndef GRINLIZ_DEBUG_INCLUDED
#define GRINLIZ_DEBUG_INCLUDED

#if defined( _DEBUG ) || defined( DEBUG ) || defined (__DEBUG__)
	#ifndef DEBUG
		#define DEBUG
	#endif
#endif

//extern "C" {
//void GrinlizSetReleaseAssertPath( const char* path );
//};
//void GrinlizReleaseAssert( const char* file, int line );

#if defined(DEBUG)
	#if defined(_MSC_VER)
		void dprintf( const char* format, ... );
		void WinDebugBreak();
		
		#define GLASSERT( x )		if ( !(x)) { _asm { int 3 } } //if ( !(x)) WinDebugBreak()
		#define GLRELASSERT( x )	if ( !(x)) { _asm { int 3 } } //if ( !(x)) WinDebugBreak()
		#define GLOUTPUT( x )		dprintf x
	#elif defined (ANDROID_NDK)
		#include <android/log.h>
		void dprintf( const char* format, ... );
		#define GLASSERT( x )		if ( !(x)) { __android_log_assert( "assert", "grinliz", "ASSERT in '%s' at %d.", __FILE__, __LINE__ ); }
		#define GLRELASSERT( x )	if ( !(x)) { __android_log_assert( "assert", "grinliz", "ASSERT in '%s' at %d.", __FILE__, __LINE__ ); }
		#define	GLOUTPUT( x )		dprintf x
	#else
		#include <assert.h>
        #include <stdio.h>
		#define GLASSERT		assert
		#define GLRELASSERT		assert
		#define GLOUTPUT( x )	printf x	
	#endif
#else
 	#if defined(UNIX)
		#define GLOUTPUT( x )
	#else
		#define GLOUTPUT( x )
	#endif

	#define GLASSERT( x )		{}
	// Turned off for now. The error reporting is hard enough.
	#define GLRELASSERT( x )	{}
#endif

#if defined(DEBUG)
	
	#if defined(_MSC_VER)
		#define GRINLIZ_DEBUG_MEM
	#endif

	#ifdef GRINLIZ_DEBUG_MEM
		void* DebugNew( size_t size, bool arrayType, const char* name, int line );
		
		inline void* operator new[]( size_t size, const char* file, int line ) {
			return DebugNew( size, true, file, line );
		}

		inline void* operator new( size_t size, const char* file, int line ) {
			return DebugNew( size, false, file, line );
		}

		void MemLeakCheck();
		void MemStartCheck();
		void MemHeapCheck();
		#define glnew new(__FILE__, __LINE__)
	#else
		#define glnew new
		inline void MemLeakCheck()	{}
		inline void MemStartCheck()	{}
		inline void MemHeapCheck()	{}
	#endif
#else
	#define glnew new
	inline void MemLeakCheck()	{}
	inline void MemStartCheck()	{}
#endif

#endif // file
