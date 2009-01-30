#include "game.h"
#include "cgame.h"
#include "../engine/platformgl.h"
#include "../engine/text.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"


const char* const gModelNames[] = 
{
	"test1",
	"test2",
	"teapot",
	"crate",
	0
};

const char* const gTextureNames[] = 
{
	"stdfont2",
	"icons",
	"green",
	"woodDark",
	"woodDarkUFO",
	0
};

enum {
	ICON_NONE,
	ICON_CAMERA_HOME
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
	iconInfo[0].Set( ICON_CAMERA_HOME, 0, 270, 50, 50, 0.0f, 0.0f, 0.25f, 0.25f );
	iconInfo[1].Set( 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f );

	surface.Set( 256, 256, 4 );		// All the memory we will ever need (? or that is the intention)

	LoadTextures();
	LoadModels();

	iconTexture = &texture[1];
	mapTexture = &texture[2];

	memset( testModel, 0, NUM_TEST_MODEL*sizeof(Model*) );

	int n = 0;
	ModelResource* resource = 0;
	resource = GetResource( "teapot" );
	testModel[n] = engine.GetModel( resource );
	testModel[n++]->SetPos( 5.f, -(float)resource->bounds[0].y/65536.0f, 60.f );

	resource = GetResource( "test2" );
	testModel[n] = engine.GetModel( resource );
	testModel[n++]->SetPos( 2.0f, 0.0f, 60.f );

	resource = GetResource( "crate" );
	testModel[n] = engine.GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 3.5f, 0.0f, 58.5f );
}


Game::~Game()
{
	for( int i=0; i<NUM_TEST_MODEL; ++i ) {
		if ( testModel[i] ) {
			engine.ReleaseModel( testModel[i] );
		}
	}

	FreeModels();
	FreeTextures();
}


void Game::LoadTextures()
{
	memset( texture, 0, sizeof(Texture)*MAX_TEXTURES );

	U32 textureID = 0;
	FILE* fp = 0;
	char buffer[512];

	for( int i=0; gTextureNames[i]; ++i ) {
		PlatformPathToResource( gTextureNames[i], "tex", buffer, 512 );
		fp = fopen( buffer, "rb" );
		GLASSERT( fp );
		textureID = surface.LoadTexture( fp );
		texture[ nTexture++ ].Set( gTextureNames[i], textureID );
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
		for( U32 j=0; j<modelResource[i].nGroups; ++j ) {
			glDeleteBuffers( 1, (const GLuint*) &modelResource[i].indexID[j] );		
		}
		glDeleteBuffers( 1, (const GLuint*) &modelResource[i].vertexID );
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

	testModel[0]->SetYRotation( testModel[0]->GetYRotation() + 0.5f );
	//if ( testModel[3] ) {
	//	testModel[3]->SetYRotation( testModel[3]->GetYRotation() + 0.1f );
	//}

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glBindTexture( GL_TEXTURE_2D, mapTexture->glID );	// FIXME!! should be in map
	engine.Draw();

	glDisable( GL_DEPTH_TEST );

	int w = engine.Width();
	int h = engine.Height();
	if ( rotation&1 ) grinliz::Swap( &w, &h );

	UFODrawText( 0, 0, "UFO Attack! %.1ffps rot=%d shadow=%d", 
				framesPerSecond, rotation, engine.ShadowMode() );

	glBindTexture( GL_TEXTURE_2D, iconTexture->glID );
	UFODrawIcons( iconInfo, w, h, rotation );

	glEnable( GL_DEPTH_TEST );

}


void Game::Tap( int count, int _x, int _y )
{
	int x = _y;
	int y = engine.Width() - _x;

	GLOUTPUT(( "Tap count=%d x=%d y=%d\n", count, x, y ));
	for( const IconInfo* icon = iconInfo; icon->iconID > 0; ++icon ) {
		if (    grinliz::InRange( x, icon->pos.x, icon->pos.x+icon->size.x )
			 && grinliz::InRange( y, icon->pos.y, icon->pos.y+icon->size.y ) ) 
		{
			switch( icon->iconID ) {
				case ICON_CAMERA_HOME:
					if ( count == 1 )
						engine.MoveCameraHome();
					break;

				default:
					GLASSERT( 0 );
					break;
			}
		}
	}
}
