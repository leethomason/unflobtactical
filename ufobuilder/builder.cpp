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
#include "../engine/enginelimits.h"

#include "SDL.h"
#if defined(__APPLE__)	// OSX, not iphone
#include "SDL_image.h"
#endif
#include "SDL_loadso.h"

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glcolor.h"
#include "../engine/enginelimits.h"

#include <string>
#include <vector>

#include "../grinliz/gldynamic.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glstringutil.h"
#include "../tinyxml2/tinyxml2.h"

#include "modelbuilder.h"
#include "../engine/serialize.h"
#include "../importers/import.h"

#include "../shared/gamedbwriter.h"
#include "../grinliz/glstringutil.h"
#include "dither.h"
#include "../version.h"

using namespace std;
using namespace grinliz;
using namespace tinyxml2;

typedef SDL_Surface* (SDLCALL * PFN_IMG_LOAD) (const char *file);
PFN_IMG_LOAD libIMG_Load;

vector<U16> pixelBuffer16;
vector<U8> pixelBuffer8;

string inputDirectory;
string inputFullPath;
string outputPath;

int totalModelMem = 0;
int totalTextureMem = 0;
int totalDataMem = 0;

gamedb::Writer* writer;


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


void ExitError( const char* tag, 
				const char* pathName,
				const char* assetName,
				const char* message )
{
	printf( "\nERROR: tag='%s' path='%s' asset='%s'. %s\n",
		   tag ? tag : "<no tag>", 
		   pathName ? pathName : "<no path>",
		   assetName ? assetName : "<no asset>", 
		   message );
	exit( 1 );
}


void InsertTextureToDB( const char* name, 
						const char* format, 
						bool isImage, 
						bool metrics, 
						int width, 
						int height, 
						const void* pixels, 
						int sizeInBytes,
						bool noMip )
{
	gamedb::WItem* witem = writer->Root()->FetchChild( "textures" )->CreateChild( name );

	witem->SetData( "pixels", pixels, sizeInBytes );
	if ( metrics ) {
		witem->SetData( "metrics", gGlyphMetric, sizeof( GlyphMetric )*GLYPH_CX*GLYPH_CY );
	}

	witem->SetString( "format", format );
	witem->SetBool( "isImage", isImage );
	witem->SetInt( "width", width );
	witem->SetInt( "height", height );
	witem->SetBool( "noMip", noMip );
}


