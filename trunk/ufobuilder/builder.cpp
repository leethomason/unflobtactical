#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#if defined(__APPLE__)
#include "SDL_image.h"
#endif
#include "SDL_loadso.h"

#include <string>

using namespace std;

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/gldynamic.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glstringutil.h"
#include "../tinyxml/tinyxml.h"

#include "modelbuilder.h"
#include "../importers/import.h"


typedef SDL_Surface* (SDLCALL * PFN_IMG_LOAD) (const char *file);
PFN_IMG_LOAD libIMG_Load;

string outputPath;
string inputPath;

void LoadLibrary()
{
	#if defined(__APPLE__)
		libIMG_Load = &IMG_Load;
	#else
		void* handle = grinliz::grinlizLoadLibrary( "SDL_image" );
		if ( !handle )
		{	
			exit( 1 );
		}
		libIMG_Load = (PFN_IMG_LOAD) grinliz::grinlizLoadFunction( handle, "IMG_Load" );
		GLASSERT( libIMG_Load );
	#endif
}


U32 GetPixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    U8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(U32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}


void ProcessModel( TiXmlElement* model )
{

	int nTotalIndex = 0;
	int nTotalVertex = 0;
	int startIndex = 0;
	int startVertex = 0;

	printf( "Model\n" );

	string filename;
	model->QueryStringAttribute( "filename", &filename );
	string fullIn = inputPath + filename;

	string base, name, extension;
	grinliz::StrSplitFilename( fullIn, &base, &name, &extension );

	string fullOut = outputPath + name + ".mod";

	ModelBuilder* builder = new ModelBuilder();;
	ImportAC3D(	fullIn, builder );

	const VertexGroup* vertexGroup = builder->Groups();

	SDL_RWops* fp = SDL_RWFromFile( fullOut.c_str(), "wb" );
	if ( !fp ) {
		printf( "**Could not open for writing: %s\n", fullOut.c_str() );
		goto graceful_exit;
	}
	else {
		printf( "  Writing: '%s', '%s'\n", name.c_str(), fullOut.c_str() );
	}
	
	for( int i=0; i<builder->NumGroups(); ++i ) {
		nTotalIndex += vertexGroup[i].nIndex;
		nTotalVertex += vertexGroup[i].nVertex;
	}
	printf( "  groups=%d nVertex=%d nIndex=%d\n", builder->NumGroups(), nTotalVertex, nTotalIndex );

	char buffer[16];
	grinliz::StrFillBuffer( name, buffer, 16 );
	SDL_RWwrite( fp, buffer, 16, 1 );

	SDL_WriteBE32( fp, builder->NumGroups() );
	SDL_WriteBE32( fp, nTotalIndex );
	SDL_WriteBE32( fp, nTotalVertex );	

	for( int i=0; i<builder->NumGroups(); ++i ) {
		printf( "    %d: '%s' nVertex=%d nIndex=%d\n",
				i,
				vertexGroup[i].textureName,
				vertexGroup[i].nVertex,
				vertexGroup[i].nIndex );

		grinliz::StrFillBuffer( vertexGroup[i].textureName, buffer, 16 );
		SDL_RWwrite( fp, buffer, 16, 1 );

		SDL_WriteBE32( fp, startVertex );	
		SDL_WriteBE32( fp, vertexGroup[i].nVertex );	
		startVertex += vertexGroup[i].nVertex;

		SDL_WriteBE32( fp, startIndex );	
		SDL_WriteBE32( fp, vertexGroup[i].nIndex );	
		startIndex += vertexGroup[i].nIndex;
	}

	for( int i=0; i<builder->NumGroups(); ++i ) {
		SDL_RWwrite( fp, vertexGroup[i].vertex, sizeof( Vertex ), vertexGroup[i].nVertex );
	}
	for( int i=0; i<builder->NumGroups(); ++i ) {
		for( int j=0; j<vertexGroup[i].nIndex; ++j ) {
			SDL_WriteBE16( fp, vertexGroup[i].index[j] );
		}
	}
	

graceful_exit:
	delete builder;
	if ( fp ) {
		SDL_FreeRW( fp );
	}
}


