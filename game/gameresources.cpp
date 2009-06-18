#include "game.h"
#include "cgame.h"
#include "unit.h"
#include "material.h"

// Only need non-item models.
const char* const gModelNames[] = 
{
	"alien0",
	"alien1",
	"alien2",
	"alien3",
	"alien0_D",
	"alien1_D",
	"alien2_D",
	"alien3_D",
	"crate",

	"maleMarine",
	"femaleMarine",
	"maleCiv",
	"femaleCiv",
	"maleMarine_D",
	"femaleMarine_D",
	"maleCiv_D",
	"femaleCiv_D",

	"gun0",
	"gun1",

	"selection",

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
	{	"woodDark",		0	},
	{	"woodDarkUFO",	0	},
	{	"woodPlank",	0	},
	{	"woodPlankD",	0	},
	{	"marine",		0	},
	{	"palette",		0	},
	{	"ufoOuter",		0	},
	{	"ufoInner",		0	},
	{	"lannder",		0	},
	{	"tree",			ALPHA_TEST	},
	{	"wheat",		ALPHA_TEST	},
	{	"farmland",		0	},
	{	"particleQuad",	0	},
	{	"particleSparkle",	0	},
	{	"translucent", 0 },
	{  0, 0 }
};


const char* gLightMapNames[ Game::NUM_LIGHT_MAPS ] = 
{
	"farmlandN",
};



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


void Game::LoadTextures()
{
	memset( texture, 0, sizeof(Texture)*MAX_TEXTURES );

	U32 textureID = 0;
	FILE* fp = 0;
	char buffer[512];

	// Create the default texture "white"
	surface.Set( 2, 2, 2 );
	memset( surface.Pixels(), 255, 8 );
	textureID = surface.CreateTexture( false );
	texture[ nTexture++ ].Set( "white", textureID );

	// Load the textures from the array:
	for( int i=0; gTextureDef[i].name; ++i ) {
		PlatformPathToResource( gTextureDef[i].name, "tex", buffer, 512 );
		fp = fopen( buffer, "rb" );
		GLASSERT( fp );
		bool alpha;
		textureID = surface.LoadTexture( fp, &alpha );
		//bool alphaTest = (gTextureDef[i].flags & ALPHA_TEST ) ? true : false;

		texture[ nTexture++ ].Set( gTextureDef[i].name, textureID, alpha );
		fclose( fp );
	}
	GLASSERT( nTexture <= MAX_TEXTURES );
}


ModelResource* Game::GetResource( const char* name )
{
	for( int i=0; i<nModelResource; ++i ) {
		if ( strcmp( modelResource[i].header.name, name ) == 0 ) {
			return &modelResource[i];
		}
	}
	return 0;
}


void Game::LoadModel( const char* name )
{
	char buffer[256];
	GLASSERT( modelLoader );

	GLASSERT( nModelResource < EL_MAX_MODEL_RESOURCES );
	PlatformPathToResource( name, "mod", buffer, 256 );
	FILE* fp = fopen( buffer, "rb" );
	GLASSERT( fp );
	modelLoader->Load( fp, &modelResource[nModelResource] );
	fclose( fp );
	nModelResource++;
}


void Game::LoadModels()
{
	memset( modelResource, 0, sizeof(ModelResource)*EL_MAX_MODEL_RESOURCES );

	//FILE* fp = 0;

	for( int i=0; gModelNames[i]; ++i ) {
		LoadModel( gModelNames[i] );
	}
}


Surface* Game::GetLightMap( const char* name )
{
	for( int i=0; i<NUM_LIGHT_MAPS; ++i ) {
		if ( strcmp( name, gLightMapNames[i] ) == 0 ) {
			return &lightMaps[i];
		}
	}
	return 0;
}


void Game::LoadLightMaps()
{
	// Load the map.
	char buffer[512];
	for( int i=0; i<NUM_LIGHT_MAPS; ++i ) {
		PlatformPathToResource( gLightMapNames[i], "tex", buffer, 512 );
		FILE* fp = fopen( buffer, "rb" );
		GLASSERT( fp );
		bool alpha;
		lightMaps[i].LoadTexture( fp, &alpha );
		GLASSERT( alpha == false );
		fclose( fp );
	}
}


