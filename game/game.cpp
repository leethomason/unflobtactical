#include "game.h"
#include "cgame.h"
#include "../engine/platformgl.h"
#include "../engine/text.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"

#include "../grinliz/glmatrix.h"
#include "../grinliz/glutil.h"


const char* const gModelNames[] = 
{
	"test1",
	"test2",
	"teapot",
	"crate",
	
	// Maps (need to move out?)
	"farmland",

	// Characters
	"maleMarine",
	"femaleMarine",
	"alien0",
	"alien1",
	"alien2",
	"alien3",
	
	// mapmaker
	"selection",

	// Farmland tiles
	"farmland",
	"wallWoodSh",
	"doorWoodOpen",
	"doorWoodCl",
	
	// UFO tiles
	"wallUFO",
	"diagUFO",
	"doorUFOOp",
	"doorUFOCl",
	"wallUFOInner",

	// decor
	"bed",
	"table",
	"tree",
	"wheat",

	0
};

struct TextureDef
{
	const char* name;
	U32 flags;
};

enum {
	ALPHA_TEST = 0x01,
};

const TextureDef gTextureDef[] = 
{
	{	"icons",		0	},
	{	"stdfont2",		0	},
	{	"grass2",		0	},
	{	"dirtGrass",	0	},
	{	"alienFloor",	0	},
	{	"woodDark",		0	},
	{	"woodDarkUFO",	0	},
	{	"woodPlank",	0	},
	{	"marine",		0	},
	{	"palette",		0	},
	{	"ufoOuter",		0	},
	{	"ufoInner",		0	},
	{	"tree",			ALPHA_TEST	},
	{	"wheat",		ALPHA_TEST	},
	{	"farmland",		0	},
	{  0, 0 }
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
	framesPerSecond( 0 ),
	trianglesPerSecond( 0 ),
	trianglesSinceMark( 0 ),
	currentMapItem( 1 )
{
	iconInfo[0].Set( ICON_CAMERA_HOME, 0, 270, 50, 50, 0.0f, 0.0f, 0.25f, 0.25f );
	iconInfo[1].Set( 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f );

	surface.Set( 256, 256, 4 );		// All the memory we will ever need (? or that is the intention)

	LoadTextures();
	LoadModels();
	LoadMapResources();

	iconTexture = GetTexture( "icons" );
	Texture* textTexture = GetTexture( "stdfont2" );
	GLASSERT( textTexture );
	UFOInitDrawText( textTexture->glID, engine.Width(), engine.Height(), rotation );

	memset( testModel, 0, NUM_TEST_MODEL*sizeof(Model*) );

	int n = 0;
	ModelResource* resource = 0;

#ifdef MAPMAKER
	resource = GetResource( "selection" );
	selection = engine.GetModel( resource );
	selection->SetPos( 0.5f, 0.0f, 0.5f );
#else
	// If we aren't the map maker, then we need to load a map.
	LoadMap( "farmland" );
#endif

	// Load the map base.
	resource = GetResource( "farmland" );
	mapModel = engine.GetModel( resource );
	mapModel->HideFromTree( true );				// don't want to double render
	engine.GetMap()->SetModel( mapModel );
	//float mx = (float)engine.GetMap()->Width();
	float mz = 40.0f; //(float)engine.GetMap()->Height();

	//engine.camera.SetPosWC( -19.4f, 62.0f, 57.2f );
	engine.camera.SetPosWC( -12.f, 45.f, 52.f );	// standard test
	//engine.camera.SetPosWC( -5.0f, engineData.cameraHeight, mz + 5.0f );


	/*
	resource = GetResource( "teapot" );
	testModel[n] = engine.GetModel( resource );
	testModel[n++]->SetPos( 5.f, -(float)resource->bounds[0].y/65536.0f, mz-4.0f );

	resource = GetResource( "test2" );
	testModel[n] = engine.GetModel( resource );
	testModel[n++]->SetPos( 2.0f, 0.0f, mz-4.0f );
	*/
	resource = GetResource( "crate" );
	testModel[n] = engine.GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 3.5f, 0.0f, mz-5.5f );

	resource = GetResource( "maleMarine" );
	for( int i=0; i<4; ++i ) {
		testModel[n] = engine.GetModel( resource );
		testModel[n]->SetPos( (float)(i*2)+10.5f, 0.0f, mz-7.5f );
		testModel[n]->SetDraggable( true );
		++n;
	}
	testModel[n-4]->SetSkin( 1, 1, 1 );
	testModel[n-3]->SetSkin( 2, 1, 1 );
	testModel[n-2]->SetSkin( 1, 0, 0 );
	testModel[n-1]->SetSkin( 1, 0, 2 );

	resource = GetResource( "femaleMarine" );
	testModel[n] = engine.GetModel( resource );
	rotTestStart = n;
	rotTestCount = 0;
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 19.5f, 0.0f, mz-9.5f );
	
	resource = GetResource( "alien0" );
	testModel[n] = engine.GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 19.5f, 0.0f, mz-7.5f );

	resource = GetResource( "alien1" );
	testModel[n] = engine.GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 19.5f, 0.0f, mz-5.5f );

	resource = GetResource( "alien2" );
	testModel[n] = engine.GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 19.5f, 0.0f, mz-3.5f );

	resource = GetResource( "alien3" );
	testModel[n] = engine.GetModel( resource );
	testModel[n]->SetDraggable( true );
	testModel[n++]->SetPos( 19.5f, 0.0f, mz-1.5f );
}