void LoadLibrary()
{
	#if defined(__APPLE__)
		libIMG_Load = &IMG_Load;
	#else
		void* handle = grinliz::grinlizLoadLibrary( "SDL_image" );
		if ( !handle )
		{	
			ExitError( "Initialization", 0, 0, "Could not load SDL_image library." );
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

	this->name.ClearBuf();
	this->name = name;
	this->flags = 0;
	this->nGroups = nGroups;
	this->nTotalVertices = nTotalVertices;
	this->nTotalIndices = nTotalIndices;
	this->bounds = bounds;
	trigger.Set( 0, 0, 0 );
	eye = 0;
	target = 0;
}


void ModelHeader::Save( gamedb::WItem* parent )
{
	gamedb::WItem* node = parent->CreateChild( "header" );
	node->SetInt( "nTotalVertices", nTotalVertices );
	node->SetInt( "nTotalIndices", nTotalIndices );
	node->SetInt( "flags", flags );
	node->SetInt( "nGroups", nGroups );

	gamedb::WItem* boundNode = node->CreateChild( "bounds" );
	boundNode->SetFloat( "min.x", bounds.min.x );
	boundNode->SetFloat( "min.y", bounds.min.y );
	boundNode->SetFloat( "min.z", bounds.min.z );
	boundNode->SetFloat( "max.x", bounds.max.x );
	boundNode->SetFloat( "max.y", bounds.max.y );
	boundNode->SetFloat( "max.z", bounds.max.z );

	if ( trigger.x != 0.0f || trigger.y != 0.0f || trigger.z != 0.0f ) {
		gamedb::WItem* triggerNode = node->CreateChild( "trigger" );
		triggerNode->SetFloat( "x", trigger.x );
		triggerNode->SetFloat( "y", trigger.y );
		triggerNode->SetFloat( "z", trigger.z );
	}
	if ( eye != 0.0f ||	target != 0.0f ) {
		gamedb::WItem* extra = node->CreateChild( "extended" );
		extra->SetFloat( "eye", eye );
		extra->SetFloat( "target", target );
	}
}


void ModelGroup::Set( const char* textureName, int nVertex, int nIndex )
{
	GLASSERT( nVertex > 0 && nVertex < EL_MAX_VERTEX_IN_MODEL );
	GLASSERT( nIndex > 0 && nIndex < EL_MAX_INDEX_IN_MODEL );
	GLASSERT( nVertex <= nIndex );

	this->textureName.ClearBuf();
	this->textureName = textureName;
	this->nVertex = nVertex;
	this->nIndex = nIndex;
}


Color4U8 GetPixel( const SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    U8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	U32 c = 0;

    switch(bpp) {
    case 1:
        c = *p;
		break;

    case 2:
        c = *(Uint16 *)p;
		break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            c =  p[0] << 16 | p[1] << 8 | p[2];
        else
            c = p[0] | p[1] << 8 | p[2] << 16;
		break;

    case 4:
        c = *(U32 *)p;
		break;
    }
	Color4U8 color;
	SDL_GetRGBA( c, surface->format, &color.r, &color.g, &color.b, &color.a );
	return color;
}


void PutPixel(SDL_Surface *surface, int x, int y, const Color4U8& c )
{
	U32 pixel = SDL_MapRGBA( surface->format, c.r, c.g, c.b, c.a );
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


template <class STRING>
void AssignIf( STRING& a, const XMLElement* element, const char* attribute ) {
	const char* attrib = element->Attribute( attribute );
	if ( attrib ) {
		a = element->Attribute( attribute );
	}
}


void ParseNames( const XMLElement* element, GLString* _assetName, GLString* _fullPath, GLString* _extension )
{
	string filename;
	AssignIf( filename, element, "filename" );

	string assetName;
	AssignIf( assetName, element, "modelName" );
	AssignIf( assetName, element, "assetName" );

	GLString fullIn = inputDirectory.c_str();
	fullIn += filename.c_str();	

	GLString base, name, extension;
	grinliz::StrSplitFilename( fullIn, &base, &name, &extension );
	if ( assetName.empty() ) {
		assetName = name.c_str();
	}

	if ( _assetName )
		*_assetName = assetName.c_str();
	if ( _fullPath )
		*_fullPath = fullIn;
	if ( _extension )
		*_extension = extension;
}


void ProcessTreeRec( gamedb::WItem* parent, XMLElement* ele )
{
	string name = ele->Value();
	gamedb::WItem* witem = parent->CreateChild( name.c_str() );

	for( const XMLAttribute* attrib=ele->FirstAttribute(); attrib; attrib=attrib->Next() ) {		
		int i;

		if ( XML_NO_ERROR == attrib->QueryIntValue( &i ) ) {
			witem->SetInt( attrib->Name(), i );
		}
		else {
			witem->SetString( attrib->Name(), attrib->Value() );
		}
	}

	if ( ele->GetText() ) {
		witem->SetData( "text", ele->GetText(), strlen( ele->GetText() ) );
	}

	for( XMLElement* child=ele->FirstChildElement(); child; child=child->NextSiblingElement() ) {
		ProcessTreeRec( witem, child );
	}
}


void ProcessTree( XMLElement* data )
{
	// Create the root tree "research"
	gamedb::WItem* witem = writer->Root()->FetchChild( "tree" );
	if ( data->FirstChildElement() )
		ProcessTreeRec( witem, data->FirstChildElement() );
}




void ProcessData( XMLElement* data )
{
	bool compression = true;
	const char* compress = data->Attribute( "compress" );
	if ( compress && StrEqual( compress, "false" ) ) {
		compression = false;
	}

	string filename;
	AssignIf( filename, data, "filename" );
	string fullIn = inputDirectory + filename;

	GLString assetName, pathName;
	ParseNames( data, &assetName, &pathName, 0 );

	// copy the entire file.
#pragma warning ( push )
#pragma warning ( disable : 4996 )	// fopen is unsafe. For video games that seems extreme.
	FILE* read = fopen( pathName.c_str(), "rb" );
#pragma warning (pop)

	if ( !read ) {
		ExitError( "Data", pathName.c_str(), assetName.c_str(), "Could not load asset file." );
	}

	// length of file.
	fseek( read, 0, SEEK_END );
	int len = ftell( read );
	fseek( read, 0, SEEK_SET );

	char* mem = new char[len];
	fread( mem, len, 1, read );

	int index = 0;
	gamedb::WItem* witem = writer->Root()->FetchChild( "data" )->CreateChild( assetName.c_str() );
	witem->SetData( "binary", mem, len, compression );

	delete [] mem;

	printf( "Data '%s' memory=%dk compression=%s\n", filename.c_str(), len/1024, compression ? "true" : "false" );
	totalDataMem += len;

	fclose( read );
}


class TextBuilder : public XMLVisitor
{
public:
	string str;
	string filter;

	bool textOn;
	int depth;

	TextBuilder() : textOn( false ), depth( 0 ) {}

	virtual bool VisitEnter( const XMLElement& element, const XMLAttribute* firstAttribute )
	{
		++depth;

		// depth = 1 element
		// depth = 2 <p>
		if ( depth == 2 ) {
			string platform;
			AssignIf( platform, &element, "system" );

			if ( platform.empty() || platform == filter )
				textOn = true;
		}
		return true;
	}
	virtual bool VisitExit( const XMLElement& element )
	{
		if ( depth == 2 ) {
			if ( textOn ) {
				str += "\n\n";
				textOn = false;
			}
		}
		--depth;
		return true;
	}
	virtual bool Visit( const XMLText& text) {
		if ( textOn )
			str += text.Value();
		return true;
	}
};


void ProcessText( XMLElement* textEle )
{
	string name;
	AssignIf( name, textEle, "name" );
	gamedb::WItem* textItem = writer->Root()->FetchChild( "text" )->CreateChild( name.c_str() );
	int index = 0;

	for( XMLElement* pageEle = textEle->FirstChildElement( "page" ); pageEle; pageEle = pageEle->NextSiblingElement( "page" ) ) {
		string pcText, androidText;

		TextBuilder textBuilder;
		textBuilder.filter = "pc";
		pageEle->Accept( &textBuilder );
		pcText = textBuilder.str;

		textBuilder.str.clear();
		textBuilder.filter = "android";
		pageEle->Accept( &textBuilder );
		androidText = textBuilder.str;

		gamedb::WItem* pageItem = textItem->CreateChild( index );

		if ( pcText == androidText ) {
			pageItem->SetData( "text", pcText.c_str(), pcText.size() );
			totalDataMem += pcText.size();
		}
		else {
			pageItem->SetData( "text_pc", pcText.c_str(), pcText.size() );
			pageItem->SetData( "text_android", androidText.c_str(), androidText.size() );
			totalDataMem += pcText.size();
			totalDataMem += androidText.size();
		}

		if ( pageEle->Attribute( "image" ) ) {
			pageItem->SetString( "image", pageEle->Attribute( "image" ) );
		}
		printf( "Text '%s' page %d\n", name.c_str(), index );
		index++;
	}
}

void StringToVector( const char* str, Vector3F* vec )
{
#pragma warning ( push )
#pragma warning ( disable : 4996 )
		sscanf( str, "%f %f %f", &vec->x, &vec->y, &vec->z );
#pragma warning ( pop )
}


void ProcessModel( XMLElement* model )
{
	int nTotalIndex = 0;
	int nTotalVertex = 0;
	int startIndex = 0;
	int startVertex = 0;
	Vector3F origin = { 0, 0, 0 };

	//string filename;
	//model->QueryStringAttribute( "filename", &filename );
	//tring modelName;
	//model->QueryStringAttribute( "modelName", &modelName );

	GLString pathName, assetName, extension;
	ParseNames( model, &assetName, &pathName, &extension );

	if ( model->Attribute( "origin" ) ) {
		StringToVector( model->Attribute( "origin" ), &origin );
	}
	bool usingSubModels = false;
	if ( model->Attribute( "modelName" ) ) {
		usingSubModels = true;
	}
	
	//GLString fullIn = inputPath.c_str(); 
	//fullIn += filename.c_str();
	//
	//GLString base, name, extension;
	//grinliz::StrSplitFilename( fullIn, &base, &name, &extension );

	// If the model name is specified, it is one model in a lineup.
	// Instead of the filename, use the specified model name.
	//if ( !modelName.empty() )
	//	name = modelName.c_str();

	printf( "Model '%s'", assetName.c_str() );

	ModelBuilder* builder = new ModelBuilder();
	builder->SetShading( ModelBuilder::FLAT );

	if ( grinliz::StrEqual( model->Attribute( "shading" ), "smooth" ) ) {
		builder->SetShading( ModelBuilder::SMOOTH );
	}
	else if ( grinliz::StrEqual( model->Attribute( "shading" ), "crease" ) ) {
		builder->SetShading( ModelBuilder::CREASE );
	}
	else if ( grinliz::StrEqual(  model->Attribute( "shading" ), "smooth" ) ) {
		builder->SetShading( ModelBuilder::SMOOTH );
	}

	if ( grinliz::StrEqual( model->Attribute( "polyRemoval" ), "pre" ) ) {
		builder->SetPolygonRemoval( ModelBuilder::POLY_PRE );
	}

	if ( extension == ".ac" ) {
		ImportAC3D(	std::string( pathName.c_str() ), builder, origin, usingSubModels ? string( assetName.c_str()) : "" );
	}
	else if ( extension == ".off" ) {
		ImportOFF( std::string( pathName.c_str() ), builder );
	}
	else {
		ExitError( "Model", pathName.c_str(), assetName.c_str(), "Could not load asset file." ); 
	}

	const VertexGroup* vertexGroup = builder->Groups();
	
	for( int i=0; i<builder->NumGroups(); ++i ) {
		nTotalIndex += vertexGroup[i].nIndex;
		nTotalVertex += vertexGroup[i].nVertex;
	}
	if ( builder->PolyCulled() ) {
		printf( " culled=%d", builder->PolyCulled() );
	}	
	printf( " groups=%d nVertex=%d nTri=%d\n", builder->NumGroups(), nTotalVertex, nTotalIndex/3 );
	ModelHeader header;
	header.Set( assetName.c_str(), builder->NumGroups(), nTotalVertex, nTotalIndex, builder->Bounds() );

	if ( grinliz::StrEqual( model->Attribute( "shadowCaster" ), "false" ) ) {
		header.flags |= ModelHeader::RESOURCE_NO_SHADOW;
	}
	if ( model->Attribute( "trigger" ) ) {
		StringToVector( model->Attribute( "trigger" ), &header.trigger );
		header.trigger -= origin;
	}
	if ( model->Attribute( "eye" ) ) {
		model->QueryFloatAttribute( "eye", &header.eye );
		header.eye -= origin.y;
	}
	if ( model->Attribute( "target" ) ) {
		model->QueryFloatAttribute( "target", &header.target );
		header.target -= origin.y;
	}

	gamedb::WItem* witem = writer->Root()->FetchChild( "models" )->CreateChild( assetName.c_str() );

	int totalMemory = 0;

	for( int i=0; i<builder->NumGroups(); ++i ) {
		int mem = vertexGroup[i].nVertex*sizeof(Vertex) + vertexGroup[i].nIndex*2;
		totalMemory += mem;
		printf( "    %d: '%s' nVertex=%d nTri=%d memory=%.1fk\n",
				i,
				vertexGroup[i].textureName.c_str(),
				vertexGroup[i].nVertex,
				vertexGroup[i].nIndex / 3,
				(float)mem/1024.0f );

		ModelGroup group;
		group.Set( vertexGroup[i].textureName.c_str(), vertexGroup[i].nVertex, vertexGroup[i].nIndex );

		gamedb::WItem* witemGroup = witem->CreateChild( i );
		witemGroup->SetString( "textureName", group.textureName.c_str() );
		witemGroup->SetInt( "nVertex", group.nVertex );
		witemGroup->SetInt( "nIndex", group.nIndex );

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
		GLASSERT( pVertex + vertexGroup[i].nVertex <= pVertexEnd );
		memcpy( pVertex, vertexGroup[i].vertex, sizeof(Vertex)*vertexGroup[i].nVertex );
		pVertex += vertexGroup[i].nVertex;
	}
	// Write the indices in each group:
	for( int i=0; i<builder->NumGroups(); ++i ) {
		GLASSERT( pIndex + vertexGroup[i].nIndex <= pIndexEnd );
		memcpy( pIndex, vertexGroup[i].index, sizeof(U16)*vertexGroup[i].nIndex );
		pIndex += vertexGroup[i].nIndex;
	}
	GLASSERT( pVertex == pVertexEnd );
	GLASSERT( pIndex == pIndexEnd );

	int vertexID = 0, indexID = 0;

	//witem->SetData( "header", &header, sizeof(header ) );
	header.Save( witem );
	witem->SetData( "vertex", vertexBuf, nTotalVertex*sizeof(Vertex) );
	witem->SetData( "index", indexBuf, nTotalIndex*sizeof(U16) );

	printf( "  total memory=%.1fk\n", (float)totalMemory / 1024.f );
	totalModelMem += totalMemory;
	
	delete [] vertexBuf;
	delete [] indexBuf;
	delete builder;
}


bool BarIsBlack( SDL_Surface* surface, int x, int y0, int y1 )
{
	bool black = true;

	for( int y=y0; y<=y1; ++y ) {
		Color4U8 c = GetPixel( surface, x, y );
		if ( c.a == 0 || ( c.r==0 && c.g==0 && c.b==0 ) ) {
			// do nothing.
		}
		else {
			black = false;
			break;
		}
	}
	return black;
}


#if 0
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
			//GLOUTPUT(( "glyph %d offset=%d width=%d\n", gy*GLYPH_CX+gx, g->offset, g->width ));
		}
	}
}
#endif


