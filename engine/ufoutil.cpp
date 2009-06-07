#include "ufoutil.h"
#include "../grinliz/glutil.h"

void CStackEnsureCap( unsigned needInBytes, unsigned* capInBytes, void** stack )
{
	if ( needInBytes > *capInBytes ) {
		*capInBytes = grinliz::CeilPowerOf2( needInBytes );
		*stack = realloc( *stack, *capInBytes );
	}
}
