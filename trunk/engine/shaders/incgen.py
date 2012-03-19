# read a set of text files, and create an include file for c++ 
# example: incgen shader.inc shader.frag shader.vert

import sys;

print "Usage: incgen target-file source0 source1 source2..."

fileWrite = open( sys.argv[1], 'w' )

for i in range( 2, len( sys.argv ) ):
	fileRead = open( sys.argv[i], 'r')

	compArr = sys.argv[i].split( '.' )
	fileWrite.write( "static const char* " + compArr[0] + "_" + compArr[1] + ' = {\n' )
	print( "Writing:" + sys.argv[i] );
	
	for line in fileRead:
		line = line.rstrip( '\n' )
		if ( len( line ) > 0 ):
			str = '    "' + line + '\\n"' + '\n'
			fileWrite.write( str )
	fileWrite.write( "};\n" );
	fileRead.close()

print "File write complete."