SDL_Surface* CreateScaledSurface( int w, int h, SDL_Surface* surface )
{
	int sx = surface->w / w;
	int sy = surface->h / h;

	GLASSERT( sx > 0 && sy > 0 );
	SDL_Surface* newSurf = SDL_CreateRGBSurface( 0, w, h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask );

	for( int y=0; y<h; ++y ) {
		for( int x=0; x<w; ++x ) {
			int r32=0, g32=0, b32=0, a32=0;
			
			int xSrc = x*sx;
			int ySrc = y*sy;
			for( int j=0; j<sy; ++j ) {
				for( int i=0; i<sx; ++i ) {
					Color4U8 c = GetPixel( surface, xSrc+i, ySrc+j );
					r32 += c.r;
					g32 += c.g;
					b32 += c.b;
					a32 += c.a;
				}
			}
			Color4U8 c = { r32/(sx*sy), g32/(sx*sy), b32/(sx*sy), a32/(sx*sy) };
			PutPixel( newSurf, x, y, c );
		}
	}
	return newSurf;
}


void BlitTexture( const XMLElement* element, SDL_Surface* target )
{
	int sx=0, sy=0, sw=0, sh=0, tx=0, ty=0, tw=target->w, th=target->h;
	element->QueryIntAttribute( "sx", &sx );
	element->QueryIntAttribute( "sy", &sy );
	element->QueryIntAttribute( "sw", &sw );
	element->QueryIntAttribute( "sh", &sh );
	element->QueryIntAttribute( "tx", &tx );
	element->QueryIntAttribute( "ty", &ty );
	element->QueryIntAttribute( "tw", &tw );
	element->QueryIntAttribute( "th", &th );

	GLString pathName;
	ParseNames( element, 0, &pathName, 0 );

	SDL_Surface* surface = libIMG_Load( pathName.c_str() );
	if ( !surface ) {
		ExitError( "Blit Tag", pathName.c_str(), "Blit", "Could not load asset file." );
	}

	for( int j=0; j<th; ++j ) {
		for( int i=0; i<tw; ++i ) {
			int targetX = tx+i;
			int targetY = ty+j;

			float srcX = (float)sx + (float)i*(float)sw/(float)tw;
			float srcY = (float)sy + (float)j*(float)sh/(float)th;

			if (    srcX >= 0 && srcX <= (float)(surface->w - 2)
				 && srcY >= 0 && srcY <= (float)(surface->h - 2)) 
			{
				int x = (int)srcX;
				int y = (int)srcY;
				Color4U8 c00 = GetPixel( surface, x, y );
				Color4U8 c10 = GetPixel( surface, x+1, y );
				Color4U8 c01 = GetPixel( surface, x, y+1 );
				Color4U8 c11 = GetPixel( surface, x+1, y+1 );
				Color4U8 c;
				for( int i=0; i<4; ++i ) {
					c[i] = (U8)BilinearInterpolate( (float)c00[i], 
													(float)c10[i], 
													(float)c01[i], 
													(float)c11[i], 
													srcX-(float)x, srcY-(float)y );
				}
				PutPixel( target, targetX, targetY, c );
			}
			else {
				int x = Clamp( (int)srcX, 0, surface->w-1 );
				int y = Clamp( (int)srcY, 0, surface->h-1 );

				Color4U8 c = GetPixel( surface, x, y );
				PutPixel( target, targetX, targetY, c );
			}
		}
	}
	SDL_FreeSurface( surface );
}


