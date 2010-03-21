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

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../engine/enginelimits.h"

#include <string>
#include <vector>

#include "../grinliz/gldynamic.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glstringutil.h"
#include "../tinyxml/tinyxml.h"

#include "modelbuilder.h"
#include "../engine/serialize.h"
#include "../importers/import.h"

#include "../sqlite3/sqlite3.h"
#include "../shared/gldatabase.h"
#include "../grinliz/glstringutil.h"
#include "dither.h"

using namespace std;
using namespace grinliz;

typedef SDL_Surface* (SDLCALL * PFN_IMG_LOAD) (const char *file);
PFN_IMG_LOAD libIMG_Load;

string outputPath;
string outputDB;
vector<U16> pixelBuffer16;
vector<U8> pixelBuffer8;

string inputPath;
int totalModelMem = 0;
int totalTextureMem = 0;
int totalMapMem = 0;

sqlite3* db = 0;
BinaryDBWriter *writer = 0;

const int GLYPH_CX = 16;
const int GLYPH_CY = 8;
const int GLYPH_WIDTH = 256 / GLYPH_CX;
const int GLYPH_HEIGHT = 128 / GLYPH_CY;

struct GlyphMetric
{
	U16 offset;
	U16 width;
};
GlyphMetric gGlyphMetric[GLYPH_CX*GLYPH_CY];

void CreateDatabase()
{
	int sqlResult = 0;

	// Set up the database tables.
	sqlResult = sqlite3_exec(	db, 
								"CREATE TABLE texture "
								"(name TEXT NOT NULL PRIMARY KEY, "
								" format TEXT, image INT, width INT, height INT, id INT, metricsID INT);",
								0, 0, 0 );
	GLASSERT( sqlResult == SQLITE_OK );

	sqlResult	= sqlite3_exec(	db,
								"CREATE TABLE model "
								"(name TEXT NOT NULL PRIMARY KEY, "
								" headerID INT, "
								" groupStart INT, nGroup INT, "
								" vertexID INT, indexID INT );",
								0, 0, 0 );
	GLASSERT( sqlResult == SQLITE_OK );

	sqlResult	= sqlite3_exec(	db,
								"CREATE TABLE modelGroup "
								"(id INT NOT NULL PRIMARY KEY, "
								" textureName TEXT, "
								" nVertex INT, nIndex INT );",
								0, 0, 0 );
	GLASSERT( sqlResult == SQLITE_OK );

	sqlResult	= sqlite3_exec(	db,
								"CREATE TABLE map "
								"(name TEXT NOT NULL PRIMARY KEY, "
								" id INT );",
								0, 0, 0 );
	GLASSERT( sqlResult == SQLITE_OK );

}


void InsertTextureToDB( const char* name, const char* format, bool isImage, bool metrics, int width, int height, const void* pixels, int sizeInBytes )
{
	int index = 0, metricsIndex=0;
	writer->Write( pixels, sizeInBytes, &index );
	if ( metrics ) {
		writer->Write( gGlyphMetric, sizeof( GlyphMetric )*GLYPH_CX*GLYPH_CY, &metricsIndex );
	}

	sqlite3_stmt* stmt = NULL;
	sqlite3_prepare_v2( db, "INSERT INTO texture VALUES (?, ?, ?, ?, ?, ?, ?);", -1, &stmt, 0 );

	sqlite3_bind_text( stmt, 1, name, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 2, format, -1, SQLITE_TRANSIENT );
	sqlite3_bind_int( stmt, 3, isImage ? 1 : 0 );
	sqlite3_bind_int( stmt, 4, width );
	sqlite3_bind_int( stmt, 5, height );
	sqlite3_bind_int( stmt, 6, index );
	sqlite3_bind_int( stmt, 7, metricsIndex );

	sqlite3_step( stmt );
	sqlite3_finalize(stmt);
}


void InsertModelHeaderToDB( const ModelHeader& header, int groupID, int vertexID, int indexID )
{
	int index = 0;
	writer->Write( &header, sizeof(header), &index );

	sqlite3_stmt* stmt = NULL;

	sqlite3_prepare_v2( db, "INSERT INTO model VALUES (?,?,?,?,?,?);", -1, &stmt, 0 );
	sqlite3_bind_text( stmt, 1,	header.name, -1, SQLITE_TRANSIENT );
	sqlite3_bind_int(  stmt, 2, index );
	sqlite3_bind_int(  stmt, 3, groupID );
	sqlite3_bind_int(  stmt, 4, header.nGroups );
	sqlite3_bind_int(  stmt, 5, vertexID );
	sqlite3_bind_int(  stmt, 6, indexID );
	sqlite3_step( stmt );
	sqlite3_finalize(stmt);
}


