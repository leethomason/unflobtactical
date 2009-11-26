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
	int det = a * d - b * c;
	GLASSERT( det != 0 );

	inverse->a =  d / det;
	inverse->b = -b / det;
	inverse->c = -c / det;
	inverse->d =  a / det;
	inverse->x   = -( inverse->c * y + inverse->a * x );
	inverse->y   = -( inverse->b * x + inverse->d * y );
}