void ProcessTexture( XMLElement* texture )
{
	bool isImage = false;
	if ( StrEqual( texture->Value(), "image" )) {
		isImage = true;
		printf( "Image" );
	}
	else {
		printf( "Texture" );
	}

	bool dither = true;
	bool nomip = false;
	if ( StrEqual( texture->Attribute( "dither" ), "false" ) ) {
		dither = false;
	}
	if ( StrEqual( texture->Attribute( "noMip" ), "true" ) ) {
		nomip = true;
	}
	int width = 0;
	int height = 0;
	texture->QueryIntAttribute( "width", &width );
	texture->QueryIntAttribute( "height", &height );
	
	GLString pathName, assetName;
	ParseNames( texture, &assetName, &pathName, 0 );

	SDL_Surface* surface = libIMG_Load( pathName.c_str() );
	if ( !surface ) {
		ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Could not load asset file." );
	}
	else {
		if (    isImage
			 || ( surface->w == 384 && surface->h == 384 )			// weird exception (battleship and base)
			 || ( IsPowerOf2( surface->w ) && IsPowerOf2( surface-> h ) ) )
		{
			// no problem.
		}
		else {
			printf( "ERROR: width=%d height=%d\n", surface->w, surface->h );
			ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Textures must be power of 2 dimension." );
		}
		printf( "  Loaded: '%s' bpp=%d", 
				assetName.c_str(),
				surface->format->BitsPerPixel );
	}

	if ( width && height ) {
		SDL_Surface* old = surface;
		surface = CreateScaledSurface( width, height, surface );
		SDL_FreeSurface( old );

		printf( " Scaled" );
	}
	printf( " w=%d h=%d", surface->w, surface->h );

	// run through child tags.
	// interpolate in
	bool sub=false;
	for( XMLElement* blit=texture->FirstChildElement( "blit" );
		 blit;
		 blit=blit->NextSiblingElement( "blit" ) ) 
	{
		BlitTexture( blit, surface );
		sub = true;
	}
	if ( sub ) {
		SDL_SaveBMP( surface, "comp.bmp" );
	}

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
						Color4U8 c = GetPixel( surface, i, j );

						U16 p =
								  ( ( c.r>>4 ) << 12 )
								| ( ( c.g>>4 ) << 8 )
								| ( ( c.b>>4 ) << 4)
								| ( ( c.a>>4 ) << 0 );

						pixelBuffer16.push_back(p);
					}
				}
			}
			else {
				pixelBuffer16.resize( surface->w*surface->h );
				pixelBuffer16.reserve( surface->w*surface->h );
				OrderedDitherTo16( surface, RGBA16, true, &pixelBuffer16[0] );
			}
			InsertTextureToDB( assetName.c_str(), "RGBA16", isImage, false, surface->w, surface->h, &pixelBuffer16[0], pixelBuffer16.size()*2, nomip );
			break;

		case 24:
			printf( "  RGB memory=%dk\n", (surface->w * surface->h * 2)/1024 );
			totalTextureMem += (surface->w * surface->h * 2);

			// Bottom up!
			if ( !dither ) {
				for( int j=surface->h-1; j>=0; --j ) {
					for( int i=0; i<surface->w; ++i ) {
						Color4U8 c = GetPixel( surface, i, j );

						U16 p = 
								  ( ( c.r>>3 ) << 11 )
								| ( ( c.g>>2 ) << 5 )
								| ( ( c.b>>3 ) );

						pixelBuffer16.push_back(p);
					}
				}
			}
			else {
				pixelBuffer16.resize( surface->w*surface->h );
				pixelBuffer16.reserve( surface->w*surface->h );
				OrderedDitherTo16( surface, RGB16, true, &pixelBuffer16[0] );
			}
			InsertTextureToDB( assetName.c_str(), "RGB16", isImage, false, surface->w, surface->h, &pixelBuffer16[0], pixelBuffer16.size()*2, nomip );
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
			InsertTextureToDB( assetName.c_str(), "ALPHA", isImage, false, surface->w, surface->h, &pixelBuffer8[0], pixelBuffer8.size(), nomip );
			break;

		default:
			ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Unsupported bit depth.\n" );
			break;
	}

	if ( surface ) { 
		SDL_FreeSurface( surface );
	}
}


