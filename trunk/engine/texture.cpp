#include "texture.h"
#include "platformgl.h"
#include "surface.h"

#include "../grinliz/glstringutil.h"

// Optimizing texture memory.
// F5 -> Continue -> NextChar, Help->Next
// Base: 1620/1290, 2644/2314   (16/0/0)-(18/0/0)
//
// ContextShift:
// 1620/1290, 2644/1290  (16/0/3)-(16/2/9) Nice.

/*static*/ void TextureManager::Create( const gamedb::Reader* reader )
{
	GLASSERT( instance == 0 );
	instance = new TextureManager( reader );
}


/*static*/ void TextureManager::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}


TextureManager::TextureManager( const gamedb::Reader* reader )
{
	cacheHit = 0;
	cacheReuse = 0;
	cacheMiss = 0;
	database = reader;
	parent = database->Root()->Child( "textures" );
	GLASSERT( parent );
}


TextureManager::~TextureManager()
{
	for( unsigned i=0; i<gpuMemArr.Size(); ++i ) {
		if ( gpuMemArr[i].glID ) {
			glDeleteTextures( 1, (const GLuint*) &gpuMemArr[i].glID );
		}
	}
}


/*static*/ TextureManager* TextureManager::instance = 0;

	
Texture* TextureManager::GetTexture( const char* name )
{
	// First check for an existing texture, or one
	// that was added. Failing that, check the database.
	// The texture may not be allocated on the GPU - that
	// happens when the GLID is hit.
	Texture* t = 0;
	map.Query( name, &t );

	if ( !t ) {
		const gamedb::Item* item = parent->Child( name );
		GLASSERT( item );
		
		if ( item ) {
			// Found the item. Generate a new texture.
			int w = item->GetInt( "width" );
			int h = item->GetInt( "height" );
			const char* fstr = item->GetString( "format" );
			int format = Surface::QueryFormat( fstr );
			
			t = textureArr.Push();
			t->Set( name, w, h, format, 0 );
			t->item = item;

			map.Add( t->Name(), t );
		}
	}
	return t;
}


Texture* TextureManager::CreateTexture( const char* name, int w, int h, int format, int flags, ITextureCreator* creator )
{
	GLASSERT( !map.Query( name,  0 ) );

	Texture* t = textureArr.Push();
	t->Set( name, w, h, format, flags );
	t->creator = creator;
	map.Add( t->Name(), t );
	return t;
}


void TextureManager::ContextShift()
{
	for( unsigned i=0; i<textureArr.Size(); ++i ) {
		if ( textureArr[i].gpuMem ) {
			GLASSERT( textureArr[i].gpuMem->inUse );
			// const to the texture, not const to this object.
			// (and this is not a const method)
			GPUMem* gpuMem = const_cast< GPUMem* >( textureArr[i].gpuMem );
			gpuMem->inUse = false;
			textureArr[i].gpuMem = 0;
		}
	}
}


const GPUMem* TextureManager::AllocGPUMemory(	int w, int h, int format, int flags, 
												const gamedb::Item* item, bool* inCache )
{
	*inCache = false;

	// Is the texture currently available? (cache hit)
	if ( item ) {
		GPUMem* gpu = 0;
		if ( gpuMap.Query( item, &gpu ) ) {
			// we're good - have this one cached and ready.
			GLASSERT( gpu->item == item );
			GLASSERT( !gpu->inUse );	// huh? why isn't the texture set?
			GLASSERT( gpu->glID );		// should be attached to memory
			gpu->inUse = true;	
			*inCache = true;
			++cacheHit;
			return gpu;
		}
	}
	// Search for memory to re-use (cache re-use)
	for( unsigned i=0; i<gpuMemArr.Size(); ++i ) {
		GPUMem* gpu = &gpuMemArr[i];

		if (	!gpu->inUse 
			  && gpu->w == w 
			  && gpu->h == h 
			  && gpu->format == format 
			  && gpu->flags == flags ) 
		{
			if ( gpu->item ) {
				gpuMap.Remove( gpu->item );
			}
			gpu->inUse = true;
			gpu->item = item;
			++cacheReuse;
			return gpu;
		}
	}

	// allocate some memory (cache miss)
	GPUMem* gpu = gpuMemArr.Push();
	gpu->w = w;
	gpu->h = h;
	gpu->format = format;
	gpu->flags = flags;
	gpu->item = item;
	gpu->inUse = true;
	gpu->glID = CreateGLTexture( w, h, format, flags );
	++cacheMiss;

	if ( gpu->item ) {
		gpuMap.Add( gpu->item, gpu );
	}
	return gpu;
}


