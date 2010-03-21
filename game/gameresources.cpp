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

#include "game.h"
#include "cgame.h"
#include "unit.h"
#include "material.h"
#include "../shared/gldatabase.h"
#include "../grinliz/glstringutil.h"
#include "../engine/text.h"

using namespace grinliz;


void Game::DumpWeaponInfo( FILE* fp, float range, const Stats& stats, int count )
{
#ifdef DEBUG
	fprintf( fp, "\nRange=%.1f DEX=%d Rank=%d Acc=%.2f\n", range, stats.DEX(), stats.Rank(), stats.Accuracy() );
	fprintf( fp, "name      PRIMARY-SNAP             PRIMARY-AIM/AUTO         SECONDARY\n" );
	fprintf( fp, "          " );
	for( int k=0; k<3; ++k )
		fprintf( fp, "dam n  hit   any  dptu   " );
	fprintf( fp, "\n" );
	fprintf( fp, "          " );
	for( int k=0; k<3; ++k )
		fprintf( fp, "--- - ---- ------ -----  " );
	fprintf( fp, "\n" );

	for( int i=1; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( itemDefArr[i] == 0 )
			continue;

		const WeaponItemDef* wid = itemDefArr[i]->IsWeapon();
		if ( wid ) {

			fprintf( fp, "%-10s", wid->name );

			for( int j=0; j<3; ++j ) {
				float fraction, fraction2, damage, dptu;
				int select, type;

				wid->FireModeToType( j, &select, &type );

				if ( wid->SupportsType( select, type ) ) {
					DamageDesc dd;
					wid->DamageBase( select, &dd );
					wid->FireStatistics( select, type, stats.Accuracy(), range, &fraction, &fraction2, &damage, &dptu );
					int nShots = (type==AUTO_SHOT) ? 3 : 1;

					fprintf( fp, "%3d %d %3d%% [%3d%%] %5.1f  ",
								 (int)dd.Total(),
								 nShots,
								 (int)(fraction*100.0f),
								 (int)(fraction2*100.0f),
								 dptu );
				}
			}
			fprintf( fp, "\n" );
			--count;
			if ( count == 0 )
				return;
		}
	}
#endif
}


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
		StrNCpy( name, (const char*)sqlite3_column_text( stmt, 0 ), 64 );	// name
		StrNCpy( format, (const char*)sqlite3_column_text( stmt, 1 ), 16 );	// format
		//int isImage = sqlite3_column_int( stmt, 2 );					// don't need
		int w = sqlite3_column_int( stmt, 3 );
		int h = sqlite3_column_int( stmt, 4 );
		int id = sqlite3_column_int( stmt, 5 );
		int metricsID = sqlite3_column_int( stmt, 6 );

		surface.Set( Surface::QueryFormat( format ), w, h );
		
		int blobSize = 0;
		reader.ReadSize( id, &blobSize );
		GLASSERT( surface.BytesInImage() == blobSize );
		reader.ReadData( id, blobSize, surface.Pixels() );

		textureID = surface.CreateTexture();
		texman->AddTexture( name, textureID, surface.Alpha() );
		GLOUTPUT(( "Texture %s\n", name ));
		
		if ( metricsID ) {
			int metricsSize = 0;
			reader.ReadSize( metricsID, &metricsSize );
			GLASSERT( metricsSize == UFOText::GLYPH_CX*UFOText::GLYPH_CY*sizeof(GlyphMetric) );
			reader.ReadData( metricsID, metricsSize, UFOText::MetricsPtr() );
		}
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
		StrNCpy( name, (const char*)sqlite3_column_text( stmt, 0 ), 64 );		// name
		LoadModel( name );
	}

	sqlite3_finalize(stmt);

}


