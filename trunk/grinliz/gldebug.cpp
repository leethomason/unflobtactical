/*
Copyright (c) 2000-2007 Lee Thomason (www.grinninglizard.com)

Grinning Lizard Utilities. Note that software that uses the 
utility package (including Lilith3D and Kyra) have more restrictive
licences which applies to code outside of the utility package.


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

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <stdio.h>

#endif	//_WIN32

#include "gldebug.h"

#ifdef DEBUG
#include <stdlib.h>

#ifdef GRINLIZ_DEBUG_MEM

size_t memTotal = 0;
size_t memWatermark = 0;
long memNewCount = 0;
long memDeleteCount = 0;
unsigned long MEM_MAGIC0 = 0xbaada55a;
unsigned long MEM_MAGIC1 = 0xbaada22a;
unsigned long MEM_DELETED0 = 0x12345678;
unsigned long MEM_DELETED1 = 0x9ABCDEF0;
unsigned long idPool = 0;
bool checking = false;

struct MemCheckHead
{
	size_t size;
	bool arrayType;
	unsigned long id;
	const char* name;
	int line;

	MemCheckHead* next;
	MemCheckHead* prev;
	unsigned long magic;
};

MemCheckHead* root = 0;

struct MemCheckTail
{
	unsigned long magic;
};

void* DebugNew( size_t size, bool arrayType, const char* name, int line )
{
	void* mem = 0;
	if ( size == 0 )
		size = 1;

	GLASSERT( size );
	size_t allocateSize = size + sizeof(MemCheckHead) + sizeof(MemCheckTail);
	mem = malloc( allocateSize );

	MemCheckHead* head = (MemCheckHead*)(mem);
	MemCheckTail* tail = (MemCheckTail*)((unsigned char*)mem+sizeof(MemCheckHead)+size);
	void* body = (void*)((unsigned char*)mem+sizeof(MemCheckHead));
	GLASSERT( body );

	head->size = size;
	head->arrayType = arrayType;
	head->id = idPool++;
	head->name = name;
	head->line = line;

	head->magic = MEM_MAGIC0;
	tail->magic = MEM_MAGIC1;
	head->prev = head->next = 0;

	if ( checking )
	{
		++memNewCount;
		memTotal += size;
		if ( memTotal > memWatermark )
			memWatermark = memTotal;

		if ( root )
			root->prev = head;
		head->next = root;
		head->prev = 0;
		root = head;
	}

	// #BREAKHERE
	//if ( head->id == 59828 )
	//	int debug = 1;

#ifdef GRINLIZ_DEBUG_MEM_DEEP
	MemHeapCheck();
#endif
	return body;
}


void DebugDelete( void* mem, bool arrayType )
{
#ifdef GRINLIZ_DEBUG_MEM_DEEP
	MemHeapCheck();
#endif
	if ( mem ) {
		MemCheckHead* head = (MemCheckHead*)((unsigned char*)mem-sizeof(MemCheckHead));
		MemCheckTail* tail = (MemCheckTail*)((unsigned char*)mem+head->size);

		// For debugging, so if the asserts do fire, we still have a copy of the values.
		MemCheckHead aHead = *head;
		MemCheckTail aTail = *tail;

		GLASSERT( head->magic == MEM_MAGIC0 );
		GLASSERT( tail->magic == MEM_MAGIC1 );
		
		// A bug in the OpenGL driver on Mac fires this.
		#if !defined( __APPLE__ )
		//GLASSERT( head->arrayType == arrayType );
		#endif

		if ( head->prev )
			head->prev->next = head->next;
		else
			root = head->next;

		if ( head->next )
			head->next->prev = head->prev;
		
		if ( checking )
		{
			++memDeleteCount;
			memTotal -= head->size;
		}
		head->magic = MEM_DELETED0;
		tail->magic = MEM_DELETED1;
		free(head);
	}
}


void* operator new( size_t size ) 
{
	return DebugNew( size, false, 0, 0 );
}

void* operator new[]( size_t size ) 
{
	return DebugNew( size, true, 0, 0 );
}

void operator delete[]( void* mem ) 
{
	DebugDelete( mem, true );
}

void operator delete( void* mem ) 
{
	DebugDelete( mem, false );
}

void MemLeakCheck()
{
	GLOUTPUT((	"MEMORY REPORT: watermark=%dk new count=%d. delete count=%d. %d allocations leaked.\n",
				memWatermark/1024, memNewCount, memDeleteCount, memNewCount-memDeleteCount ));

	for( MemCheckHead* node = root; node; node=node->next )
	{
		GLOUTPUT(( "  size=%d %s id=%d name=%s line=%d\n",
					node->size, (node->arrayType) ? "array" : "single", node->id,
					(node->name) ? node->name :  "(null)", node->line ));
	}		    

	/*
		If these fire, then a memory leak has been detected. The library doesn't track
		the source. The best way to find the source is to break in the allocator,
		search for: #BREAKHERE. Each allocation has a unique ID. Once you know the ID 
		of the thing you are leaking, you can track the source of the leak.

		It's not elegant, but it does work.
	*/
	GLASSERT( memNewCount-memDeleteCount == 0 && !root );

	// If this fires, the code isn't working or you never allocated memory:
	GLASSERT( memNewCount );
}

void MemHeapCheck()
{
	unsigned long size = 0;
	for ( MemCheckHead* head = root; head; head=head->next )
	{
		MemCheckTail* tail = (MemCheckTail*)((unsigned char*)head + sizeof(MemCheckHead) + head->size);

		GLASSERT( head->magic == MEM_MAGIC0 );
		GLASSERT( tail->magic == MEM_MAGIC1 );
		size += head->size;
	}
	GLASSERT( size == memTotal );
}

void MemStartCheck()
{
	checking = true;
}


#endif // GRINLIZ_DEBUG_MEM

#endif 

#ifdef _WIN32

void WinDebugBreak()
{
	DebugBreak();
}

void dprintf( const char* format, ... )
{
    va_list     va;
    char		buffer[1024];

    //
    //  format and output the message..
    //
    va_start( va, format );
    vsprintf( buffer, format, va );
    va_end( va );

	OutputDebugStringA( buffer );

	printf( "%s", buffer );
	fflush( 0 );
}
#endif