void InsertModelGroupToDB( const ModelGroup& group, int *groupID )
{
	static int groupIDPool = 1;
	sqlite3_stmt* stmt = NULL;
	*groupID = groupIDPool;

	sqlite3_prepare_v2( db, "INSERT INTO modelGroup VALUES (?,?,?,?);", -1, &stmt, 0 );
	sqlite3_bind_int(  stmt, 1, groupIDPool );
	sqlite3_bind_text( stmt, 2,	group.textureName, -1, SQLITE_TRANSIENT );
	sqlite3_bind_int(  stmt, 3, group.nVertex );
	sqlite3_bind_int(  stmt, 4, group.nIndex );
	sqlite3_step( stmt );
	sqlite3_finalize(stmt);

	++groupIDPool;
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
	StrNCpy( this->name, name, EL_FILE_STRING_LEN );
	this->flags = 0;
	this->nGroups = nGroups;
	this->nTotalVertices = nTotalVertices;
	this->nTotalIndices = nTotalIndices;
	this->bounds = bounds;
	trigger.Set( 0, 0, 0 );
	eye = 0;
	target = 0;
}


void ModelGroup::Set( const char* textureName, int nVertex, int nIndex )
{
	GLASSERT( nVertex > 0 && nVertex < EL_MAX_VERTEX_IN_MODEL );
	GLASSERT( nIndex > 0 && nIndex < EL_MAX_INDEX_IN_MODEL );
	GLASSERT( nVertex <= nIndex );

	memset( this->textureName, 0, EL_FILE_STRING_LEN );
	StrNCpy( this->textureName, textureName, EL_FILE_STRING_LEN );

	this->nVertex = nVertex;
	this->nIndex = nIndex;
}


U32 GetPixel( const SDL_Surface *surface, int x, int y)
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


void PutPixel(SDL_Surface *surface, int x, int y, U32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    U8 *p = (U8*)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(U16*)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(U32*)p = pixel;
        break;
    }
}


void ProcessMap( TiXmlElement* map )
{
	string filename;
	map->QueryStringAttribute( "filename", &filename );
	string fullIn = inputPath + filename;

	string name;
	grinliz::StrSplitFilename( fullIn, 0, &name, 0 );

	// copy the entire file.
#pragma warning ( push )
#pragma warning ( disable : 4996 )	// fopen is unsafe. For video games that seems extreme.
	FILE* read = fopen( fullIn.c_str(), "rb" );
#pragma warning (pop)

	if ( !read ) {
		printf( "**Unrecognized map file. '%s'\n",
				 fullIn.c_str() );
		exit( 1 );
	}

	// length of file.
	fseek( read, 0, SEEK_END );
	int len = ftell( read );
	fseek( read, 0, SEEK_SET );

	char* mem = new char[len];
	fread( mem, len, 1, read );
	//fwrite( mem, len, 1, write );

	int index = 0;
	writer->Write( mem, len, &index );

	sqlite3_stmt* stmt = NULL;
	sqlite3_prepare_v2( db, "INSERT INTO map VALUES (?,?);", -1, &stmt, 0 );
	sqlite3_bind_text( stmt, 1,	name.c_str(), -1, SQLITE_TRANSIENT );
	sqlite3_bind_int(  stmt, 2, index );
	sqlite3_step( stmt );
	sqlite3_finalize(stmt);

	delete [] mem;

	printf( "Map '%s' memory=%dk\n", filename.c_str(), len/1024 );
	totalMapMem += len;

	fclose( read );
}