Game::~Game()
{
	for( int i=0; i<NUM_TEST_MODEL; ++i ) {
		if ( testModel[i] ) {
			engine.ReleaseModel( testModel[i] );
		}
	}
	engine.ReleaseModel( mapModel );
#ifdef MAPMAKER
	engine.ReleaseModel( selection );
#endif

	FreeModels();
	FreeTextures();
}


void Game::LoadMapResources()
{
	Map* map = engine.GetMap();
	map->SetItemDef( 1, GetResource( "wallWoodSh" ) );
	map->SetItemDef( 2, GetResource( "doorWoodCl" ) );
	map->SetItemDef( 3, GetResource( "table" ) );
	map->SetItemDef( 4, GetResource( "bed" ) );
	map->SetItemDef( 5, GetResource( "diagUFO" ) );
	map->SetItemDef( 6, GetResource( "wallUFO" ) );
	map->SetItemDef( 7, GetResource( "doorUFOOp" ) );
	map->SetItemDef( 8, GetResource( "doorUFOCl" ) );
	map->SetItemDef( 9, GetResource( "wallUFOInner" ) );
	map->SetItemDef( 10, GetResource( "tree" ) );
	map->SetItemDef( 11, GetResource( "wheat" ) );
}


void Game::LoadTextures()
{
	memset( texture, 0, sizeof(Texture)*MAX_TEXTURES );

	U32 textureID = 0;
	FILE* fp = 0;
	char buffer[512];

	// Create the default texture "white"
	surface.Set( 2, 2, 3 );
	memset( surface.Pixels(), 255, 16 );
	textureID = surface.CreateTexture();
	texture[ nTexture++ ].Set( "white", textureID );

	// Load the textures from the array:
	for( int i=0; gTextureDef[i].name; ++i ) {
		PlatformPathToResource( gTextureDef[i].name, "tex", buffer, 512 );
		fp = fopen( buffer, "rb" );
		GLASSERT( fp );
		textureID = surface.LoadTexture( fp );
		bool alphaTest = (gTextureDef[i].flags & ALPHA_TEST ) ? true : false;
		texture[ nTexture++ ].Set( gTextureDef[i].name, textureID, alphaTest );
		fclose( fp );
	}
	GLASSERT( nTexture <= MAX_TEXTURES );
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


Texture* Game::GetTexture( const char* name )
{
	for( int i=0; i<nTexture; ++i ) {
		if ( strcmp( texture[i].name, name ) == 0 ) {
			return &texture[i];
		}
	}
	GLASSERT( 0 );
	return 0;
}


void Game::LoadMap( const char* name )
{
	char buffer[512];

	PlatformPathToResource( name, "map", buffer, 512 );
	FILE* fp = fopen( buffer, "rb" );
	GLASSERT( fp );

	LoadMap( fp );
	fclose( fp );
}


void Game::LoadModels()
{
	ModelLoader* loader = new ModelLoader( texture, nTexture );
	memset( modelResource, 0, sizeof(ModelResource)*EL_MAX_MODEL_RESOURCES );

	FILE* fp = 0;
	char buffer[512];

	for( int i=0; gModelNames[i]; ++i ) {
		GLASSERT( i < EL_MAX_MODEL_RESOURCES );
		PlatformPathToResource( gModelNames[i], "mod", buffer, 512 );
		fp = fopen( buffer, "rb" );
		GLASSERT( fp );
		loader->Load( fp, &modelResource[i] );
		fclose( fp );
		nModelResource++;
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
			glDeleteBuffers( 1, (const GLuint*) &modelResource[i].atom[i].indexID );		
		}
		glDeleteBuffers( 1, (const GLuint*) &modelResource[i].atom[i].vertexID );
	}
}


