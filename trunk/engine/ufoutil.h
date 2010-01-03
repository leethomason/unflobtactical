#ifndef UFOATTACK_UTIL_INCLUDED
#define UFOATTACK_UTIL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glfixed.h"

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


/* A dynamic array class for c-structs. (No constructor, no destructor, can be copied.)
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

	T* Push() {
		if ( capInBytes <= (size+1)*sizeof(T) ) {
			CEnsureCap( (size+1)*sizeof(T), &capInBytes, (void**) &vec );	
		}
		size++;
		return &vec[size-1];
	}

	T* PushArr( int count ) {
		if ( capInBytes <= (size+count)*sizeof(T) ) {
			CEnsureCap( (size+count)*sizeof(T), &capInBytes, (void**) &vec );	
		}
		T* result = &vec[size];
		size += count;
		return result;
	}

	void Pop() {
		GLASSERT( size > 0 );
		--size;
	}

	void SwapRemove( int i ) {
		GLASSERT( i<(int)size );
		GLASSERT( size > 0 );
		grinliz::Swap( &vec[i], &vec[size-1] );
		Pop();
	}

	int Size() const		{ return (int)size; }
	
	void Clear()			{ size = 0; }
	bool Empty() const		{ return size==0; }
	const T* Mem() const	{ return vec; }
	T* Mem()				{ return vec; }

private:
	T* vec;
	unsigned size;
	unsigned capInBytes;
};





/* A fixed array class for c-structs.
 */
template < class T, int CAPACITY >
class CArray
{
public:
	CArray() : size( 0 )	{}
	~CArray()				{}

	T& operator[]( int i )				{ GLASSERT( i>=0 && i<(int)size ); return vec[i]; }
	const T& operator[]( int i ) const	{ GLASSERT( i>=0 && i<(int)size ); return vec[i]; }

	void Push( T t ) {
		GLASSERT( size < CAPACITY );
		vec[size++] = t;
	}
	T* Push() {
		GLASSERT( size < CAPACITY );
		size++;
		return &vec[size-1];
	}
	unsigned Size() const	{ return size; }
	
	void Clear()			{ size = 0; }
	bool Empty() const		{ return size==0; }
	const T* Mem() const	{ return vec; }

private:
	T vec[CAPACITY];
	unsigned size;
};


class Matrix2I
{
  public:
	int a, b, c, d, x, y;

	Matrix2I()								{	SetIdentity();	}
	Matrix2I( const Matrix2I& rhs )			{	a=rhs.a; b=rhs.b; c=rhs.c; d=rhs.d; x=rhs.x; y=rhs.y; }
	void operator=( const Matrix2I& rhs )	{	a=rhs.a; b=rhs.b; c=rhs.c; d=rhs.d; x=rhs.x; y=rhs.y; }

	// Set the matrix to identity
	void SetIdentity()		{	a=1; b=0; c=0; d=1; x=0; y=0; }

	// valid inputs are 0, 90, 180, and 270
	void SetRotation( int r );
	
	void Invert( Matrix2I* inverse ) const;

	bool operator==( const Matrix2I& rhs ) const	{ 
		return ( a==rhs.a && b==rhs.b && c==rhs.c && d==rhs.d && x==rhs.x && y==rhs.y );
	}

	bool operator!=( const Matrix2I& rhs ) const	{ 
		return !( a==rhs.a && b==rhs.b && c==rhs.c && d==rhs.d && x==rhs.x && y==rhs.y );
	}

	friend void MultMatrix2I( const Matrix2I& a, const Matrix2I& b, Matrix2I* c );
	friend void MultMatrix2I( const Matrix2I& a, const grinliz::Vector3I& b, grinliz::Vector3I* c );
	
	friend Matrix2I operator*( const Matrix2I& a, const Matrix2I& b )
	{	
		Matrix2I result;
		MultMatrix2I( a, b, &result );
		return result;
	}
	friend grinliz::Vector3I operator*( const Matrix2I& a, const grinliz::Vector3I& b )
	{
		grinliz::Vector3I result;
		MultMatrix2I( a, b, &result );
		return result;
	}
};


inline void MultMatrix2I( const Matrix2I& x, const Matrix2I& y, Matrix2I* w )
{
	// This does not support the target being one of the sources.
	GLASSERT( w != &x && w != &y && &x != &y );

	// a b x
	// c d y
	// 0 0 1

	w->a = x.a*y.a + x.b*y.c;
	w->b = x.a*y.b + x.b*y.d;
	w->c = x.c*y.a + x.d*y.c;
	w->d = x.c*y.b + x.d*y.d;
	w->x = x.a*y.x + x.b*y.y + x.x;
	w->y = x.c*y.x + x.d*y.y + x.y;
}


inline void MultMatrix2I( const Matrix2I& x, const grinliz::Vector3I& y, grinliz::Vector3I* w )
{
	GLASSERT( w != &y );
	w->x = x.a*y.x + x.b*y.y + x.x*y.z;
	w->y = x.c*y.x + x.d*y.y + x.y*y.z;
	w->z =                         y.z;
}

/*
	A class to walk a line between 2 integer points.
	
	ex#1:	A line from (0,0) to (1,1) has 0 steps.
			P(0,0), Q(1,1)
	ex#2:	A line from (0,0) to (2,0) has 1 steps.
			P(0,0), Q(1,0)
			P(1,0), Q(2,0)
	In general:
		for( ; line.CurrentStep() < (line.NumSteps()-1); line.Step() )

*/
class LineWalk
{
public:
	LineWalk( int x0, int y0, int x1, int y1 );
	// (x,y) is the current location, (nx,ny) is the next location.
	int X() const { return (int)p.x; }
	int Y() const { return (int)p.y; }
	int NX() const { return (int)q.x; }
	int NY() const { return (int)q.y; }

	grinliz::Vector2I P() const { grinliz::Vector2I p = { X(), Y() }; return p; }
	grinliz::Vector2I Q() const { grinliz::Vector2I q = { NX(), NY() }; return q; }

	void Step( int n=1 );

	int CurrentStep() const		{ return step; }
	int NumSteps() const		{ return nSteps; }

private:
	int axis;		// 1 if y is major axis. 0 if x.
	int axisDir;	// +1 if positive on the major axis, -1 if negative on the major axis
	int nSteps;		// # of steps in the walk
	int step;		// step==nSteps then done
	grinliz::Fixed delta;	// step in fixed point
	grinliz::Vector2<grinliz::Fixed> p;
	grinliz::Vector2<grinliz::Fixed> q;
};

#endif