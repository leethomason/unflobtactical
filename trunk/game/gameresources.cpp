#include "game.h"
#include "cgame.h"
#include "unit.h"
#include "material.h"
#include "../shared/gldatabase.h"
#include "../grinliz/glstringutil.h"


void Game::LoadTextures()
{
	U32 textureID = 0;
	char name[64];
	char format[16];

	TextureManager* texman = TextureManager::Instance();

	// Create the default texture "white"
	surface.Set( Surface::RGB16, 2, 2 );
	memset( surface.Pixels(), 255, 8 );
	textureID = surface.CreateTexture();
	texman->AddTexture( "white", textureID, false );

	// Run through the database, and load all the textures.
	sqlite3_stmt* stmt = 0;
	sqlite3_prepare_v2(database, "SELECT * FROM texture WHERE image=0;", -1, &stmt, 0 );
	BinaryDBReader reader( database );

	while (sqlite3_step(stmt) == SQLITE_ROW) 
	{
		strcpy( name, (const char*)sqlite3_column_text( stmt, 0 ) );	// name
		strcpy( format, (const char*)sqlite3_column_text( stmt, 1 ) );	// format
		//int isImage = sqlite3_column_int( stmt, 2 );					// don't need
		int w = sqlite3_column_int( stmt, 3 );
		int h = sqlite3_column_int( stmt, 4 );
		int id = sqlite3_column_int( stmt, 5 );

		surface.Set( Surface::QueryFormat( format ), w, h );
		
		int blobSize = 0;
		reader.ReadSize( id, &blobSize );
		GLASSERT( surface.BytesInImage() == blobSize );
		reader.ReadData( id, blobSize, surface.Pixels() );

		textureID = surface.CreateTexture();
		texman->AddTexture( name, textureID, surface.Alpha() );
		GLOUTPUT(( "Texture %s\n", name ));
	}

	sqlite3_finalize(stmt);
}



void Game::LoadModel( const char* name )
{
	GLASSERT( modelLoader );

	ModelResource* res = new ModelResource();
	modelLoader->Load( database, name, res );
	ModelResourceManager::Instance()->AddModelResource( res );
}


void Game::LoadModels()
{
	char name[64];

	// Run through the database, and load all the textures.
	sqlite3_stmt* stmt = 0;
	sqlite3_prepare_v2(database, "select name from model;", -1, &stmt, 0 );

	while (sqlite3_step(stmt) == SQLITE_ROW) 
	{
		strcpy( name, (const char*)sqlite3_column_text( stmt, 0 ) );		// name
		LoadModel( name );
	}

	sqlite3_finalize(stmt);

}


Surface* Game::GetLightMap( const char* name )
{
	for( int i=0; i<nLightMaps; ++i ) {
		if ( strcmp( name, lightMaps[i].Name() ) == 0 ) {
			return &lightMaps[i];
		}
	}
	return 0;
}


void Game::LoadLightMaps()
{
	char name[64];
	char format[16];
	nLightMaps = 0;
	BinaryDBReader reader( database );

	// Run through the database, and load all the images.
	sqlite3_stmt* stmt = 0;
	sqlite3_prepare_v2(database, "SELECT * FROM texture WHERE image=1;", -1, &stmt, 0 );

	while (sqlite3_step(stmt) == SQLITE_ROW) 
	{
		GLASSERT( nLightMaps < MAX_NUM_LIGHT_MAPS );

		strcpy( name, (const char*)sqlite3_column_text( stmt, 0 ) );	// name
		strcpy( format, (const char*)sqlite3_column_text( stmt, 1 ) );	// format
		//int isImage = sqlite3_column_int( stmt, 2 );					// don't need
		int w = sqlite3_column_int( stmt, 3 );
		int h = sqlite3_column_int( stmt, 4 );
		int id = sqlite3_column_int( stmt, 5 );

		lightMaps[nLightMaps].Set( Surface::QueryFormat( format ), w, h );
		
		int blobSize = 0;
		reader.ReadSize( id, &blobSize );
		GLASSERT( lightMaps[nLightMaps].BytesInImage() == blobSize );
		reader.ReadData( id, blobSize, lightMaps[nLightMaps].Pixels() );
		lightMaps[nLightMaps].SetName( name );

		GLOUTPUT(( "LightMap %s\n", lightMaps[nLightMaps].Name() ));
		nLightMaps++;
	}

	sqlite3_finalize(stmt);
}