void ProcessTexture( TiXmlElement* texture )
{
	printf( "Texture\n" );

	string filename;
	texture->QueryStringAttribute( "filename", &filename );

	string fullIn = inputPath + filename;	

	string base, name, extension;
	grinliz::StrSplitFilename( fullIn, &base, &name, &extension );

	string fullOut = outputPath + name + ".tex";


	SDL_Surface* surface = libIMG_Load( fullIn.c_str() );
	if ( !surface ) {
		printf( "**Could not load: %s\n", fullIn.c_str() );
		goto graceful_exit;
	}
	else {
		printf( "  Loaded: '%s' bpp=%d width=%d height=%d\n", 
				fullIn.c_str(),
				surface->format->BitsPerPixel,
				surface->w,
				surface->h );
	}

	SDL_RWops* fp = SDL_RWFromFile( fullOut.c_str(), "wb" );
	if ( !fp ) {
		printf( "**Could not open for writing: %s\n", fullOut.c_str() );
		goto graceful_exit;
	}
	else {
		printf( "  Writing: '%s'\n", fullOut.c_str() );
	}

	char buffer[16];
	grinliz::StrFillBuffer( name, buffer, 16 );
	SDL_RWwrite( fp, buffer, 16, 1 );

	/* PixelFormat */
	#define GL_ALPHA                          0x1906
	#define GL_RGB                            0x1907
	#define GL_RGBA                           0x1908

	/* PixelType */
	#define GL_UNSIGNED_BYTE                  0x1401
	#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
	#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
	#define GL_UNSIGNED_SHORT_5_6_5           0x8363

	U8 r, g, b, a;

	switch( surface->format->BitsPerPixel ) {
		case 32:
			printf( "  RGBA\n" );
			SDL_WriteBE32( fp, GL_RGBA );
			SDL_WriteBE32( fp, GL_UNSIGNED_SHORT_4_4_4_4 );
			SDL_WriteBE32( fp, surface->w );
			SDL_WriteBE32( fp, surface->h );

			// Bottom up!
			for( int j=surface->h-1; j>=0; --j ) {
				for( int i=0; i<surface->w; ++i ) {
					U32 c = GetPixel( surface, i, j );
					SDL_GetRGBA( c, surface->format, &r, &g, &b, &a );

					U16 p =
							  ( ( r>>4 ) << 12 )
							| ( ( g>>4 ) << 8 )
							| ( ( b>>4 ) << 4)
							| ( ( a>>4 ) << 0 );

					SDL_WriteBE16( fp, p );
				}
			}
			break;

		case 24:
			printf( "  RGB\n" );
			SDL_WriteBE32( fp, GL_RGB );
			SDL_WriteBE32( fp, GL_UNSIGNED_SHORT_5_6_5 );
			SDL_WriteBE32( fp, surface->w );
			SDL_WriteBE32( fp, surface->h );

			// Bottom up!
			for( int j=surface->h-1; j>=0; --j ) {
				for( int i=0; i<surface->w; ++i ) {
					U32 c = GetPixel( surface, i, j );
					SDL_GetRGBA( c, surface->format, &r, &g, &b, &a );

					U16 p = 
							  ( ( r>>3 ) << 11 )
							| ( ( g>>2 ) << 5 )
							| ( ( b>>3 ) );

					SDL_WriteBE16( fp, p );
				}
			}
			break;

		case 8:
			printf( "  Alpha\n" );
			SDL_WriteBE32( fp, GL_ALPHA );
			SDL_WriteBE32( fp, GL_UNSIGNED_BYTE );
			SDL_WriteBE32( fp, surface->w );
			SDL_WriteBE32( fp, surface->h );

			// Bottom up!
			for( int j=surface->h-1; j>=0; --j ) {
				for( int i=0; i<surface->w; ++i ) {
				    U8 *p = (U8 *)surface->pixels + j*surface->pitch + i;
					SDL_RWwrite( fp, p, 1, 1 );
				}
			}
			break;

		default:
			printf( "Unsupported bit depth!\n" );
			break;
	}


graceful_exit:
	if ( surface ) { 
		SDL_FreeSurface( surface );
	}
	if ( fp ) {
		SDL_FreeRW( fp );
	}

}


int main( int argc, char* argv[] )
{
	printf( "UFO Builder. argc=%d argv[1]=%s\n", argc, argv[1] );
	if ( argc < 3 ) {
		printf( "Usage: ufobuilder ./inputPath/ inputXMLName\n" );
		exit( 1 );
	}

    SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER );
	LoadLibrary();

	inputPath = argv[1];
	const char* xmlfile = argv[2];
	printf( "Opening, path: '%s' filename: '%s'\n", inputPath.c_str(), xmlfile );
	string input = inputPath + xmlfile;

	TiXmlDocument xmlDoc;
	xmlDoc.LoadFile( input );
	if ( xmlDoc.Error() || !xmlDoc.FirstChildElement() ) {
		printf( "Failed to parse XML file. err=%s\n", xmlDoc.ErrorDesc() );
		exit( 2 );
	}

	xmlDoc.FirstChildElement()->QueryStringAttribute( "output", &outputPath );

	printf( "Output Path: %s\n", inputPath.c_str() );
	printf( "Processing tags:\n" );

	for( TiXmlElement* child = xmlDoc.FirstChildElement()->FirstChildElement();
		 child;
		 child = child->NextSiblingElement() )
	{
		if ( child->ValueStr() == "texture" ) {
			ProcessTexture( child );
		}
		else if ( child->ValueStr() == "model" ) {
			ProcessModel( child );
		}
		else {
			printf( "Unrecognized element: %s\n", child->Value() );
		}
	}

	printf( "All done.\n" );
	SDL_Quit();

	return 0;
}