void Game::LoadImages()
{
	char name[64];
	char format[16];
	BinaryDBReader reader( database );

	// Run through the database, and load all the images.
	sqlite3_stmt* stmt = 0;
	sqlite3_prepare_v2(database, "SELECT * FROM texture WHERE image=1;", -1, &stmt, 0 );

	while (sqlite3_step(stmt) == SQLITE_ROW) 
	{
		StrNCpy( name, (const char*)sqlite3_column_text( stmt, 0 ), 64 );	// name
		StrNCpy( format, (const char*)sqlite3_column_text( stmt, 1 ), 16 );	// format
		//int isImage = sqlite3_column_int( stmt, 2 );					// don't need
		int w = sqlite3_column_int( stmt, 3 );
		int h = sqlite3_column_int( stmt, 4 );
		int id = sqlite3_column_int( stmt, 5 );
		int metricsID = sqlite3_column_int( stmt, 6 );

		Surface* s = ImageManager::Instance()->AddLockedSurface();
		s->Set( Surface::QueryFormat( format ), w, h );
		
		int blobSize = 0;
		reader.ReadSize( id, &blobSize );
		GLASSERT( s->BytesInImage() == blobSize );
		reader.ReadData( id, blobSize, s->Pixels() );
		s->SetName( name );
		
		ImageManager::Instance()->Unlock();

		GLOUTPUT(( "LightMap %s\n", s->Name() ));
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

		StrNCpy( itemDef->name, init->name, EL_FILE_STRING_LEN );

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
		itemDef->flammable = (U8)(init->flammable*255.0);
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

		StrNCpy( itemDef->name, init->Name(), EL_FILE_STRING_LEN );
		
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
	const int HP_MED = 80;
	const int HP_HIGH = 250;
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
	const float BURN = 0.5f;
	const float FASTBURN = 1.0f;

	enum {
		LANDER_LIGHT = LIGHT_SET
	};
	
	const MapLightInit lights[] = 
	{
		//	name			object   x  y   cx  cy	upperLeft 
		{	"landerLight",	-1,	0,	 1,	0,	8,	10,	true },
		{	0	}
	};
	InitMapLight( LIGHT_SET, lights );


	const MapItemInit farmSet[] =
	{
			// model		open			destroyed	cx, cz	hp		material		pather visibility
		{	"farmBed",		0,				0,			1,	1,	HP_MED,	BURN,			"f"	 "0"	},
		{	"farmTable",	0,				0,			1,	1,	HP_MED,	BURN,			"f", "0" 	},
		{	"farmTable2x1",	0,				0,			2,	1,	HP_MED,	BURN,			"ff","00"	},
		{	"farmWheat",	0,				0,			1,	1,	HP_LOW,	FASTBURN		,"0", "0" },
		{	0	}
	};
	InitMapItemDef( FARM_SET, farmSet );
  
	const MapItemInit marineSet[] =
	{
			// model		open			destroyed	cx, cz	hp			material	pather
		{	"lander",		0,				0,			6,	6,	INDESTRUCT,	0,			"00ff00 00ff00 ff00ff ff00ff ff00ff ff00ff",
																						"00ff00 00ff00 0f00f0 0f00f0 0f00f0 0f00f0", 
																						LANDER_LIGHT },
		{	"guard",		0,				0,			1,  1,  INDESTRUCT, 0,			"0" },
		{	"scout",		0,				0,			1,  1,  INDESTRUCT, 0,			"0" },
		{	0	}
	};
	InitMapItemDef( MARINE_SET, marineSet );

	const MapItemInit forestSet[] = 
	{
			// model		open			destroyed	cx, cz	hp		 material		pather
		{	"tree",			0,				0,			1,	1,	HP_HIGH, BURN,			"f", "0" },
		{	0	}
	};
	InitMapItemDef( FOREST_SET, forestSet );

	const MapItemInit ufoSet[] = 
	{
			// model		open			destroyed	cx, cz	hp			material	pather
		{	"ufo_Diag",		0,				0,			1,	1,	HP_STEEL,	0,			"f" },
		{	"ufo_DoorCld",	"ufo_DoorOpn",	0,			1,	1,	HP_STEEL,	0,			"0", "1" },
		{	"ufo_WallInn",	0,				0,			1,	1,	HP_STEEL,	0,			"1" },
		{	"ufo_CrnrInn",	0,				0,			1,	1,	HP_STEEL,	0,			"3" },
		{	"ufo_WallOut",	0,				0,			1,	1,	HP_STEEL,	0,			"1" },
		{	"ufo_Join0",	0,				0,			2,	1,	HP_STEEL*2,	0,			"1f" },
		{	"ufo_Join1",	0,				0,			1,	2,	HP_STEEL,	0,			"2f" },
		{	0	}
	};
	InitMapItemDef( UFO_SET0, ufoSet );

	const MapItemInit woodSet[] =
	{
			// model		open			destroyed	cx, cz	hp		material		pather
		{	"woodCrnr",		0,				"woodCrnrD",1,	1,	HP_MED,	BURN,			"3" },
		{	"woodDoorCld",	"woodDoorOpn",	0,			1,	1,	HP_MED,	BURN,			"0", "1" },
		{	"woodWall",		0,				0,			1,	1,	HP_MED,	BURN,			"1" },
		{	"woodWallWin",	0,				0,			1,	1,	HP_MED,	BURN,			"1", "0" },
		{	0	}
	};
	InitMapItemDef(  WOOD_SET, woodSet );
}


void Game::LoadItemResources()
{
	const float DAM_LOW		=  20.0f;
	const float DAM_MEDLOW	=  30.0f;
	const float DAM_MED		=  40.0f;
	const float DAM_MEDHI	=  50.0f;
	const float DAM_HI		=  60.0f;
	const float DAM_VHI		=  80.0f;

	const float EXDAM_MED   = 100.0f;
	const float EXDAM_HI	= 180.0f;
	const float EXDAM_VHI	= 250.0f;

	const float ACC_LOW		= 1.4f;
	const float ACC_MEDLOW	= 1.2f;
	const float ACC_MED		= 1.0f;
	const float ACC_MEDHI	= 0.8f;
	const float ACC_HI		= 0.7f;
	const float ACC_VHI		= 0.6f;

	const float SPEED_FAST = 0.8f;
	const float SPEED_NORMAL = 1.0f;
	const float SPEED_SLOW	= 1.3f;
	const int POW_LOW = 10;
	const int POW_MED = 20;
	const int POW_HI  = 50;

	memset( itemDefArr, 0, sizeof( ItemDef* )*EL_MAX_ITEM_DEFS );
	int nItemDef = 1;	// keep the first entry null


	struct ClipInit {
		const char* name;
		//int type;
		bool alien;
		int deco;
		int rounds;
		DamageDesc dd;

		Color4F color;
		float speed;
		float width;
		float length;
		
		const char* desc;
	};

	const float SPEED = 0.02f;	// ray gun settings
	const float WIDTH = 0.3f;
	const float BOLT  = 2.0f;

	const ClipInit clips[] = {
		{ "Clip",	false,	DECO_SHELLS,	40,	 { 1, 0, 0 },
					{ 0.8f, 1.0f, 0.8f, 0.8f }, SPEED*2.0f, WIDTH*0.5f, BOLT*3.0f, "4x10 round clip" },
		{ "AClip",	false,	DECO_SHELLS,	15,	 { 1, 0, 0 },
					{ 0.8f, 1.0f, 0.8f, 0.8f }, SPEED*2.0f, WIDTH*0.5f, BOLT*3.0f, "4mm 15 round auto-clip" },
		{ "Zing",	true,	DECO_CELL,		40,  { 0.0f, 0.8f, 0.2f },
					{ 0.2f, 1.0f, 0.2f, 0.8f }, SPEED,		 WIDTH,		 BOLT,		"Zing Rounds" },
		{ "Cell",	true,	DECO_CELL,		12,  { 0.0f, 0.8f, 0.2f },
					{ 0.2f, 1.0f, 0.2f, 0.8f }, SPEED,		 WIDTH,		 BOLT,		"10MW Cell" },
		{ "Tachy",	true,	DECO_CELL,		4,   { 0, 0.6f, 0.4f },
					{ 1, 1, 0, 0.8f },			 SPEED,		 WIDTH,		 BOLT,		"Tachyon field rounds" },
		{ "Flame",	false,	DECO_SHELLS,	2,	 { 0, 0, 1 },
					{ 1.0f, 0, 0, 0.8f },		 SPEED*0.5f, WIDTH,		 BOLT,		"Incendiary Heavy Round" },
		{ "RPG",	false,	DECO_ROCKET,	4,	 { 0.8f, 0, 0.2f },
					{ 0.8f, 1.0f, 0.8f, 0.8f }, SPEED*0.8f, WIDTH,		 BOLT,		"Grenade Rounds" },
		{ 0 }
	};

	for( int i=0; clips[i].name; ++i ) {
		ClipItemDef* item = new ClipItemDef();
		item->InitBase( clips[i].name,
						clips[i].desc,
						clips[i].deco,
						0,
						nItemDef );

		item->defaultRounds = clips[i].rounds;
		item->dd = clips[i].dd;

		item->alien = clips[i].alien;
		item->color = clips[i].color;
		item->speed = clips[i].speed;
		item->width = clips[i].width;
		item->length = clips[i].length;

		itemDefArr[nItemDef++] = item;
	}
	GLASSERT( nItemDef < EL_MAX_ITEM_DEFS );


	struct WeaponInit {
		const char* name;
		const char* resName;
		int deco;
		const char* desc;
		const char* modes[3];
		float	speed;		// speed/weight modifier - 1.0 is normal, 2.0 is twice as slow

		const char* clip0;		// 
		int		flags0;
		float	damage0;	// damage modifier of clip - modifies shell
		float	acc0;		// accuracy modifier - independent of shell

		const char*	clip1;
		int		flags1;
		float	damage1;
		float	acc1;
	};

	const char* FAST = "Fast";
	const char* AIMED = "Aimed";
	const char* BURST = "Burst";
	const char* BOOM = "Boom";
	const char* BANG = "Bang";
	const char* STRIKE = "Strike";

	const WeaponInit weapons[] = {		
		// Terran	resName		deco			description
		//				
		{ "PST",	"pst1",		DECO_PISTOL,	"Pistol",				FAST, AIMED, 0,		SPEED_FAST,
				"Clip",			0,										DAM_LOW,	ACC_MED,
				0  },
		{ "AR-1",	"ar1",		DECO_RIFLE,		"Klk Assault Rifle",	FAST, BURST, 0,		SPEED_NORMAL,
				"AClip",		WEAPON_AUTO,							DAM_LOW,	ACC_MED,
				0  },
		{ "AR-2",	"ar2",		DECO_RIFLE,		"N7 Assault Rifle",		FAST, BURST, 0,		SPEED_NORMAL,
				"AClip",		WEAPON_AUTO,							DAM_MEDLOW,	ACC_MED,
				0  },
		{ "AR-3P",	"ar3p",		DECO_RIFLE,		"Pulse Rifle",			FAST, BURST, BOOM,	SPEED_NORMAL,
				"AClip",		WEAPON_AUTO,							DAM_MEDHI,	ACC_MED,
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_HI,	ACC_LOW },
		{ "AR-3L",	"ar3l",		DECO_RIFLE,		"Long AR 'Vera'",		FAST, AIMED, BOOM,	SPEED_NORMAL,
				"AClip",		0,										DAM_HI,		ACC_HI,
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_HI,	ACC_LOW },
		{ "CAN-I",	"cani",		DECO_RIFLE,		"Mini-Canon/Flame",		FAST, BANG, BOOM,	SPEED_SLOW,
				"AClip",		WEAPON_EXPLOSIVE,						DAM_HI,		ACC_MED,
				"Flame",		WEAPON_EXPLOSIVE,						EXDAM_MED,	ACC_MED },
		{ "CAN-X",	"canx",		DECO_RIFLE,		"Mini-Canon/Rocket",	FAST, BANG, BOOM,	SPEED_SLOW,
				"AClip",		WEAPON_EXPLOSIVE,						DAM_HI,		ACC_MED,
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_MED,	ACC_MED },


		// Alien
		{ "RAY-1",	"ray1",		DECO_PISTOL,	"Ray Gun",				FAST, AIMED, 0,		SPEED_FAST,
				"Zing",			0,										DAM_MEDLOW,	ACC_MED,
				0  },
		{ "RAY-2",	"ray2",		DECO_PISTOL,	"Advanced Ray Gun",		FAST, AIMED, 0,		SPEED_FAST,
				"Cell",			0,										DAM_MED,	ACC_MED,
				0  },
		{ "RAY-3",	"ray3",		DECO_PISTOL,	"Disinigrator",			FAST, AIMED, 0,		SPEED_FAST,
				"Cell",			0,										DAM_HI,		ACC_MED,
				0  },
		{ "PLS-1",	"pls1",		DECO_RIFLE,		"Plasma Rifle",			FAST, BURST, 0,		SPEED_NORMAL,
				"Cell",			WEAPON_AUTO,							DAM_HI,	ACC_MED,
				0  },
		{ "PLS-2",	"pls2",		DECO_RIFLE,		"Plasma Assault Rifle",	FAST, BURST, BOOM,	SPEED_NORMAL,
				"Cell",			WEAPON_AUTO,							DAM_VHI,	ACC_MED,
				"Tachy",		WEAPON_EXPLOSIVE,						EXDAM_HI,	ACC_MED },
		{ "BEAM",	"beam",		DECO_RIFLE,		"Blade Beam",			FAST, AIMED, 0,		SPEED_FAST,
				"Cell",			0,										DAM_MEDHI,	ACC_HI,
				0  },
		{ "NULL",	"nullp",	DECO_RIFLE,		"Null Point Blaster",	FAST, BANG, 0,		SPEED_NORMAL,
				"Tachy",		WEAPON_EXPLOSIVE,						DAM_VHI, ACC_HI,
				0  },
		{ 0 }
	};

	for( int i=0; weapons[i].name; ++i ) {
		WeaponItemDef* item = new WeaponItemDef();

		GLASSERT( !weapons[i].clip0 || ( weapons[i].clip0 != weapons[i].clip1 ) );	// code later get confused.

		item->InitBase( weapons[i].name, 
						weapons[i].desc, 
						weapons[i].deco,
						ModelResourceManager::Instance()->GetModelResource( weapons[i].resName ),
						nItemDef );

		for( int j=0; j<3; ++j ) {
			item->fireDesc[j]		= weapons[i].modes[j];
		}
		item->speed					= weapons[i].speed;

		item->weapon[0].clipItemDef = 0;
		if ( weapons[i].clip0 ) {
			item->weapon[0].clipItemDef = GetItemDef( weapons[i].clip0 )->IsClip();
			GLASSERT( item->weapon[0].clipItemDef );
		}
		item->weapon[0].flags		= weapons[i].flags0;
		item->weapon[0].damage		= weapons[i].damage0;
		item->weapon[0].accuracy	= weapons[i].acc0;

		item->weapon[1].clipItemDef = 0;
		if ( weapons[i].clip1 ) {
			item->weapon[1].clipItemDef = GetItemDef( weapons[i].clip1 )->IsClip();
			GLASSERT( item->weapon[1].clipItemDef );
		}
		item->weapon[1].flags		= weapons[i].flags1;
		item->weapon[1].damage		= weapons[i].damage1;
		item->weapon[1].accuracy	= weapons[i].acc1;

		itemDefArr[nItemDef++] = item;
	}

	struct ItemInit {
		const char* name;
		const char* resName;
		int deco;
		const char* desc;
	};

	const ItemInit items[] = {		
		{ "Med",	0,				DECO_MEDKIT,	"Medkit" },
		{ "Steel",	0,				DECO_METAL,		"Memsteel" },
		{ "Tech",	0,				DECO_TECH,		"Alien Tech" },
		{ "Gel",	0,				DECO_FUEL,		"Plasma Gel" },
		{ "Aln-0",	0,				DECO_ALIEN,		"Alien 0" },
		{ "Aln-1",	0,				DECO_ALIEN,		"Alien 1" },
		{ "Aln-2",	0,				DECO_ALIEN,		"Alien 2" },
		{ "Aln-3",	0,				DECO_ALIEN,		"Alien 3" },
		{ 0 }
	};

	for( int i=0; items[i].name; ++i ) {
		ItemDef* item = new ItemDef();
		item->InitBase( items[i].name,
						items[i].desc,
						items[i].deco,
						items[i].resName ? ModelResourceManager::Instance()->GetModelResource( items[i].resName ) : 0,
						nItemDef );
		itemDefArr[nItemDef++] = item;
	}

	const ItemInit armor[] = {		
		{ "ARM-1",	0,				DECO_ARMOR,		"Memsteel Armor" },
		{ "ARM-2",	0,				DECO_ARMOR,		"Power Armor" },
		{ "ARM-3",	0,				DECO_ARMOR,		"Power Shield" },
		{ 0 }
	};

	for( int i=0; armor[i].name; ++i ) {
		ArmorItemDef* item = new ArmorItemDef();
		item->InitBase( armor[i].name,
						armor[i].desc,
						armor[i].deco,
						armor[i].resName ? ModelResourceManager::Instance()->GetModelResource( items[i].resName ) : 0,
						nItemDef );
		item->amount = ARMOR_AMOUNT*i;
		itemDefArr[nItemDef++] = item;
	}

#if defined( DEBUG ) && defined( _MSC_VER )
	// Dump out all the weapon statistics.
	{
#pragma warning ( push )
#pragma warning ( disable : 4996 )	// fopen is unsafe. For video games that seems extreme.
		FILE* fp = fopen( "weapons.txt", "w" );
#pragma warning (pop)

		Stats stats;
		stats.SetSTR( (TRAIT_TERRAN_HIGH + TRAIT_TERRAN_LOW)/2 );
		stats.SetDEX( (TRAIT_TERRAN_HIGH + TRAIT_TERRAN_LOW)/2 );
		stats.SetPSY( (TRAIT_TERRAN_HIGH + TRAIT_TERRAN_LOW)/2 );
		stats.SetRank( 2 );

		const float range[] = { 6.0f, 3.0f, 12.0f };

		for( int r=0; r<3; ++r ) {
			DumpWeaponInfo( fp, range[r], stats, 0 );
		}
		stats.SetRank( 0 );
		DumpWeaponInfo( fp, 6.0f, stats, 5 );

		stats.SetRank( 4 );
		DumpWeaponInfo( fp, 6.0f, stats, 5 );

		fprintf( fp, "\nRandom 32 bit ints:\n" );
		Random random;
		for( int i=0; i<10; ++i ) {
			fprintf( fp, "%d\t%d\t%d\t%d\n", random.Rand(), random.Rand(), random.Rand(), random.Rand() );
		}
		fprintf( fp, "\nRandom human traits:\n" );
		for( int i=0; i<10; ++i ) {
			fprintf( fp, "%d\t%d\t%d\t%d\n", Stats::GenStat( &random, TRAIT_TERRAN_LOW, TRAIT_TERRAN_HIGH ),
											 Stats::GenStat( &random, TRAIT_TERRAN_LOW, TRAIT_TERRAN_HIGH ),
											 Stats::GenStat( &random, TRAIT_TERRAN_LOW, TRAIT_TERRAN_HIGH ),
											 Stats::GenStat( &random, TRAIT_TERRAN_LOW, TRAIT_TERRAN_HIGH ) );
		}

		fclose( fp );
	}
#endif
}


const ItemDef* Game::GetItemDef( const char* name )
{
	GLASSERT( name && *name );
	for( unsigned i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( itemDefArr[i] && strcmp( itemDefArr[i]->name, name ) == 0 ) {
			return itemDefArr[i];
		}
	}
	GLASSERT( 0 );
	return 0;
}
