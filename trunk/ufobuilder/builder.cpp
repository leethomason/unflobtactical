/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#if defined(__APPLE__)
#include "SDL_image.h"
#endif
#include "SDL_loadso.h"
#include "../engine/enginelimits.h"

#include <string>

using namespace std;

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/gldynamic.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glstringutil.h"
#include "../tinyxml/tinyxml.h"

#include "modelbuilder.h"
#include "../engine/serialize.h"
#include "../importers/import.h"


typedef SDL_Surface* (SDLCALL * PFN_IMG_LOAD) (const char *file);
PFN_IMG_LOAD libIMG_Load;

string outputPath;
string inputPath;
int totalModelMem = 0;
int totalTextureMem = 0;
int totalMapMem = 0;


bool StrEqual( const char* a, const char* b ) {
	if ( a && b && strcmp( a, b ) == 0 )
		return true;
	return false;
}

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



void WriteFloat( SDL_RWops* ctx, float f )
{
	SDL_RWwrite( ctx, &f, sizeof(float), 1 );
}


void TextureHeader::Save( SDL_RWops* fp )
{
	SDL_RWwrite( fp, this, sizeof(TextureHeader), 1 );
}

void TextureHeader::Set( const char* name, U32 format, U32 type, U16 width, U16 height )
{
	memset( this->name, 0, EL_FILE_STRING_LEN );
	strncpy( this->name, name, EL_FILE_STRING_LEN );
	this->format = format;
	this->type = type;
	this->width = width;
	this->height = height;
}


void ModelHeader::Save( SDL_RWops* fp )
{
	SDL_RWwrite( fp, this, sizeof(ModelHeader), 1 );
}

void ModelHeader::Set(	const char* name, int nGroups, int nTotalVertices, int nTotalIndices,
						const grinliz::Rectangle3F& bounds )
{
	GLASSERT( nGroups > 0 && nGroups < EL_MAX_MODEL_GROUPS );
	GLASSERT( nTotalVertices > 0 && nTotalVertices < EL_MAX_VERTEX_IN_MODEL );
	GLASSERT( EL_MAX_VERTEX_IN_MODEL <= 0xffff );
	GLASSERT( nTotalIndices > 0 && nTotalIndices < EL_MAX_INDEX_IN_MODEL );
	GLASSERT( EL_MAX_INDEX_IN_MODEL <= 0xffff );
	GLASSERT( nTotalVertices <= nTotalIndices );

	memset( this->name, 0, EL_FILE_STRING_LEN );
	strncpy( this->name, name, EL_FILE_STRING_LEN );
	this->flags = 0;
	this->nGroups = nGroups;
	this->nTotalVertices = nTotalVertices;
	this->nTotalIndices = nTotalIndices;
	this->bounds = bounds;
}


void ModelGroup::Save( SDL_RWops* fp )
{
	SDL_RWwrite( fp, this, sizeof(ModelGroup), 1 );
}