void ProcessPalette( XMLElement* pal )
{
	int dx=0;
	int dy=0;
	pal->QueryIntAttribute( "dx", &dx );
	pal->QueryIntAttribute( "dy", &dy );
	float sx=0, sy=0;
	pal->QueryFloatAttribute( "sx", &sx );
	pal->QueryFloatAttribute( "sy", &sy );
	
	GLString pathName, assetName;
	ParseNames( pal, &assetName, &pathName, 0 );

	SDL_Surface* surface = libIMG_Load( pathName.c_str() );
	if ( !surface ) {
		ExitError( "Palette", pathName.c_str(), assetName.c_str(), "Could not load asset." );
	}
	printf( "Palette[data] asset=%s cx=%d cy=%d sx=%f sy=%f\n",
		    assetName.c_str(), dx, dy, sx, sy );

	std::vector<Color4U8> colors;
	int offsetX = (int)(sx * (float)(surface->w / dx));
	int offsetY = (int)(sy * (float)(surface->h / dy));

	for( int y=0; y<dy; ++y ) {
		for( int x=0; x<dx; ++x ) {
			int px = surface->w * x / dx + offsetX;
			int py = surface->h * y / dy + offsetY;
			Color4U8 rgba = GetPixel( surface, px, py );
			colors.push_back( rgba );
		}
	}

	gamedb::WItem* witem = writer->Root()->FetchChild( "data" )
										 ->FetchChild( "palettes" )
										 ->CreateChild( assetName.c_str() );
	witem->SetInt( "dx", dx );
	witem->SetInt( "dy", dy );
	witem->SetData( "colors", &colors[0], dx*dy*sizeof(Color4U8), true );

	if ( surface ) { 
		SDL_FreeSurface( surface );
	}
}


