#include "game.h"
#include "cgame.h"
#include "../engine/platformgl.h"
#include "../engine/text.h"
#include "../engine/model.h"


const char* const gModelNames[] = 
{
	"test1",
	"test2",
	"teapot",
	0
};


Game::Game( int width, int height ) :
	engine( width, height, engineData ),
	rotation( 0 ),
	nTexture( 0 ),
	nModelResource( 0 ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 )
{
	surface.Set( 256, 256, 4 );		// All the memory we will ever need (? or that is the intention)

	LoadTextures();
	LoadModels();

	ModelResource* resource = GetResource( "teapot" );
	testModel = engine.GetModel( resource );
	testModel->SetPos( 5.f, 0.0f, 60.f );
}


Game::~Game()
{
	engine.ReleaseModel( testModel );

	FreeModels();
	FreeTextures();
}


void Game::LoadTextures()
{
	memset( texture, 0, sizeof(Texture)*MAX_TEXTURES );

	U32 textureID = 0;
	FILE* fp = 0;
	char buffer[512];

	const char* names[] = 
	{
		"stdfont2",
		"green",
		0
	};

	for( int i=0; names[i]; ++i ) {
		PlatformPathToResource( names[i], "tex", buffer, 512 );
		fp = fopen( buffer, "rb" );
		GLASSERT( fp );
		textureID = surface.LoadTexture( fp );
		texture[ nTexture++ ].Set( names[i], textureID );
		fclose( fp );
	}
	GLASSERT( nTexture <= MAX_TEXTURES );
	UFOInitDrawText( texture[0].glID, engine.Width(), engine.Height(), rotation );
}


void Game::SetScreenRotation( int value ) 
{
	rotation = ((unsigned)value)%4;

	UFOInitDrawText( 0, engine.Width(), engine.Height(), rotation );
	engine.camera.SetViewRotation( rotation );
}


ModelResource* Game::GetResource( const char* name )
{
	for( int i=0; gModelNames[i]; ++i ) {
		if ( strcmp( gModelNames[i], name ) == 0 ) {
			return &modelResource[i];
		}
	}
	GLASSERT( 0 );
	return 0;
}


void Game::LoadModels()
{
	ModelLoader* loader = new ModelLoader( texture, nTexture );
	memset( modelResource, 0, sizeof(ModelResource)*MAX_MODELS );

	FILE* fp = 0;
	char buffer[512];

	for( int i=0; gModelNames[i]; ++i ) {
		PlatformPathToResource( gModelNames[i], "mod", buffer, 512 );
		fp = fopen( buffer, "rb" );
		GLASSERT( fp );
		loader->Load( fp, &modelResource[i] );
		fclose( fp );
	}
	delete loader;
}


void Game::FreeTextures()
{
	for( int i=0; i<nTexture; ++i ) {
		if ( texture[i].glID ) {
			glDeleteTextures( 1, (const GLuint*) &texture[i].glID );
			texture[i].name[0] = 0;
		}
	}
}


void Game::FreeModels()
{
	for( int i=0; i<nModelResource; ++i ) {
		glDeleteBuffers( 1, (const GLuint*) &modelResource[i].dataID );
		glDeleteBuffers( 1, (const GLuint*) &modelResource[i].indexID );		
	}
}


void Game::DoTick( U32 currentTime )
{
	if ( markFrameTime == 0 ) {
		markFrameTime			= currentTime;
		frameCountsSinceMark	= 0;
		framesPerSecond			= 0.0f;
	}
	else {
		++frameCountsSinceMark;
		if ( currentTime - markFrameTime > 500 ) {
			framesPerSecond		= 1000.0f*(float)(frameCountsSinceMark) / ((float)(currentTime - markFrameTime));
			markFrameTime		= currentTime;
			frameCountsSinceMark = 0;
		}
	}

	testModel->SetYRotation( testModel->GetYRotation() + 0.5f );

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glBindTexture( GL_TEXTURE_2D, texture[1].glID );
	engine.Draw();

	glDisable( GL_DEPTH_TEST );
	UFODrawText( 0, 0, "UFO Attack! %.1ffps rot=%d shadow=%d", 
				framesPerSecond, rotation, engine.ShadowMode() );
	glEnable( GL_DEPTH_TEST );
}