void ModelGroup::Set( const char* textureName, int nVertex, int nIndex )
{
	GLASSERT( nVertex > 0 && nVertex < EL_MAX_VERTEX_IN_MODEL );
	GLASSERT( nIndex > 0 && nIndex < EL_MAX_INDEX_IN_MODEL );
	GLASSERT( nVertex <= nIndex );

	memset( this->textureName, 0, EL_FILE_STRING_LEN );
	strncpy( this->textureName, textureName, EL_FILE_STRING_LEN );

	this->nVertex = nVertex;
	this->nIndex = nIndex;
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


void ProcessMap( TiXmlElement* map )
{

	string filename;
	map->QueryStringAttribute( "filename", &filename );
	string fullIn = inputPath + filename;

	string base, name, extension;
	grinliz::StrSplitFilename( fullIn, &base, &name, &extension );
	string fullOut = outputPath + name + ".map";

	// copy the entire file.
	FILE* read = fopen( fullIn.c_str(), "rb" );
	if ( !read ) {
		printf( "**Unrecognized map file. full='%s' base='%s' name='%s' extension='%s'\n",
				 fullIn.c_str(), base.c_str(), name.c_str(), extension.c_str() );
		exit( 1 );
	}

	FILE* write = fopen( fullOut.c_str(), "wb" );
	GLASSERT( write );

	// length of file.
	fseek( read, 0, SEEK_END );
	int len = ftell( read );
	fseek( read, 0, SEEK_SET );

	char* mem = new char[len];
	fread( mem, len, 1, read );
	fwrite( mem, len, 1, write );
	delete [] mem;

	printf( "Map '%s' memory=%dk\n", filename.c_str(), len/1024 );
	totalMapMem += len;

	fclose( read );
	fclose( write );
}


void ProcessModel( TiXmlElement* model )
{
	int nTotalIndex = 0;
	int nTotalVertex = 0;
	int startIndex = 0;
	int startVertex = 0;

	printf( "Model " );

	string filename;
	model->QueryStringAttribute( "filename", &filename );
	string fullIn = inputPath + filename;

	string base, name, extension;
	grinliz::StrSplitFilename( fullIn, &base, &name, &extension );
	string fullOut = outputPath + name + ".mod";

	ModelBuilder* builder = new ModelBuilder();
	if ( extension == ".ac" ) {
		ImportAC3D(	fullIn, builder );
	}
	else if ( extension == ".off" ) {
		ImportOFF( fullIn, builder );
	}
	else {
		printf( "**Unrecognized model file. full='%s' base='%s' name='%s' extension='%s'\n",
				 fullIn.c_str(), base.c_str(), name.c_str(), extension.c_str() );
		exit( 1 );
	}

	const VertexGroup* vertexGroup = builder->Groups();

	SDL_RWops* fp = SDL_RWFromFile( fullOut.c_str(), "wb" );
	if ( !fp ) {
		printf( "**Could not open for writing: %s\n", fullOut.c_str() );
		exit( 1 );
	}
	else {
		//printf( "  Writing: '%s', '%s'", name.c_str(), fullOut.c_str() );
		printf( "  '%s'", name.c_str() );
	}
	
	for( int i=0; i<builder->NumGroups(); ++i ) {
		nTotalIndex += vertexGroup[i].nIndex;
		nTotalVertex += vertexGroup[i].nVertex;
	}
	printf( " groups=%d nVertex=%d nTri=%d\n", builder->NumGroups(), nTotalVertex, nTotalIndex/3 );

	
	ModelHeader header;
	header.Set( name.c_str(), builder->NumGroups(), nTotalVertex, nTotalIndex, builder->Bounds() );

	if ( StrEqual( model->Attribute( "billboard" ), "true" ) ) {
		header.flags |= ModelHeader::BILLBOARD;
	}

	header.Save( fp );

	int totalMemory = 0;

	for( int i=0; i<builder->NumGroups(); ++i ) {
		int mem = vertexGroup[i].nVertex*sizeof(Vertex) + vertexGroup[i].nIndex*2;
		totalMemory += mem;
		printf( "    %d: '%s' nVertex=%d nTri=%d memory=%.1fk\n",
				i,
				vertexGroup[i].textureName,
				vertexGroup[i].nVertex,
				vertexGroup[i].nIndex / 3,
				(float)mem/1024.0f );

		ModelGroup group;
		group.Set( vertexGroup[i].textureName, vertexGroup[i].nVertex, vertexGroup[i].nIndex );
		group.Save( fp );

		startVertex += vertexGroup[i].nVertex;
		startIndex += vertexGroup[i].nIndex;
	}

	// Write the vertices in each group:
	for( int i=0; i<builder->NumGroups(); ++i ) {

		SDL_RWwrite( fp, vertexGroup[i].vertex, sizeof(Vertex), vertexGroup[i].nVertex );

	}
	// Write the indices in each group:
	for( int i=0; i<builder->NumGroups(); ++i ) {
		SDL_RWwrite( fp, vertexGroup[i].index, sizeof(U16), vertexGroup[i].nIndex );
	}
	printf( "  total memory=%.1fk\n", (float)totalMemory / 1024.f );
	totalModelMem += totalMemory;
	
	delete builder;
	if ( fp ) {
		SDL_FreeRW( fp );
	}
}


void ProcessTexture( TiXmlElement* texture )
{
	printf( "Texture" );

	string filename;
	texture->QueryStringAttribute( "filename", &filename );

	string fullIn = inputPath + filename;	

	string base, name, extension;
	grinliz::StrSplitFilename( fullIn, &base, &name, &extension );

	string fullOut = outputPath + name + ".tex";
	SDL_RWops* fp = 0;


	SDL_Surface* surface = libIMG_Load( fullIn.c_str() );
	if ( !surface ) {
		printf( "**Could not load: %s\n", fullIn.c_str() );
		goto graceful_exit;
	}
	else {
		printf( "  Loaded: '%s' bpp=%d width=%d height=%d", 
				name.c_str(), //fullIn.c_str(),
				surface->format->BitsPerPixel,
				surface->w,
				surface->h );
	}

	fp = SDL_RWFromFile( fullOut.c_str(), "wb" );
	if ( !fp ) {
		printf( "**Could not open for writing: %s\n", fullOut.c_str() );
		goto graceful_exit;
	}
	else {
		//printf( "  Writing: '%s'\n", fullOut.c_str() );
	}

	TextureHeader header;

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
			printf( "  RGBA memory=%dk\n", (surface->w * surface->h * 2)/1024 );
			totalTextureMem += (surface->w * surface->h * 2);
			header.Set( name.c_str(), GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, surface->w, surface->h );
			header.Save( fp );

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

					SDL_WriteLE16( fp, p );
				}
			}
			break;

		case 24:
			printf( "  RGB memory=%dk\n", (surface->w * surface->h * 2)/1024 );
			totalTextureMem += (surface->w * surface->h * 2);
			header.Set( name.c_str(), GL_RGB, GL_UNSIGNED_SHORT_5_6_5, surface->w, surface->h );
			header.Save( fp );

			// Bottom up!
			for( int j=surface->h-1; j>=0; --j ) {
				for( int i=0; i<surface->w; ++i ) {
					U32 c = GetPixel( surface, i, j );
					SDL_GetRGBA( c, surface->format, &r, &g, &b, &a );

					U16 p = 
							  ( ( r>>3 ) << 11 )
							| ( ( g>>2 ) << 5 )
							| ( ( b>>3 ) );

					SDL_WriteLE16( fp, p );
				}
			}
			break;

		case 8:
			printf( "  Alpha memory=%dk\n", (surface->w * surface->h * 1)/1024 );
			totalTextureMem += (surface->w * surface->h * 1);
			header.Set( name.c_str(), GL_ALPHA, GL_UNSIGNED_BYTE, surface->w, surface->h );
			header.Save( fp );

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
		else if ( child->ValueStr() == "map" ) {
			ProcessMap( child );
		}
		else {
			printf( "Unrecognized element: %s\n", child->Value() );
		}
	}

	int total = totalTextureMem + totalModelMem + totalMapMem;

	printf( "Total memory=%dk Texture=%dk Model=%dk Map=%dk\n", 
			total/1024, totalTextureMem/1024, totalModelMem/1024, totalMapMem/1024 );
	printf( "All done.\n" );
	SDL_Quit();

	return 0;
}