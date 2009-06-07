#ifndef UFOATTACK_UTIL_INCLUDED
#define UFOATTACK_UTIL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

void CStackEnsureCap( unsigned needInBytes, unsigned* capInBytes, void** stack );

template < class T >
class CStack
{
public:
	CStack( int allocate = 16 ) : stack( 0 ), size( 0 ), capInBytes( 0 ) 
	{
		//CStackEnsureCap( allocate*sizeof(T), &capInBytes, (void**) &stack );	
		CStackEnsureCap( 1, &capInBytes, (void**) &stack );	
	}
	~CStack() {
		free( stack );
	}

	void Push( T t ) {
		if ( capInBytes <= (size+1)*sizeof(T) ) {
			CStackEnsureCap( (size+1)*sizeof(T), &capInBytes, (void**) &stack );	
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

#endif