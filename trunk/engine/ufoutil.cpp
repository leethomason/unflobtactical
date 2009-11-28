#include <stdlib.h>
#include "ufoutil.h"
#include "../grinliz/glutil.h"

void CEnsureCap( unsigned needInBytes, unsigned* capInBytes, void** stack )
{
	if ( needInBytes > *capInBytes ) {
		*capInBytes = grinliz::CeilPowerOf2( needInBytes );
		*stack = realloc( *stack, *capInBytes );
	}
}


void Matrix2I::SetRotation( int r )
{
	switch ( r ) {
		case 0:		a = 1;	b = 0;	c = 0;	d = 1;	break;
		case 90:	a = 0;	b = 1;	c = -1;	d = 0;	break;
		case 180:	a = -1;	b = 0;	c = 0;	d = -1;	break;
		case 270:	a = 0;	b = -1;	c = 1;	d = 0;	break;
		default:
			GLASSERT( 0 );
	};
}


void Matrix2I::Invert( Matrix2I* inverse ) const
{
	/*
	| a b x |-1             |   1d-0y  -(1b-0x)   yb-dx  |
	| c d y |    =  1/DET * | -(1c-0y)   1a-0x  -(ya-cx) |
	| 0 0 1 |               |   0c-0d  -(0a-0b)   da-cb  |

	DET  =  a(1d-0y)-c(1b-0x)+0(yb-dx)
	*/

	int det = a*d - c*b;
	GLASSERT( det != 0 );

	inverse->a =  d / det;
	inverse->b = -b / det;
	inverse->c = -c / det;
	inverse->d =  a / det;
	inverse->x   =  (y*b-d*x) / det;
	inverse->y   = -(y*a-c*x) / det;

#ifdef DEBUG
	Matrix2I test;
	test = (*this) * (*inverse);
	GLASSERT( test.a == 1 && test.d == 1 && test.b == 0 && test.c == 0 && test.x == 0 && test.y == 0 );
#endif
}