void Game::InitMapLight( int index, const MapLightInit* init )
{
	while( init->name ) 
	{
		Map::MapItemDef* itemDef = engine.GetMap()->InitItemDef( index );
		itemDef->Init();
	
		GLASSERT( init->x || init->y );

		strncpy( itemDef->name, init->name, EL_FILE_STRING_LEN );

		itemDef->lightOffsetX = init->objectX;
		itemDef->lightOffsetY = init->objectY;
		itemDef->lightTX = init->x;
		itemDef->lightTY = init->y;
		itemDef->cx = init->cx;
		itemDef->cy = init->cy;
		itemDef->isUpperLeft = init->upperLeft ? 1 : 0;

		++init;
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
		itemDef->lightDef = init->lightDef;

		ModelResourceManager* modman = ModelResourceManager::Instance();
		{
			const ModelResource* resource = 0;
			resource = modman->GetModelResource( init->model, false );
			if ( !resource ) {
				LoadModel( init->model );
				resource = modman->GetModelResource( init->model, true );
			}
			GLASSERT( resource );
			itemDef->modelResource = resource;

			itemDef->isUpperLeft = resource->IsOriginUpperLeft() ? 1 : 0;
		}
		{
			const ModelResource* resource = 0;
			if ( init->modelOpen ) {
				resource = modman->GetModelResource( init->modelOpen, false );
				if ( !resource ) {
					LoadModel( init->modelOpen );
					resource = modman->GetModelResource( init->modelOpen, true );
				}
				GLASSERT( resource );
			}
			itemDef->modelResourceOpen = resource;
		}
		{
			const ModelResource* resource = 0;
			if ( init->modelDestroyed ) {
				resource = modman->GetModelResource( init->modelDestroyed, false );
				if ( !resource ) {
					LoadModel( init->modelDestroyed );
					resource = modman->GetModelResource( init->modelDestroyed, true );
				}
				GLASSERT( resource );
			}
			itemDef->modelResourceDestroyed = resource;
		}

		strncpy( itemDef->name, init->Name(), EL_FILE_STRING_LEN );
		
		//  Parse the pathing
		//	spaces are ignored
		//	S-1, E-2, N-4, W-8
		const char* p =init->pather;

		int i=0;
		while( p && *p ) {
			if ( *p >= '0' && *p <= '9' ) {
				itemDef->pather[i/init->cx][i%init->cx] = *p - '0';
				++i;
			}
			else if ( *p >= 'a' && *p <= 'f' ) {
				itemDef->pather[i/init->cx][i%init->cx] = 10 + *p - 'a';
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


		//  Parse the visibility
		//	spaces are ignored
		//	S-1, E-2, N-4, W-8
		p =init->visibility;

		if ( !p || !*p ) {
			// If not specified, then visibility is the same as pathing. (The common case
			// when there are windows, or the object is short.)
			memcpy( itemDef->visibility, itemDef->pather, Map::MapItemDef::MAX_CX*Map::MapItemDef::MAX_CY );
		}
		else {
			i=0;
			while( p && *p ) {
				if ( *p >= '0' && *p <= '9' ) {
					itemDef->visibility[i/init->cx][i%init->cx] = *p - '0';
					++i;
				}
				else if ( *p >= 'a' && *p <= 'f' ) {
					itemDef->visibility[i/init->cx][i%init->cx] = 10 + *p - 'a';
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
	const int INDESTRUCT = 0xffff;

	const int FOREST_SET = 0x01;
	const int FARM_SET	 = 0x10;
	const int WOOD_SET	 = 0x20;
	const int TOWN_SET   = 0x30;
	const int UFO_SET0	 = 0x40;
	const int UFO_SET1	 = 0x50;
	const int MARINE_SET = 0x60;

	const int LIGHT_SET  = Map::LIGHT_START;

	const int STEEL		= MAT_STEEL;
	const int WOOD		= MAT_WOOD;
	const int GENERIC	= MAT_GENERIC;
	const int GENERIC_FASTBURN	= MAT_GENERIC;

	enum {
		LANDER_LIGHT = LIGHT_SET
	};
	
	const MapLightInit lights[] = 
	{
		//	name			object   x  y   cx  cy	upperLeft 
		{	"landerLight",	-1,	-1,	 1,	0,	8,	8,	true },
		{	0	}
	};
	InitMapLight( LIGHT_SET, lights );


	const MapItemInit farmSet[] =
	{
			// model		open			destroyed	cx, cz	hp		material		pather visibility
		{	"farmBed",		0,				0,			1,	1,	HP_MED,	GENERIC,		"f"	 "0"	},
		{	"farmTable",	0,				0,			1,	1,	HP_MED,	WOOD,			"f", "0" 	},
		{	"farmTable2x1",	0,				0,			2,	1,	HP_MED,	WOOD,			"ff","00"	},
		{	"farmWheat",	0,				0,			1,	1,	HP_LOW,	GENERIC_FASTBURN,"0", "0" },
		{	0	}
	};
	InitMapItemDef( FARM_SET, farmSet );
  
	Map::LightItemDef lightLander = { 0, 0, 6, 6 };

	const MapItemInit marineSet[] =
	{
			// model		open			destroyed	cx, cz	hp			material	pather
		{	"lander",		0,				0,			6,	6,	INDESTRUCT,	STEEL,		"00ff00 00ff00 ff00ff ff00ff ff00ff ff00ff",
																						"00ff00 00ff00 0f00f0 0f00f0 0f00f0 0f00f0", 
																						LANDER_LIGHT },
		{	0	}
	};
	InitMapItemDef( MARINE_SET, marineSet );

	const MapItemInit forestSet[] = 
	{
			// model		open			destroyed	cx, cz	hp		 material		pather
		{	"tree",			0,				0,			1,	1,	HP_HIGH, WOOD,			"f", "0" },
		{	0	}
	};
	InitMapItemDef( FOREST_SET, forestSet );

	const MapItemInit ufoSet[] = 
	{
			// model		open			destroyed	cx, cz	hp			material		pather
		{	"ufo_Diag",		0,				0,			1,	1,	HP_STEEL,	STEEL,		"f" },
		{	"ufo_DoorCld",	"ufo_DoorOpn",	0,			1,	1,	HP_STEEL,	STEEL,		"0", "1" },
		{	"ufo_WallInn",	0,				0,			1,	1,	HP_STEEL,	STEEL,		"1" },
		{	"ufo_CrnrInn",	0,				0,			1,	1,	HP_STEEL,	STEEL,		"3" },
		{	"ufo_WallOut",	0,				0,			1,	1,	HP_STEEL,	STEEL,		"1" },
		{	"ufo_Join0",	0,				0,			2,	1,	HP_STEEL,	STEEL,		"1f" },
		{	"ufo_Join1",	0,				0,			1,	2,	HP_STEEL,	STEEL,		"2f" },
		{	0	}
	};
	InitMapItemDef( UFO_SET0, ufoSet );

	const MapItemInit woodSet[] =
	{
			// model		open			destroyed	cx, cz	hp		material		pather
		{	"woodCrnr",		0,				"woodCrnrD",1,	1,	HP_MED,	WOOD,			"3" },
		{	"woodDoorCld",	"woodDoorOpn",	0,			1,	1,	HP_MED,	WOOD,			"0", "1" },
		{	"woodWall",		"woodWall",		0,			1,	1,	HP_MED,	WOOD,			"1" },
		{	"woodWallWin",	"woodWallWin",	0,			1,	1,	HP_MED,	WOOD,			"1", "0" },
		{	0	}
	};
	InitMapItemDef(  WOOD_SET, woodSet );
}


void Game::LoadItemResources()
{
	const int DAM_LOW = 50;
	const int DAM_MED = 80;
	const int DAM_HI  = 120;
	const int DAM_VHI = 200;
	const int RANGE_MELEE = 1;
	const int RANGE_SHORT = 4;
	const int RANGE_MED = 8;
	const int RANGE_FAR = 12;
	const float ACC_LOW = 0.7f;
	const float ACC_MED = 1.0f;
	const float ACC_HI  = 1.3f;
	const int POW_LOW = 10;
	const int POW_MED = 20;
	const int POW_HI  = 50;

	memset( itemDefArr, 0, sizeof( ItemDef* )*EL_MAX_ITEM_DEFS );
	int nItemDef = 1;	// keep the first entry null

	struct WeaponInit {
		const char* name;
		const char* resName;
		int deco;
		const char* desc;

		int		shell0, 
				clip0, 
				auto0, 
				damage0, 
				range0;
		float	acc0;
		int		power0;

		int shell1, clip1, auto1, damage1, range1;
		float acc1;
		int power1;
	};

	const WeaponInit weapons[] = {		
		// Terran
		{ "PST-1",	"gun0",		DECO_PISTOL,	"Pistol",
								SH_KINETIC,		ITEM_CLIP_SHELL,	0, DAM_MED,	RANGE_SHORT,	ACC_MED, 0,
								0  },
		{ "AR-1",	"gun0",		DECO_RIFLE,		"Assault Rifle Standard Issue",
								SH_KINETIC,		ITEM_CLIP_AUTO,		3, DAM_LOW,	RANGE_MED,		ACC_MED, 0,
								0  },
		{ "AR-2",	"gun0",		DECO_RIFLE,		"Assault Rifle 'Vera'",
								SH_KINETIC,		ITEM_CLIP_AUTO,		3, DAM_MED,	RANGE_FAR,		ACC_HI,  0,
								SH_EXPLOSIVE,	ITEM_CLIP_GRENADE,	0, DAM_HI,	RANGE_MED,		ACC_LOW, 0 },
		{ "AR-3",	"gun0",		DECO_RIFLE,		"Pulse Rifle",
								SH_KINETIC,		ITEM_CLIP_AUTO,		4, DAM_MED,	RANGE_MED,		ACC_MED, 0,
								SH_EXPLOSIVE,	ITEM_CLIP_GRENADE,	0, DAM_HI,	RANGE_MED,		ACC_MED, 0 },
		{ "RKT",	"gun0",		DECO_RIFLE,		"Rocket Launcher",
								SH_EXPLOSIVE,	ITEM_CLIP_ROCKET,	0, DAM_HI,  RANGE_FAR,		ACC_MED, 0,
								0  },
		{ "CANON",	"gun0",		DECO_RIFLE,		"Mini-Canon",
								SH_KINETIC,		ITEM_CLIP_CANON,	3, DAM_HI,	RANGE_MED,		ACC_MED, 0,
								0 },
		{ "KNIFE",	"gun0",		DECO_KNIFE,		"Knife",
								SH_KINETIC,		0,					0, DAM_MED, RANGE_MELEE,	ACC_MED, 0,
								0  },

		// Alien
		{ "RAY-1",	"gun1",		DECO_PISTOL,	"Alien Ray Gun",
								SH_ENERGY,		ITEM_CLIP_CELL,		0, DAM_LOW,	RANGE_SHORT,	ACC_MED, POW_LOW,
								0  },
		{ "RAY-2",	"gun1",		DECO_PISTOL,	"Advanced Alien Ray Gun",
								SH_ENERGY,		ITEM_CLIP_CELL,		0, DAM_MED,	RANGE_MED,		ACC_MED, POW_LOW,
								0  },
		{ "PLS-1",	"gun0",		DECO_RIFLE,		"Plasma Rifle",
								SH_ENERGY,		ITEM_CLIP_CELL,		3, DAM_MED, RANGE_MED,		ACC_MED, POW_MED,
								0  },
		{ "PLS-2",	"gun0",		DECO_RIFLE,		"Plasma Assault Rifle",
								SH_ENERGY,		ITEM_CLIP_CELL,		3, DAM_MED, RANGE_FAR,		ACC_MED, POW_MED,
								SH_ENERGY|SH_EXPLOSIVE, ITEM_CLIP_CELL, 0, DAM_HI, RANGE_FAR,   ACC_LOW, POW_HI },
		{ "PLS-3",	"gun0",		DECO_RIFLE,		"Advanced Plasma Assault",
								SH_ENERGY,		ITEM_CLIP_CELL,		3, DAM_HI, RANGE_FAR,		ACC_HI, POW_MED,
								SH_ENERGY|SH_EXPLOSIVE, ITEM_CLIP_CELL, 0, DAM_HI, RANGE_FAR,   ACC_MED, POW_HI },
		{ "BEAM",	"gun0",		DECO_RIFLE,		"Blade Beam",
								SH_ENERGY,		ITEM_CLIP_CELL,		0, DAM_MED, RANGE_FAR,		ACC_HI, POW_MED,
								0  },
		{ "NULL",	"gun0",		DECO_RIFLE,		"Null Point Blaster",
								SH_ENERGY|SH_EXPLOSIVE,	ITEM_CLIP_CELL,		0, DAM_HI, RANGE_FAR,	ACC_HI, POW_HI,
								0  },
		{ "SWORD",	"gun0",		DECO_KNIFE,		"Plasma Field Sword",
								SH_ENERGY,		0,					0, DAM_HI, RANGE_MELEE,		ACC_HI, 0,
								0  },
		{ 0 }
	};

	for( int i=0; weapons[i].name; ++i ) {
		WeaponItemDef* item = new WeaponItemDef();
		item->InitBase( ITEM_WEAPON, 
						weapons[i].name, 
						weapons[i].desc, 
						weapons[i].deco,
						ModelResourceManager::Instance()->GetModelResource( weapons[i].resName ),
						nItemDef );

		item->weapon[0].shell		= weapons[i].shell0;
		item->weapon[0].clipType	= weapons[i].clip0;
		item->weapon[0].autoRounds	= weapons[i].auto0;
		item->weapon[0].damageBase	= weapons[i].damage0;
		item->weapon[0].range		= weapons[i].range0;
		item->weapon[0].accuracy	= weapons[i].acc0;
		item->weapon[0].power		= weapons[i].power0;

		item->weapon[1].shell		= weapons[i].shell1;
		item->weapon[1].clipType	= weapons[i].clip1;
		item->weapon[1].autoRounds	= weapons[i].auto1;
		item->weapon[1].damageBase	= weapons[i].damage1;
		item->weapon[1].range		= weapons[i].range1;
		item->weapon[1].accuracy	= weapons[i].acc1;
		item->weapon[1].power		= weapons[i].power1;

		itemDefArr[nItemDef++] = item;
	}

	struct ItemInit {
		const char* name;
		const char* resName;
		int type;
		int deco;
		const char* desc;
	};

	const ItemInit items[] = {		
		{ "Med",	0,				ITEM_GENERAL,	DECO_MEDKIT,	"Medkit" },
		{ "Steel",	0,				ITEM_GENERAL,	DECO_METAL,		"Memsteel" },
		{ "Tech",	0,				ITEM_GENERAL,	DECO_TECH,		"Alien Tech" },
		{ "Gel",	0,				ITEM_GENERAL,	DECO_FUEL,		"Plasma Gel" },
		{ "ARM-1",	0,				ITEM_ARMOR,		DECO_ARMOR,		"Memsteel Armor" },
		{ "ARM-2",	0,				ITEM_ARMOR,		DECO_ARMOR,		"Power Armor" },
		{ "ARM-3",	0,				ITEM_ARMOR,		DECO_ARMOR,		"Power Shield" },
		{ "Aln-0",	0,				ITEM_GENERAL,	DECO_ALIEN,		"Alien 0" },
		{ "Aln-1",	0,				ITEM_GENERAL,	DECO_ALIEN,		"Alien 1" },
		{ "Aln-2",	0,				ITEM_GENERAL,	DECO_ALIEN,		"Alien 2" },
		{ "Aln-3",	0,				ITEM_GENERAL,	DECO_ALIEN,		"Alien 3" },
		{ 0 }
	};

	for( int i=0; items[i].name; ++i ) {
		ItemDef* item = new ItemDef();
		item->InitBase( items[i].type,
						items[i].name,
						items[i].desc,
						items[i].deco,
						items[i].resName ? ModelResourceManager::Instance()->GetModelResource( items[i].resName ) : 0,
						nItemDef );
		itemDefArr[nItemDef++] = item;
	}

	struct ClipInit {
		const char* name;
		int type;
		int deco;
		int rounds;
		int shell;
		const char* desc;
	};

	const ClipInit clips[] = {
		{ "Clip",	ITEM_CLIP_SHELL,	DECO_SHELLS,	8,	 SH_KINETIC,		"9mm 8 round clip" },
		{ "AClip",	ITEM_CLIP_AUTO,		DECO_SHELLS,	15,	 SH_KINETIC,		"4mm 15 round auto-clip" },
		{ "Cell",	ITEM_CLIP_CELL,		DECO_CELL,		100, SH_ENERGY,			"10MW Cell" },
		{ "MinR",	ITEM_CLIP_ROCKET,	DECO_ROCKET,	1,	 SH_EXPLOSIVE,		"Mini Rocket" },
		{ "MC-AC",	ITEM_CLIP_CANON,	DECO_SHELLS,	1,	 SH_KINETIC,		"Mini Canon Armor Piercing Round" },
		{ "MC-I",	ITEM_CLIP_CANON,	DECO_SHELLS,	1,	 SH_INCINDIARY,		"Mini Canon Incendiary Round" },
		{ "RPG",	ITEM_CLIP_GRENADE,	DECO_ROCKET,	4,	 SH_EXPLOSIVE,		"Propelled Grenade Rounds" },
		{ 0 }
	};

	for( int i=0; clips[i].name; ++i ) {
		ClipItemDef* item = new ClipItemDef();
		item->InitBase( clips[i].type,
						clips[i].name,
						clips[i].desc,
						clips[i].deco,
						0,
						nItemDef );

		item->shell = clips[i].shell;
		item->rounds = clips[i].rounds;

		itemDefArr[nItemDef++] = item;
	}
	GLASSERT( nItemDef < EL_MAX_ITEM_DEFS );
}


const ItemDef* Game::GetItemDef( const char* name )
{
	for( unsigned i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( itemDefArr[i] && strcmp( itemDefArr[i]->name, name ) == 0 ) {
			return itemDefArr[i];
		}
	}
	GLASSERT( 0 );
	return 0;
}
