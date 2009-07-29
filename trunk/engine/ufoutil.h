#ifndef UFOATTACK_UTIL_INCLUDED
#define UFOATTACK_UTIL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

void CEnsureCap( unsigned needInBytes, unsigned* capInBytes, void** stack );

/* A stack class for c-structs. (No constructor, no destructor, can be copied.)
 */
template < class T >
class CStack
{
public:
	CStack( int allocate = 16 ) : stack( 0 ), size( 0 ), capInBytes( 0 ) 
	{
		CEnsureCap( allocate*sizeof(T), &capInBytes, (void**) &stack );	
	}
	~CStack() {
		free( stack );
	}

	void Push( T t ) {
		if ( capInBytes <= (size+1)*sizeof(T) ) {
			CEnsureCap( (size+1)*sizeof(T), &capInBytes, (void**) &stack );	
		}
		stack[size++] = t;
	}
	
	void Pop()		{ GLASSERT( size > 0 ); size--; }
	void Clear()	{ size = 0; }
	bool Empty()	{ return size==0; }
	T& Top()  {
		GLASSERT( size > 0 );
		return stack[size-1];
	}

private:
	T* stack;
	unsigned size;
	unsigned capInBytes;
};


/* A stack class for c-structs. (No constructor, no destructor, can be copied.)
 */
template < class T >
class CDynArray
{
public:
	CDynArray( int allocate = 16 ) : vec( 0 ), size( 0 ), capInBytes( 0 ) 
	{
		CEnsureCap( allocate*sizeof(T), &capInBytes, (void**) &vec );	
	}
	~CDynArray() {
		free( vec );
	}

	T& operator[]( int i )				{ GLASSERT( i>=0 && i<(int)size ); return vec[i]; }
	const T& operator[]( int i ) const	{ GLASSERT( i>=0 && i<(int)size ); return vec[i]; }

	void Push( T t ) {
		if ( capInBytes <= (size+1)*sizeof(T) ) {
			CEnsureCap( (size+1)*sizeof(T), &capInBytes, (void**) &vec );	
		}
		vec[size++] = t;
	}
	unsigned Size() const	{ return size; }
	
	void Clear()			{ size = 0; }
	bool Empty() const		{ return size==0; }
	const T* Mem() const	{ return vec; }

private:
	T* vec;
	unsigned size;
	unsigned capInBytes;
};


#endif