void Game::InitMapItemDef( int index, const MapItemInit* init )
{
	while( init->model ) 
	{
		Map::MapItemDef* itemDef = engine.GetMap()->InitItemDef( index );
		itemDef->Init();

		itemDef->cx = init->cx;
		itemDef->cy = init->cy;
		itemDef->hp = init->hp;
		itemDef->materialFlags = init->materialFlags;
		//itemDef->flammable = init->flammable;
		//itemDef->explosive = init->explosive;


		{
			ModelResource* resource = 0;
			resource = GetResource( init->model );
			if ( !resource ) {
				LoadModel( init->model );
				resource = GetResource( init->model );
			}
			GLASSERT( resource );
			itemDef->modelResource = resource;
		}
		{
			ModelResource* resource = 0;
			if ( init->modelOpen ) {
				resource = GetResource( init->modelOpen );
				if ( !resource ) {
					LoadModel( init->modelOpen );
					resource = GetResource( init->modelOpen );
				}
				GLASSERT( resource );
			}
			itemDef->modelResourceOpen = resource;
		}
		{
			ModelResource* resource = 0;
			if ( init->modelDestroyed ) {
				resource = GetResource( init->modelDestroyed );
				if ( !resource ) {
					LoadModel( init->modelDestroyed );
					resource = GetResource( init->modelDestroyed );
				}
				GLASSERT( resource );
			}
			itemDef->modelResourceDestroyed = resource;
		}

		strncpy( itemDef->name, init->Name(), EL_FILE_STRING_LEN );
		
		// Parse the pathing
		//	spaces are ignored
		//	S-1, E-2, N-4, W-8
		for( int k=0; k<2; ++k ) {
			const char* p = (k==0) ? init->pather0 : init->pather1;

			int i=0;
			while( p && *p ) {
				if ( *p >= '0' && *p <= '9' ) {
					itemDef->pather[k][i/init->cx][i%init->cx] = *p - '0';
					++i;
				}
				else if ( *p >= 'a' && *p <= 'f' ) {
					itemDef->pather[k][i/init->cx][i%init->cx] = 10 + *p - 'a';
					++i;
				}
				else if ( *p == ' ' ) {
					// nothing.
				}
				else {
					GLASSERT( 0 );
				}
				p++;
			}
		}
		++init;
		++index;
	}
}

	
void Game::LoadMapResources()
{
	/*
		Groups:

		Forest
		Farmhouse
		Town
		Base
		UFOs
	*/

	const int HP_LOW = 10;
	const int HP_MED = 100;
	const int HP_HIGH = 300;
	const int HP_STEEL = 1000;
	const int INDESTRUCT = 0;

	const int FOREST_SET = 0x01;
	const int FARM_SET	 = 0x10;
	const int WOOD_SET	 = 0x20;
	const int TOWN_SET   = 0x30;
	const int UFO_SET0	 = 0x40;
	const int UFO_SET1	 = 0x50;
	const int MARINE_SET = 0x60;

	const int STEEL		= MaterialDef::MAT_STEEL;
	const int WOOD		= MaterialDef::MAT_WOOD;
	const int GENERIC	= MaterialDef::MAT_GENERIC;
	const int GENERIC_FASTBURN	= MaterialDef::MAT_GENERIC;


	const MapItemInit farmSet[] =
	{
			// model		open			destroyed	cx, cz	hp		material		pather0	pather1
		{	"farmBed",		0,				0,			1,	1,	HP_MED,	GENERIC,		"f",	"0" },
		{	"farmTable",	0,				0,			1,	1,	HP_MED,	WOOD,			"f",	"0" },
		{	"farmTable2x1",	0,				0,			2,	1,	HP_MED,	WOOD,			"ff",	"0" },
		{	"farmWheat",	0,				0,			1,	1,	HP_LOW,	GENERIC_FASTBURN,"0",	"0" },
		{	0	}
	};
	InitMapItemDef( FARM_SET, farmSet );
  
	const MapItemInit marineSet[] =
	{
			// model		open			destroyed	cx, cz	hp			material	pather0	pather1
		{	"lander",		0,				0,			6,	6,	INDESTRUCT,	STEEL,		"00ff00 00ff00 ff00ff ff00ff ff00ff ff00ff",	"0" },
		{	0	}
	};
	InitMapItemDef( MARINE_SET, marineSet );

	const MapItemInit forestSet[] = 
	{
			// model		open			destroyed	cx, cz	hp		 material		pather0	pather1
		{	"tree",			0,				0,			1,	1,	HP_HIGH, WOOD,			"f",	"0" },
		{	0	}
	};
	InitMapItemDef( FOREST_SET, forestSet );

	const MapItemInit ufoSet[] = 
	{
			// model		open			destroyed	cx, cz	hp			material		pather0	pather1
		{	"ufo_Diag",		0,				0,			1,	1,	HP_STEEL,	STEEL,		"f",	"0" },
		{	"ufo_DoorCld",	"ufo_DoorOpn",	0,			1,	1,	HP_STEEL,	STEEL,		"0",	"0" },
		{	"ufo_WallInn",	0,				0,			1,	1,	HP_STEEL,	STEEL,		"1",	"0" },
		{	"ufo_CrnrInn",	0,				0,			1,	1,	HP_STEEL,	STEEL,		"3",	"0" },
		{	"ufo_WallOut",	0,				0,			1,	1,	HP_STEEL,	STEEL,		"1",	"0" },
		{	"ufo_Join0",	0,				0,			2,	1,	HP_STEEL,	STEEL,		"1f",	"0" },
		{	"ufo_Join1",	0,				0,			1,	2,	HP_STEEL,	STEEL,		"2f",	"0" },
		{	0	}
	};
	InitMapItemDef( UFO_SET0, ufoSet );

	const MapItemInit woodSet[] =
	{
			// model		open			destroyed	cx, cz	hp		material		pather0	pather1
		{	"woodCrnr",		0,				"woodCrnrD",1,	1,	HP_MED,	WOOD,			"3",	"0" },
		{	"woodDoorCld",	"woodDoorOpn",	0,			1,	1,	HP_MED,	WOOD,			"0",	"0" },
		{	"woodWall",		"woodWall",		0,			1,	1,	HP_MED,	WOOD,			"1",	"0" },
		{	"woodWallWin",	"woodWallWin",	0,			1,	1,	HP_MED,	WOOD,			"1",	"0" },
		{	0	}
	};
	InitMapItemDef(  WOOD_SET, woodSet );
}


void Game::LoadItemResources()
{
	ItemDef item;

	item.InitWeapon( "gun0", GetResource( "gun0" ),		MaterialDef::SH_KINETIC,	50 );
	itemDefArr.Push( item );

	item.InitWeapon( "gun1", GetResource( "gun1" ),		MaterialDef::SH_ENERGY,	70 );
	itemDefArr.Push( item );
}


const ItemDef* Game::GetItemDef( const char* name )
{
	for( unsigned i=0; i<itemDefArr.Size(); ++i ) {
		if ( strcmp( itemDefArr[i].name, name ) == 0 ) {
			return &itemDefArr[i];
		}
	}
	GLASSERT( 0 );
	return 0;
}