void ProcessFont( XMLElement* font )
{
	GLString pathName, assetName;
	ParseNames( font, &assetName, &pathName, 0 );

	SDL_Surface* surface = libIMG_Load( pathName.c_str() );
	if ( !surface ) {
		ExitError( "Font", pathName.c_str(), assetName.c_str(), "Could not load asset." );
	}
	ProcessTexture( font );

	printf( "font asset='%s'\n",
		    assetName.c_str() );
	gamedb::WItem* witem = writer->Root()->FetchChild( "data" )
										 ->FetchChild( "fonts" )
										 ->CreateChild( assetName.c_str() );
	
	// Read the data tags. These describe the font.
	static const char* tags[] = { "info", "common", 0 };
	for( int i=0; tags[i]; ++i ) {
		const char* tag = tags[i];
		
		const XMLElement* element = font->FirstChildElement( tag );
		if ( !element ) {
			char buf[90];
			SNPrintf( buf, 90, "XML Element %s not found.", tag );
			ExitError( "Font", pathName.c_str(), assetName.c_str(), buf );
		}
		
		gamedb::WItem* tagItem = witem->CreateChild( tag );
		for( const XMLAttribute* attrib=element->FirstAttribute();
			 attrib;
			 attrib = attrib->Next() )
		{
			tagItem->SetInt( attrib->Name(), attrib->IntValue() );
		}
	}

	// Character data.
	const XMLElement* charsElement = font->FirstChildElement( "chars" );
	if ( !charsElement ) {
		ExitError( "Font", pathName.c_str(), assetName.c_str(), "Font contains no 'chars' XML Element." );
	}

	gamedb::WItem* charsItem = witem->CreateChild( "chars" );
	for( const XMLElement* charElement = charsElement->FirstChildElement( "char" );
		 charElement;
		 charElement = charElement->NextSiblingElement( "char" ) )
	{
		// Gamedb items must be unique. Munge name and id.
		char buffer[6] = "char0";
		int id = 1;
		charElement->QueryIntAttribute( "id", &id );
		buffer[4] = id;
		gamedb::WItem* charItem = charsItem->CreateChild( buffer );

		for( const XMLAttribute* attrib=charElement->FirstAttribute();
			 attrib;
			 attrib = attrib->Next() )
		{
			if ( !StrEqual( attrib->Name(), "id" ) ) {
				charItem->SetInt( attrib->Name(), attrib->IntValue() );
			}
		}
	}

	const XMLElement* kerningsElement = font->FirstChildElement( "kernings" );
	if ( !kerningsElement ) {
		return;
	}
	gamedb::WItem* kerningsItem = witem->CreateChild( "kernings" );
	for( const XMLElement* kerningElement = kerningsElement->FirstChildElement( "kerning" );
		 kerningElement;
		 kerningElement = kerningElement->NextSiblingElement( "kerning" ) )
	{
		// Gamedb items must be unique. Munge name and id.
		char buffer[10] = "kerning00";
		int first = 1, second = 1;
		kerningElement->QueryIntAttribute( "first", &first );
		kerningElement->QueryIntAttribute( "second", &second );
		buffer[7] = first;
		buffer[8] = second;
		gamedb::WItem* kerningItem = kerningsItem->CreateChild( buffer );
		int amount = 0;
		kerningElement->QueryIntAttribute( "amount", &amount );
		kerningItem->SetInt( "amount",	amount );
	}
}



