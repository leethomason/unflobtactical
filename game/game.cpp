#include "game.h"
#include "cgame.h"
#include "../engine/platformgl.h"
#include "../engine/text.h"
#include "../engine/model.h"


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


	testModel = engine.GetModel( &modelResource[0] );
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
	
	/*PlatformPathToResource( "stdfont2", "tex", buffer, 512 );
	fp = fopen( buffer, "rb" );
	GLASSERT( fp );
	textureID = surface.LoadTexture( fp );
	//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	UFOInitDrawText( textureID );
	texture[ nTexture++ ].Set( "stdfont", textureID );
	fclose( fp );
	*/

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


void Game::SetRotation( int value ) 
{
	rotation = ((unsigned)value)%4;

	UFOInitDrawText( 0, engine.Width(), engine.Height(), rotation );
	engine.camera.SetViewRotation( rotation );
}


void Game::LoadModels()
{
	/*
	 ModelLoader* loader = new ModelLoader( texture, nTexture );
	 memset( modelResource, 0, sizeof(ModelResource)*MAX_MODELS );

	 FILE* fp = 0;

	 fp = fopen( "./res/test.mod", "rb" );
	 GLASSERT( fp );
	 loader->Load( fp, &modelResource[nModelResource++] );

	 delete loader;
	 */
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
	/*
	for( int i=0; i<nModelResource; ++i ) {
		glDeleteBuffers( 1, (const GLuint*) &modelResource[i].dataID );
		glDeleteBuffers( 1, (const GLuint*) &modelResource[i].indexID );		
	}
	 */
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

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable( GL_DEPTH_TEST );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glBindTexture( GL_TEXTURE_2D, texture[1].glID );
	engine.Draw();


	glDisable( GL_DEPTH_TEST );
	UFODrawText( 0, 0, "UFO Attack! %.1ffps rot=%d", framesPerSecond, rotation );
	glEnable( GL_DEPTH_TEST );
}
