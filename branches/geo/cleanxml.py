# Python program to set the version.
##############################################
import sys


def process( name ):
	filestream = open( name, 'r' )
	if filestream.closed:
		print( "file " + name + " not open." )
		return

	output = ""
	while 1:
		line = filestream.readline()
		if not line: break
		output += line
	filestream.close()
	
	if not output: return			# basic error checking

	result = output.replace( '\\"', '"' );
	
	print( "Writing file " + name )
	filestream = open( name, "w" );
	filestream.write( result );
	filestream.close()

process( sys.argv[1] )

	