int main( int argc, char* argv[] )
{
	printf( "UFO Builder. version=%d argc=%d argv[1]=%s\n", VERSION, argc, argv[1] );
	if ( argc < 3 ) {
		printf( "Usage: ufobuilder ./path/xmlFile.xml ./outPath/filename.db <options>\n" );
		printf( "options:\n" );
		printf( "    -d    print database\n" );
		exit( 0 );
	}

    SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER );
	LoadLibrary();

	inputFullPath = argv[1];
	outputPath = argv[2];
	bool printDatabase = false;
	for( int i=3; i<argc; ++i ) {
		if ( StrEqual( argv[i], "-d" ) ) {
			printDatabase = true;
		}
	}

	GLString _inputFullPath( inputFullPath.c_str() ), _inputDirectory, name, extension;
	grinliz::StrSplitFilename( _inputFullPath, &_inputDirectory, &name, &extension );
	inputDirectory = _inputDirectory.c_str();

	const char* xmlfile = argv[2];
	printf( "Opening, path: '%s' filename: '%s'\n", inputDirectory.c_str(), inputFullPath.c_str() );
	
	// Test:
	{
		string testInput = inputDirectory + "Lenna.png";
		SDL_Surface* surface = libIMG_Load( testInput.c_str() );
		if ( surface ) {
			pixelBuffer16.resize( surface->w*surface->h );
			pixelBuffer16.reserve( surface->w*surface->h );

			// 444
			OrderedDitherTo16( surface, RGBA16, false, &pixelBuffer16[0] );
			SDL_Surface* newSurf = SDL_CreateRGBSurfaceFrom(	&pixelBuffer16[0], surface->w, surface->h, 16, surface->w*2,
																0xf000, 0x0f00, 0x00f0, 0 );
			string out = inputDirectory + "Lenna4440.bmp";
			SDL_SaveBMP( newSurf, out.c_str() );
			
			SDL_FreeSurface( newSurf );

			// 565
			OrderedDitherTo16( surface, RGB16, false, &pixelBuffer16[0] );
			newSurf = SDL_CreateRGBSurfaceFrom(	&pixelBuffer16[0], surface->w, surface->h, 16, surface->w*2,
												0xf800, 0x07e0, 0x001f, 0 );
			string out1 = inputDirectory + "Lenna565.bmp";
			SDL_SaveBMP( newSurf, out1.c_str() );

			SDL_FreeSurface( newSurf );

			// 565 Diffusion
			DiffusionDitherTo16( surface, RGB16, false, &pixelBuffer16[0] );
			newSurf = SDL_CreateRGBSurfaceFrom(	&pixelBuffer16[0], surface->w, surface->h, 16, surface->w*2,
												0xf800, 0x07e0, 0x001f, 0 );
			string out2 = inputDirectory + "Lenna565Diffuse.bmp";
			SDL_SaveBMP( newSurf, out2.c_str() );

			SDL_FreeSurface( newSurf );
			
			SDL_FreeSurface( surface );
		}
	}

	XMLDocument xmlDoc( true, COLLAPSE_WHITESPACE );
	xmlDoc.LoadFile( inputFullPath.c_str() );
	if ( xmlDoc.Error() || !xmlDoc.FirstChildElement() ) {
		xmlDoc.PrintError();
		exit( 2 );
	}

	printf( "Processing tags:\n" );

	// Remove the old table.
	writer = new gamedb::Writer();

	for( XMLElement* child = xmlDoc.FirstChildElement()->FirstChildElement();
		 child;
		 child = child->NextSiblingElement() )
	{
		if (    StrEqual( child->Value(), "texture" )
			 || StrEqual( child->Value(), "image" ))
		{
			ProcessTexture( child );
		}
		else if ( StrEqual( child->Value(),  "model" )) {
			ProcessModel( child );
		}
		else if ( StrEqual( child->Value(), "data" )) {
			ProcessData( child );
		}
		else if ( StrEqual( child->Value(), "text" )) {
			ProcessText( child );
		}
		else if ( StrEqual( child->Value(), "tree" )) {
			ProcessTree( child );
		}
		else if ( StrEqual( child->Value(), "palette" )) {
			ProcessPalette( child );
		}
		else if ( StrEqual( child->Value(), "font" )) {
			ProcessFont( child );
		}
		else {
			printf( "Unrecognized element: %s\n", child->Value() );
		}
	}

	int total = totalTextureMem + totalModelMem + totalDataMem;

	printf( "Total memory=%dk Texture=%dk Model=%dk Data=%dk\n", 
			total/1024, totalTextureMem/1024, totalModelMem/1024, totalDataMem/1024 );
	printf( "All done.\n" );
	SDL_Quit();

	writer->Save( outputPath.c_str() );
	delete writer;

	if ( printDatabase ) {
		gamedb::Reader reader;
		reader.Init( 0, outputPath.c_str(), 0 );
		reader.RecWalk( reader.Root(), 0 );
	}

	return 0;
}