
#include "../shared/gamedbreader.h"
using namespace gamedb;

int main( int argc, const char* argv[] )
{
	Reader reader;

	reader.Init( 0, argv[1] );
	reader.RecWalk( reader.Root(), 0 );

	return 0;
}