void ProcessModel( TiXmlElement* model )
{
	int nTotalIndex = 0;
	int nTotalVertex = 0;
	int startIndex = 0;
	int startVertex = 0;


	string filename;
	model->QueryStringAttribute( "filename", &filename );
	string fullIn = inputPath + filename;

	string base, name, extension;
	grinliz::StrSplitFilename( fullIn, &base, &name, &extension );
	//string fullOut = outputPath + name + ".mod";

	bool smoothShading = false;
	if ( grinliz::StrEqual( model->Attribute( "shading" ), "smooth" ) ) {
		smoothShading = true;
	}

	printf( "Model '%s'", name.c_str() );

	ModelBuilder* builder = new ModelBuilder();
	builder->SetShading( smoothShading );

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
	
	for( int i=0; i<builder->NumGroups(); ++i ) {
		nTotalIndex += vertexGroup[i].nIndex;
		nTotalVertex += vertexGroup[i].nVertex;
	}
	printf( " groups=%d nVertex=%d nTri=%d\n", builder->NumGroups(), nTotalVertex, nTotalIndex/3 );

	
	ModelHeader header;
	header.Set( name.c_str(), builder->NumGroups(), nTotalVertex, nTotalIndex, builder->Bounds() );

	if ( grinliz::StrEqual( model->Attribute( "billboard" ), "true" ) ) {
		header.flags |= ModelHeader::BILLBOARD;
		// Make the bounds square.
		float d = grinliz::Max( -header.bounds.min.x, -header.bounds.min.z, header.bounds.max.x, header.bounds.max.z );
		header.bounds.min.x = -d;
		header.bounds.min.z = -d;
		header.bounds.max.x = d;
		header.bounds.max.z = d;
	}
	if ( grinliz::StrEqual( model->Attribute( "shadow" ), "rotate" ) ) {
		header.flags |= ModelHeader::ROTATE_SHADOWS;
	}
	if ( grinliz::StrEqual( model->Attribute( "origin" ), "upperLeft" ) ) {
		header.flags |= ModelHeader::UPPER_LEFT;
	}
	if ( model->Attribute( "trigger" ) ) {
#pragma warning ( disable : 4996 )
		sscanf( model->Attribute( "trigger" ), "%f %f %f", &header.trigger.x, &header.trigger.y, &header.trigger.z );
	}
	if ( model->Attribute( "eye" ) ) {
		model->QueryFloatAttribute( "eye", &header.eye );
	}
	if ( model->Attribute( "target" ) ) {
		model->QueryFloatAttribute( "target", &header.target );
	}

	//header.Save( fp );
	int groupID = 0;
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
		//group.Save( fp );
		int id=0;
		InsertModelGroupToDB( group, &id );
		if ( i==0 )
			groupID = id;

		startVertex += vertexGroup[i].nVertex;
		startIndex += vertexGroup[i].nIndex;
	}

	Vertex* vertexBuf = new Vertex[nTotalVertex];
	U16* indexBuf = new U16[nTotalIndex];

	Vertex* pVertex = vertexBuf;
	U16* pIndex = indexBuf;
	
	const Vertex* pVertexEnd = vertexBuf + nTotalVertex;
	const U16* pIndexEnd = indexBuf + nTotalIndex;

	// Write the vertices in each group:
	for( int i=0; i<builder->NumGroups(); ++i ) {
		//SDL_RWwrite( fp, vertexGroup[i].vertex, sizeof(Vertex), vertexGroup[i].nVertex );

		GLASSERT( pVertex + vertexGroup[i].nVertex <= pVertexEnd );
		memcpy( pVertex, vertexGroup[i].vertex, sizeof(Vertex)*vertexGroup[i].nVertex );
		pVertex += vertexGroup[i].nVertex;
	}
	// Write the indices in each group:
	for( int i=0; i<builder->NumGroups(); ++i ) {
		//SDL_RWwrite( fp, vertexGroup[i].index, sizeof(U16), vertexGroup[i].nIndex );

		GLASSERT( pIndex + vertexGroup[i].nIndex <= pIndexEnd );
		memcpy( pIndex, vertexGroup[i].index, sizeof(U16)*vertexGroup[i].nIndex );
		pIndex += vertexGroup[i].nIndex;
	}
	GLASSERT( pVertex == pVertexEnd );
	GLASSERT( pIndex == pIndexEnd );

	int vertexID = 0, indexID = 0;

	writer->Write( vertexBuf, nTotalVertex*sizeof(Vertex), &vertexID );
	writer->Write( indexBuf,  nTotalIndex*sizeof(U16),     &indexID );
	InsertModelHeaderToDB( header, groupID, vertexID, indexID );

	printf( "  total memory=%.1fk\n", (float)totalMemory / 1024.f );
	totalModelMem += totalMemory;
	
	delete [] vertexBuf;
	delete [] indexBuf;
	delete builder;
}


bool BarIsBlack( SDL_Surface* surface, int x, int y0, int y1 )
{
	U8 r, g, b, a;
	bool black = true;

	for( int y=y0; y<=y1; ++y ) {
		U32 c = GetPixel( surface, x, y );
		SDL_GetRGBA( c, surface->format, &r, &g, &b, &a );
		if ( a == 0 || ( r==0 && g==0 && b==0 ) ) {
			// do nothing.
		}
		else {
			black = false;
			break;
		}
	}
	return black;
}

