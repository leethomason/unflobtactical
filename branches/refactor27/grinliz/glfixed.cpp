#if 0
#include "glfixed.h"
using namespace grinliz;

/*static*/ bool Fixed::approxSqrtTableInit = false;
/*static*/ Fixed Fixed::approxSqrtTable[ SQRT_TABLE_SIZE ];

Fixed Fixed::ApproxSqrt()
{
	if ( !approxSqrtTableInit ) {
		for( int i=0; i<SQRT_TABLE_SIZE; ++i ) {
			Fixed v = Fixed(i) / Fixed( SQRT_TABLE_SIZE-1 );
			approxSqrtTable[i] = v.Sqrt();
		}
		approxSqrtTableInit = true;
	}

	GLASSERT( x >= 0 && x <= 65536 );

	//static const 1.0/(SQRT_TABLE_SIZE-1) )
	static const S32 INV = 1040;
	S32 i = INV * x;
	i >>= 10;	// 2^16 -> 2^6
	GLASSERT( i>=0 && i<SQRT_TABLE_SIZE );
	return approxSqrtTable[i];
}
#endif