void Texture::Set( const char* p_name, int p_w, int p_h, int p_format, int p_flags )
{
	name = p_name;
	w = p_w;
	h = p_h;
	format = p_format;
	flags = p_flags;
	creator = 0;
	item = 0;
	gpuMem = 0;
}


U32 Texture::GLID() 
{
	if ( gpuMem ) 
		return gpuMem->glID;
	
	TextureManager* manager = TextureManager::Instance();
	bool inCache = false;

	// Allocate memory to store this. The memory should always be available. We
	// may even get memory that already contains the correct pixels.
	gpuMem = manager->AllocGPUMemory( w, h, format, flags, item, &inCache );

	if ( creator || !inCache ) {
		// Need to actually generate the texture. Either pull it from 
		// the database, or call the ICreator to push it to the GPU.
		if ( item ) {
			const gamedb::Reader* database = gamedb::Reader::GetContext( item );
			GLASSERT( item->HasAttribute( "pixels" ) );
			int size;
			const void* pixels = database->AccessData( item, "pixels", &size );
			Upload( pixels, size );
		}
		else if ( creator ) {
			creator->CreateTexture( this );
		}
		else {
			GLASSERT( 0 );
		}
	}
	return gpuMem->glID;
}


void TextureManager::CalcOpenGL( int format, int* glFormat, int* glType )
{
	*glFormat = GL_RGB;
	*glType = GL_UNSIGNED_BYTE;

	switch( format ) {
		case Texture::RGBA16:
			*glFormat = GL_RGBA;
			*glType = GL_UNSIGNED_SHORT_4_4_4_4;
			break;

		case Texture::RGB16:
			*glFormat = GL_RGB;
			*glType = GL_UNSIGNED_SHORT_5_6_5;
			break;

		case Texture::ALPHA:
			*glFormat = GL_ALPHA;
			*glType = GL_UNSIGNED_BYTE;
			break;

		default:
			GLASSERT( 0 );
	}
}


U32 TextureManager::CreateGLTexture( int w, int h, int format, int flags )
{
	int glFormat, glType;
	CalcOpenGL( format, &glFormat, &glType );

	GLASSERT( w && h && (format >= 0) );
	TESTGLERR();

	glEnable( GL_TEXTURE_2D );

	GLuint texID;
	glGenTextures( 1, &texID );
	glBindTexture( GL_TEXTURE_2D, texID );

	if ( flags & Texture::PARAM_NEAREST ) {
		glTexParameteri(	GL_TEXTURE_2D,
							GL_GENERATE_MIPMAP,
							GL_FALSE );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MAG_FILTER,
							GL_NEAREST );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MIN_FILTER,
							GL_NEAREST );
	}
	else {
		glTexParameteri(	GL_TEXTURE_2D,
							GL_GENERATE_MIPMAP,
							GL_TRUE );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MAG_FILTER,
							GL_LINEAR );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MIN_FILTER,
							GL_LINEAR_MIPMAP_NEAREST );
	}
					
//	glTexImage2D(	GL_TEXTURE_2D,
//					0,
//					glFormat,
//					w,
//					h,
//					0,
//					glFormat,
//					glType,
//					pixels );

	TESTGLERR();

	return texID;
}


void Texture::Upload( const void* pixels, int size )
{
	GLASSERT( pixels );
	GLASSERT( size == BytesInImage() );
	GLID();
	GLASSERT( gpuMem );
	GLASSERT( gpuMem->glID );

	int glFormat, glType;
	TextureManager::Instance()->CalcOpenGL( format, &glFormat, &glType );

	glBindTexture( GL_TEXTURE_2D, gpuMem->glID );

	glTexImage2D(	GL_TEXTURE_2D,
					0,
					glFormat,
					w,
					h,
					0,
					glFormat,
					glType,
					pixels );

	TESTGLERR();

}


void Texture::Upload( const Surface& surface )
{
	Upload( surface.Pixels(), surface.BytesInImage() );
}


U32 TextureManager::CalcTextureMem() const
{
	U32 mem = 0;
	for( unsigned i=0; i<textureArr.Size(); ++i ) {
		mem += textureArr[i].BytesInImage();
	}
	return mem;
}


U32 TextureManager::CalcGPUMem() const
{
	U32 mem = 0;
	for( unsigned i=0; i<gpuMemArr.Size(); ++i ) {
		mem += gpuMemArr[i].Memory();
	}
	return mem;
}