void Game::DoTick( U32 currentTime )
{
	if ( markFrameTime == 0 ) {
		markFrameTime			= currentTime;
		frameCountsSinceMark	= 0;
		framesPerSecond			= 0.0f;
		trianglesPerSecond		= 0;
		trianglesSinceMark		= 0;
	}
	else {
		++frameCountsSinceMark;
		if ( currentTime - markFrameTime > 500 ) {
			framesPerSecond		= 1000.0f*(float)(frameCountsSinceMark) / ((float)(currentTime - markFrameTime));
			// actually K-tris/second
			trianglesPerSecond  = trianglesSinceMark / (currentTime - markFrameTime);
			markFrameTime		= currentTime;
			frameCountsSinceMark = 0;
			trianglesSinceMark = 0;
		}
	}

#ifdef EL_SHOW_MODELS
	for( int i=0; i<nModelResource; ++i ) {
		for ( unsigned k=0; k<modelResource[i].nGroups; ++ k ) {
			modelResource[i].atom[k].trisRendered = 0;
		}
	}
#endif

	for( int i=0; i<rotTestCount; ++i ) {
		Model* m = testModel[rotTestStart+i];
		m->SetYRotation( m->GetYRotation() + 0.3f );
	}

	// FIXME: check that all client state is always submitted
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	int triCount = 0;
	engine.Draw( &triCount );
	trianglesSinceMark += triCount;

	glDisable( GL_DEPTH_TEST );

	int w = engine.Width();
	int h = engine.Height();
	if ( rotation&1 ) grinliz::Swap( &w, &h );

	UFODrawText( 0,  0, "UFO Attack! s=%d %4.1ffps %5.1fK/f %4dK/s", 
				 engine.ShadowMode(),
				 framesPerSecond, 
				 (float)triCount/1000.0f,
				 trianglesPerSecond );

#ifdef EL_SHOW_MODELS
	int k=0;
	while ( k < nModelResource ) {
		int total = 0;
		for( unsigned i=0; i<modelResource[k].nGroups; ++i ) {
			total += modelResource[k].atom[i].trisRendered;
		}
		UFODrawText( 0, 12+12*k, "%16s %5d K", modelResource[k].name, total );
		++k;
	}
#endif
#ifdef MAPMAKER
	UFODrawText( 0,  16, "%3d:'%s'", currentMapItem, engine.GetMap()->GetItemDefName( currentMapItem ) );
#endif
	glBindTexture( GL_TEXTURE_2D, iconTexture->glID );
	UFODrawIcons( iconInfo, w, h, rotation );

	glEnable( GL_DEPTH_TEST );

}


void Game::TransformScreen( int x0, int y0, int *x1, int *y1 )
{
	switch ( rotation ) {
		case 0:
			*x1 = x0;
			*y1 = y0;
			break;

		case 1:
			*x1 = y0;
			*y1 = engine.Width() - 1 - x0;
			break;

		case 2:
			*x1 = engine.Width() - 1 - x0;
			*y1 = y0;
			break;

		case 3:
			*x1 = engine.Height() - 1 - y0;
			*y1 = x0;
			break;

		default:
			GLASSERT( 0 );
			break;
	}
}



void Game::Tap( int tap, int x, int y )
{
	if ( tap == 1 ) {
		// In the double-tap case, the inverse computation will take care
		// of the coordinate transform. Here we do it explicity via TransformScreen.
		int x0, y0;
		TransformScreen( x, y, &x0, &y0 );
		//GLOUTPUT(( "Screen: %d,%d\n", x0, y0 ));

		bool iconFound = false;
		for( const IconInfo* icon = iconInfo; icon->iconID > 0; ++icon ) {
			if (    grinliz::InRange( x0, icon->pos.x, icon->pos.x+icon->size.x )
				 && grinliz::InRange( y0, icon->pos.y, icon->pos.y+icon->size.y ) ) 
			{
				switch( icon->iconID ) {
					case ICON_CAMERA_HOME:
						engine.MoveCameraHome();
						iconFound = true;
						break;

					default:
						GLASSERT( 0 );
						break;
				}
			}
		}

#ifdef MAPMAKER
		if ( !iconFound ) {
			const Vector3X& pos = selection->Pos();
			int rotation = (int) (selection->GetYRotation() / grinliz::Fixed(90) );
			engine.GetMap()->AddToTile( (int)pos.x, (int)pos.z, currentMapItem, rotation );
		}
#endif	

	}
	else if ( tap == 2 ) {
		grinliz::Matrix4 mvpi;
		grinliz::Ray ray;
		grinliz::Vector3F p;

		engine.CalcModelViewProjectionInverse( &mvpi );
		engine.RayFromScreenToYPlane( x, y, mvpi, &ray, &p );
		engine.MoveCameraXZ( p.x, p.z ); 
	}
}


#ifdef MAPMAKER

void Game::MouseMove( int x, int y )
{
	grinliz::Matrix4 mvpi;
	grinliz::Ray ray;
	grinliz::Vector3F p;

	engine.CalcModelViewProjectionInverse( &mvpi );
	engine.RayFromScreenToYPlane( x, y, mvpi, &ray, &p );

	int newX = (int)( p.x );
	int newZ = (int)( p.z );
	selection->SetPos( (float)newX + 0.5f, 0.0f, (float)newZ + 0.5f );
}

void Game::RotateSelection()
{
	grinliz::Fixed rot = selection->GetYRotation() + grinliz::Fixed(90);
	selection->SetYRotation( rot );
}

void Game::DeleteAtSelection()
{
	Vector3X pos = selection->Pos();
	engine.GetMap()->DeleteAt( (int)pos.x, (int)pos.z );
}


void Game::DeltaCurrentMapItem( int d )
{
	currentMapItem += d;
	while ( currentMapItem < 0 ) { currentMapItem += Map::MAX_ITEM_DEF; }
	while ( currentMapItem >= Map::MAX_ITEM_DEF ) { currentMapItem -= Map::MAX_ITEM_DEF; }
	if ( currentMapItem == 0 ) currentMapItem = 1;
}


#endif