void CalcFontWidths( SDL_Surface* surface )
{
	// Walk all the glyphs, calculate the minimum width
	for( int gy=0; gy<GLYPH_CY; ++gy ) {
		for( int gx=0; gx<GLYPH_CX; ++gx ) {
	
			// In image space.
			int y0 = gy*GLYPH_HEIGHT;
			int y1 = y0 + GLYPH_HEIGHT - 1;
			int x0 = gx*GLYPH_WIDTH;
			int x1 = x0 + GLYPH_WIDTH-1;

			int x0p = x0;
			while( x0p < x1 && BarIsBlack( surface, x0p, y0, y1 ) ) {
				++x0p;
			}
			int x1p = x1;
			while ( x1p >= x0p && BarIsBlack( surface, x1p, y0, y1 ) ) {
				--x1p;
			}
			GlyphMetric* g = &gGlyphMetric[gy*GLYPH_CX+gx];
			g->offset = x0p - x0;
			g->width = x1p - x0p + 1;
			GLASSERT( g->offset >= 0 && g->offset < GLYPH_WIDTH );
			GLASSERT( g->width >= 0 && g->width <= GLYPH_WIDTH );
			GLOUTPUT(( "glyph %d offset=%d width=%d\n", gy*GLYPH_CX+gx, g->offset, g->width ));
		}
	}
}


