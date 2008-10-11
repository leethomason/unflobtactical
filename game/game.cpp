#include "game.h"
#include "../engine/platformgl.h"
#include "../engine/text.h"


Game::Game() :
	engine( WIDTH, HEIGHT, engineData ),
	nTexture( 0 ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 )
{
	surface.Set( 256, 256, 4 );		// All the memory we will ever need (? or that is the intention)

	LoadTextures();
}


Game::~Game()
{
	FreeTextures();
}


void Texture::Set( const char* name, U32 glID )
{
	GLASSERT( strlen( name ) < 16 );
	strcpy( this->name, name );
	this->glID = glID;
}


void Game::LoadTextures()
{
	memset( texture, 0, sizeof(Texture)*MAX_TEXTURES );

	U32 textureID = 0;
	FILE* fp = 0;

	fp = fopen( "./res/stdfont.tex", "rb" );
	GLASSERT( fp );
	textureID = surface.LoadTexture( fp );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	UFOInitDrawText( textureID );
	texture[ nTexture++ ].Set( "stdfont", textureID );
	fclose( fp );

	fp = fopen( "./res/green.tex", "rb" );
	GLASSERT( fp );
	textureID = surface.LoadTexture( fp );
	texture[ nTexture++ ].Set( "testTile", textureID );
	fclose( fp );

	GLASSERT( nTexture <= MAX_TEXTURES );
}


void Game::FreeTextures()
{
	for( int i=0; i<nTexture; ++i ) {
		if ( texture[i].glID ) {
			glDeleteTextures( 1, &texture[i].glID );
			texture[i].name[0] = 0;
		}
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

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable( GL_DEPTH_TEST );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glBindTexture( GL_TEXTURE_2D, texture[1].glID );
	engine.Draw();


	glDisable( GL_DEPTH_TEST );
	UFODrawText( 0, 0, "UFO Attack! %.1ffps", framesPerSecond );
	glEnable( GL_DEPTH_TEST );
}