void ProcessTexture( TiXmlElement* texture )
{
	bool isImage = false;
	if ( texture->ValueStr() == "image" ) {
		isImage = true;
		printf( "Image" );
	}
	else {
		printf( "Texture" );
	}

	bool dither = true;
	bool isFont = false;
	if ( StrEqual( texture->Attribute( "dither" ), "false" ) ) {
		dither = false;
	}
	if ( StrEqual( texture->Attribute( "font" ), "true" ) ) {
		isFont = true;
	}

	string filename;
	texture->QueryStringAttribute( "filename", &filename );

	string fullIn = inputPath + filename;	

	string base, name, extension;
	grinliz::StrSplitFilename( fullIn, &base, &name, &extension );

	SDL_Surface* surface = libIMG_Load( fullIn.c_str() );
	if ( !surface ) {
		printf( "**Could not load: %s\n", fullIn.c_str() );
		exit( 1 );
	}
	else {
		printf( "  Loaded: '%s' bpp=%d width=%d height=%d", 
				name.c_str(),
				surface->format->BitsPerPixel,
				surface->w,
				surface->h );
	}

	if ( isFont ) {
		CalcFontWidths( surface );
	}

	U8 r, g, b, a;
	pixelBuffer16.resize(0);
	pixelBuffer8.resize(0);

	switch( surface->format->BitsPerPixel ) {
		case 32:
			printf( "  RGBA memory=%dk\n", (surface->w * surface->h * 2)/1024 );
			totalTextureMem += (surface->w * surface->h * 2);

			// Bottom up!
			if ( !dither ) {
				for( int j=surface->h-1; j>=0; --j ) {
					for( int i=0; i<surface->w; ++i ) {
						U32 c = GetPixel( surface, i, j );
						SDL_GetRGBA( c, surface->format, &r, &g, &b, &a );

						U16 p =
								  ( ( r>>4 ) << 12 )
								| ( ( g>>4 ) << 8 )
								| ( ( b>>4 ) << 4)
								| ( ( a>>4 ) << 0 );

						pixelBuffer16.push_back(p);
					}
				}
			}
			else {
				pixelBuffer16.resize( surface->w*surface->h );
				pixelBuffer16.reserve( surface->w*surface->h );
				DitherTo16( surface, RGBA16, true, &pixelBuffer16[0] );
			}
			InsertTextureToDB( name.c_str(), "RGBA16", isImage, isFont, surface->w, surface->h, &pixelBuffer16[0], pixelBuffer16.size()*2 );
			break;

		case 24:
			printf( "  RGB memory=%dk\n", (surface->w * surface->h * 2)/1024 );
			totalTextureMem += (surface->w * surface->h * 2);

			// Bottom up!
			if ( !dither ) {
				for( int j=surface->h-1; j>=0; --j ) {
					for( int i=0; i<surface->w; ++i ) {
						U32 c = GetPixel( surface, i, j );
						SDL_GetRGBA( c, surface->format, &r, &g, &b, &a );

						U16 p = 
								  ( ( r>>3 ) << 11 )
								| ( ( g>>2 ) << 5 )
								| ( ( b>>3 ) );

						pixelBuffer16.push_back(p);
					}
				}
			}
			else {
				pixelBuffer16.resize( surface->w*surface->h );
				pixelBuffer16.reserve( surface->w*surface->h );
				DitherTo16( surface, RGB16, true, &pixelBuffer16[0] );
			}
			InsertTextureToDB( name.c_str(), "RGB16", isImage, isFont, surface->w, surface->h, &pixelBuffer16[0], pixelBuffer16.size()*2 );
			break;

		case 8:
			printf( "  Alpha memory=%dk\n", (surface->w * surface->h * 1)/1024 );
			totalTextureMem += (surface->w * surface->h * 1);

			// Bottom up!
			for( int j=surface->h-1; j>=0; --j ) {
				for( int i=0; i<surface->w; ++i ) {
				    U8 *p = (U8 *)surface->pixels + j*surface->pitch + i;
					//SDL_RWwrite( fp, p, 1, 1 );
					pixelBuffer8.push_back(*p);
				}
			}
			InsertTextureToDB( name.c_str(), "ALPHA", isImage, isFont, surface->w, surface->h, &pixelBuffer8[0], pixelBuffer8.size() );
			break;

		default:
			printf( "Unsupported bit depth!\n" );
			exit( 1 );
			break;
	}

	if ( surface ) { 
		SDL_FreeSurface( surface );
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

	
	// Test:
	/*{
		string testInput = inputPath + "Lenna.png";
		SDL_Surface* surface = libIMG_Load( testInput.c_str() );
		if ( surface ) {
			// 444
			U16* data = DitherTo16( surface, RGBA16 );
			SDL_Surface* newSurf = SDL_CreateRGBSurfaceFrom(	data, surface->w, surface->h, 16, surface->w*2,
																0xf000, 0x0f00, 0x00f0, 0 );
			string out = inputPath + "Lenna4440.bmp";
			SDL_SaveBMP( newSurf, out.c_str() );
			
			delete [] data;
			SDL_FreeSurface( newSurf );

			// 565
			data = DitherTo16( surface, RGB16 );
			newSurf = SDL_CreateRGBSurfaceFrom(	data, surface->w, surface->h, 16, surface->w*2,
												0xf800, 0x07e0, 0x001f, 0 );
			string out1 = inputPath + "Lenna565.bmp";
			SDL_SaveBMP( newSurf, out1.c_str() );

			delete [] data;
			SDL_FreeSurface( surface );
			SDL_FreeSurface( newSurf );
		}
	}*/


	TiXmlDocument xmlDoc;
	xmlDoc.LoadFile( input );
	if ( xmlDoc.Error() || !xmlDoc.FirstChildElement() ) {
		printf( "Failed to parse XML file. err=%s\n", xmlDoc.ErrorDesc() );
		exit( 2 );
	}

	xmlDoc.FirstChildElement()->QueryStringAttribute( "output", &outputPath );
	xmlDoc.FirstChildElement()->QueryStringAttribute( "outputDB", &outputDB );

	printf( "Output Path: %s\n", outputPath.c_str() );
	printf( "Output DataBase: %s\n", outputDB.c_str() );
	printf( "Processing tags:\n" );

	// Remove the old table.
#pragma warning ( push )
#pragma warning ( disable : 4996 )	// fopen is unsafe. For video games that seems extreme.
	FILE* fp = fopen( outputDB.c_str(), "wb" );
#pragma warning (pop)
	fclose( fp );

	int sqlResult = sqlite3_open( outputDB.c_str(), &db);
	GLASSERT( sqlResult == SQLITE_OK );
	writer = new BinaryDBWriter( db, true );
	CreateDatabase();

	for( TiXmlElement* child = xmlDoc.FirstChildElement()->FirstChildElement();
		 child;
		 child = child->NextSiblingElement() )
	{
		if (    child->ValueStr() == "texture" 
			 || child->ValueStr() == "image" ) 
		{
			ProcessTexture( child );
		}
		else if ( child->ValueStr() == "model" ) {
			ProcessModel( child );
		}
		else if (	 child->ValueStr() == "map" 
			      || child->ValueStr() == "data" ) {
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

	delete writer;
	sqlResult = sqlite3_close( db );
	GLASSERT( sqlResult == SQLITE_OK );
	return 0;
}