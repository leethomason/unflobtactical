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
#include "../grinliz/glstringutil.h"
#include "../engine/text.h"

using namespace grinliz;


void Game::DumpWeaponInfo( FILE* fp, float range, const Stats& stats, int count )
{
#ifdef DEBUG
	fprintf( fp, "\nRange=%.1f DEX=%d Rank=%d Acc=%.3f\n", range, stats.DEX(), stats.Rank(), stats.AccuracyArea() );
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
				WeaponMode mode = (WeaponMode)j;
				if ( wid->HasWeapon( mode )) {
					DamageDesc dd;
					wid->DamageBase( mode, &dd );
					wid->FireStatistics( mode, stats.AccuracyArea(), range, &fraction, &fraction2, &damage, &dptu );
					int nShots = wid->RoundsNeeded( mode );

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


void Game::CreateTexture( Texture* t )
{
	if ( StrEqual( t->Name(), "white" ) ) {
		U16 pixels[4] = { 0xffff, 0xffff, 0xffff, 0xffff };
		GLASSERT( t->Width() == 2 && t->Height() == 2 && t->Format() == Surface::RGB16 );
		GLASSERT( t->BytesInImage() == 8 );
		t->Upload( pixels, 8 );
	}
	else {
		GLASSERT( 0 );
	}
}


void Game::LoadTextures()
{
	TextureManager* texman = TextureManager::Instance();
	texman->CreateTexture( "white", 2, 2, Surface::RGB16, Texture::PARAM_NONE, this );

	const gamedb::Item* node = database->Root()->Child( "textures" )->Child( "stdfont2" );
	GLASSERT( node );
	int metricsSize = node->GetDataSize( "metrics" );
	GLASSERT( metricsSize == UFOText::GLYPH_CX*UFOText::GLYPH_CY*sizeof(GlyphMetric) );
	node->GetData( "metrics", UFOText::MetricsPtr(), metricsSize );
}



void Game::LoadModel( const char* name )
{
	GLASSERT( modelLoader );

	const gamedb::Item* item = database->Root()->Child( "models" )->Child( name );
	GLASSERT( item );

	ModelResource* res = new ModelResource();
	modelLoader->Load( item, res );
	ModelResourceManager::Instance()->AddModelResource( res );
}


void Game::LoadModels()
{
	// Run through the database, and load all the models.
	const gamedb::Item* parent = database->Root()->Child( "models" );
	GLASSERT( parent );

	for( int i=0; i<parent->NumChildren(); ++i )
	{
		const gamedb::Item* node = parent->Child( i );
		LoadModel( node->Name() );
	}
}


void Game::InitMapLight( int index, const MapLightInit* init )
{
	while( init->name ) 
	{
		Map::MapItemDef* itemDef = engine->GetMap()->InitItemDef( index );
		itemDef->Init();
	
		GLASSERT( init->x || init->y );

		itemDef->name = init->name;
		itemDef->lightOffsetX = init->objectX;
		itemDef->lightOffsetY = init->objectY;
		itemDef->lightTX = init->x;
		itemDef->lightTY = init->y;
		itemDef->cx = init->cx;
		itemDef->cy = init->cy;

		++init;
		++index;
	}
}


void Game::InitMapItemDef( int index, const MapItemInit* init )
{
	while( init->model ) 
	{
		Map::MapItemDef* itemDef = engine->GetMap()->InitItemDef( index );
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

		itemDef->name = init->Name();
		
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

	const int HP_LOW		= 10;
	const int HP_MEDLOW		= 40;
	const int HP_MED		= 80;
	const int HP_HIGH		= 200;
	const int HP_STEEL		= 400;
	const int INDESTRUCT	= 0xffff;

	const int FOREST_SET = 0x01;
	const int FARM_SET	 = 0x10;
	const int WOOD_SET	 = 0x20;
	const int TOWN_SET   = 0x30;
	const int UFO_SET0	 = 0x40;
	const int UFO_SET1	 = 0x50;
	const int MARINE_SET = 0x60;

	const int LIGHT_SET  = Map::LIGHT_START;
	const float SLOWBURN = 0.2f;
	const float BURN = 0.5f;
	const float FASTBURN = 1.0f;

	enum {
		LANDER_LIGHT = LIGHT_SET
	};
	
	static const MapLightInit lights[] = 
	{
		//	name			object		 x  y   cx  cy	 
		{	"landerLight",	-1,	0,		1,	0,	 8,	10 },
		{	"fireLight",	-2, -2,		10, 0,	 5,  5 },
		{	0	}
	};
	InitMapLight( LIGHT_SET, lights );


	static const MapItemInit farmSet[] =
	{
			// model		open			destroyed	cx, cz	hp		material		pather visibility
		{	"farmBed",		0,				0,			2,	2,	HP_MED,	BURN,			"f"	 "0"	},
		{	"farmTable",	0,				0,			1,	1,	HP_MED,	BURN,			"f", "0" 	},
		{	"farmTable2x1",	0,				0,			2,	1,	HP_MED,	BURN,			"ff","00"	},
		{	"stonewall_unit",0,	"stonewall_unitD",		1,	1,	HP_MED,	0,				"f", "0" },
		{	"stonewall_join",0,	"stonewall_unitD",		1,	1,	HP_MED,	0,				"f", "0" },
		{	"woodfence",	0,				0,			2,	1,	HP_LOW,	FASTBURN,		"44", "0" },
		{	"oldwell",		0,				0,			1,	1,	HP_MED,	SLOWBURN,		"f", "0" },
		{	"haypile",		0,				0,			2,	2,	HP_MED,	FASTBURN,		"ffff", "ffff" },
		{	"whitepicketfence",	0,			0,			1,	1,	HP_LOW,	FASTBURN,		"1", "0" },
		{	0	}
	};
	InitMapItemDef( FARM_SET, farmSet );
  
	static const MapItemInit marineSet[] =
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

	static const MapItemInit forestSet[] = 
	{
			// model		open			destroyed	cx, cz	hp			material		pather
		{	"tree",			0,				0,			1,	1,	HP_MEDLOW,	BURN,			"f", "0" },
		{	"tree2",		0,				0,			1,	1,	HP_MEDLOW,	BURN,			"f", "0" },
		{	"tree3",		0,				0,			1,	1,	HP_MEDLOW,	BURN,			"f", "0" },
		{	0	}
	};
	InitMapItemDef( FOREST_SET, forestSet );

	static const MapItemInit ufoSet[] = 
	{
			// model		open			destroyed	cx, cz	hp			material	pather
		{	"ufo_WallOut",	0,				0,			1,	1,	HP_STEEL,	0,			"1" },
		{	"ufo_WallCurve4", 0,			0,			4,	4,	INDESTRUCT,	0,			"0002 0003 0030 1300",		// pather
																						"0002 0003 0030 1300" },	// visibility
		{	"ufo_DoorCld",	"ufo_DoorOpn",	0,			1,	1,	HP_STEEL,	0,			"0", "1" },
		{	"ufo_WallInn",	0,				0,			1,	1,	HP_STEEL,	0,			"1" },
		{	"ufo_CrnrInn",	0,				0,			1,	1,	HP_STEEL,	0,			"3" },
		{	0	}
	};
	InitMapItemDef( UFO_SET0, ufoSet );

	static const MapItemInit woodSet[] =
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
	const float DAM_LOW		=   20.0f;
	const float DAM_MEDLOW	=   30.0f;
	const float DAM_MED		=   40.0f;
	const float DAM_MEDHI	=   60.0f;
	const float DAM_HI		=   80.0f;
	const float DAM_HIPLUS	=   90.0f;
	const float DAM_VHI		=  100.0f;

	const float EXDAM_LOW   =  50.0f;
	const float EXDAM_MED   = 100.0f;
	const float EXDAM_HI	= 150.0f;
	const float EXDAM_VHI	= 200.0f;

	// lower is better, and in terms of AREA
	const float ACC_VLOW	= 2.50f;
	const float ACC_LOW		= 2.00f;
	const float ACC_MEDLOW	= 1.50f;
	const float ACC_MED		= 1.00f;
	const float ACC_MEDHI	= 0.80f;
	const float ACC_HI		= 0.50f;
	const float ACC_VHI		= 0.30f;

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
		char		abbreviation;
		bool		alien;
		int			deco;
		int			rounds;
		DamageDesc	dd;

		// Rendering description.
		Color4F		color;
		float		speed;
		float		width;
		float		length;
		
		const char* desc;
	};

	const float SPEED = 0.02f;	// ray gun settings
	const float WIDTH = 0.3f;
	const float BOLT  = 2.0f;
#	define COLORDEF( r, g, b ) { (float)r/255.f, (float)g/255.f, (float)b/255.f, 0.8f }

	static const ClipInit clips[] = {
		{ "Clip",	'C',	false,	DECO_SHELLS,	15,	 { 1, 0, 0 },
					COLORDEF( 200, 204, 213 ), SPEED*2.0f,	WIDTH*0.5f, BOLT*3.0f, "7mm 15 round auto-clip" },
		{ "Cell",	'E',	true,	DECO_CELL,		12,  { 0.0f, 0.8f, 0.2f },
					COLORDEF( 199, 216, 6 ),	SPEED,		 WIDTH,		BOLT,		"10MW Cell" },
		{ "Tachy",	'T',	true,	DECO_CELL,		4,   { 0, 0.6f, 0.4f },
					COLORDEF( 227, 125, 220 ),	SPEED,		WIDTH,		BOLT,		"Tachyon field rounds" },
		{ "Flame",	'F',	false,	DECO_SHELLS,	3,	 { 0, 0, 1 },
					COLORDEF( 213, 63, 63 ),	SPEED*0.7f,	WIDTH,		BOLT,		"Incendiary Heavy Round" },
		{ "RPG",	'R',	false,	DECO_ROCKET,	4,	 { 0.8f, 0, 0.2f },
					COLORDEF( 200, 204, 213 ),  SPEED*0.8f, WIDTH,		BOLT,		"Grenade Rounds" },
		{ 0 }
	};

#	undef COLORDEF

	for( int i=0; clips[i].name; ++i ) {
		ClipItemDef* item = new ClipItemDef();
		item->InitBase( clips[i].name,
						clips[i].desc,
						clips[i].deco,
						0,
						nItemDef );

		item->defaultRounds = clips[i].rounds;
		item->dd = clips[i].dd;

		item->abbreviation = clips[i].abbreviation;
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
		const char* sound0;

		const char*	clip1;
		int		flags1;
		float	damage1;
		float	acc1;
		const char* sound1;
	};

	const char* FAST = "Fast";
	const char* AIMED = "Aimed";
	const char* BURST = "Burst";
	const char* FLAME = "Flame";
	const char* EXPLO = "Boom";

	static const WeaponInit weapons[] = {		
		// Terran	resName		deco			description
		//				
		{ "PST",	"pst1",		DECO_PISTOL,	"Pistol",				FAST, AIMED, FLAME,	SPEED_FAST,
				"Clip",			0,										DAM_LOW,	ACC_MED,	"ar",
				"Flame",		WEAPON_EXPLOSIVE,						EXDAM_LOW,	ACC_MED,	"can"  },
		{ "SLUG",	"slug",		DECO_PISTOL,	"Slug gun",				FAST, AIMED, 0,		SPEED_NORMAL,
				"Clip",			0,										DAM_HI,		ACC_VLOW,	"can",
				0  },
		{ "AR-1",	"ar1",		DECO_RIFLE,		"Kaliknov Rifle",		FAST, BURST, 0,		SPEED_NORMAL,
				"Clip",			WEAPON_AUTO,							DAM_MEDLOW,	ACC_MED,	"ar",
				0  },
		{ "AR-2",	"ar2",		DECO_RIFLE,		"Glek Rifle",			FAST, BURST, 0,		SPEED_NORMAL,
				"Clip",			WEAPON_AUTO,							DAM_MED,	ACC_MED,	"ar",
				0  },
		{ "AR-3P",	"ar3p",		DECO_RIFLE,		"Pulse Rifle",			FAST, BURST, EXPLO,	SPEED_NORMAL,
				"Clip",			WEAPON_AUTO,							DAM_MEDHI,	ACC_MED,	"ar3p",
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_HI,	ACC_LOW,	"can" },
		{ "AR-3L",	"ar3l",		DECO_RIFLE,		"Callahan LR",			FAST, AIMED, EXPLO,	SPEED_NORMAL,
				"Clip",			0,										DAM_HI,		ACC_VHI,	"ar3l",
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_HI,	ACC_LOW,	"can" },
		{ "CANON",	"canon",	DECO_RIFLE,		"Mini-Canon",			FAST, FLAME, EXPLO,	SPEED_SLOW,
				"Flame",		WEAPON_EXPLOSIVE,						EXDAM_MED,	ACC_MED,	"can",
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_MED,	ACC_MED,	"can", },

		// Alien
		{ "RAY-1",	"ray1",		DECO_PISTOL,	"Ray Gun",				FAST, AIMED, 0,		SPEED_FAST,
				"Cell",			0,										DAM_MEDLOW,	ACC_MED,	"ray1",
				0  },
		{ "RAY-2",	"ray2",		DECO_PISTOL,	"Advanced Ray Gun",		FAST, AIMED, 0,		SPEED_FAST,
				"Cell",			0,										DAM_MED,	ACC_MED,	"ray2",
				0  },
		{ "RAY-3",	"ray3",		DECO_PISTOL,	"Disinigrator",			FAST, AIMED, 0,		SPEED_FAST,
				"Cell",			0,										DAM_HI,		ACC_MED,	"ray3",
				0  },
		{ "PLS-1",	"pls1",		DECO_RIFLE,		"Plasma Rifle",			FAST, BURST, 0,		SPEED_NORMAL,
				"Cell",			WEAPON_AUTO,							DAM_HI,	ACC_MED,		"plasma",
				0  },
		{ "PLS-2",	"pls2",		DECO_RIFLE,		"Heavy Plasma",			FAST, BURST, EXPLO,	SPEED_NORMAL,
				"Cell",			WEAPON_AUTO,							DAM_HIPLUS,	ACC_MED,	"plasma",
				"Tachy",		WEAPON_EXPLOSIVE,						EXDAM_HI,	ACC_MED,	"nullp", },
		{ "BEAM",	"beam",		DECO_RIFLE,		"Blade Beam",			FAST, AIMED, 0,		SPEED_FAST,
				"Cell",			0,										DAM_MEDHI,	ACC_HI,		"beam",
				0  },
		{ "NULL",	"nullp",	DECO_RIFLE,		"Null Point Blaster",	FAST, EXPLO, 0,		SPEED_NORMAL,
				"Tachy",		WEAPON_EXPLOSIVE,						DAM_VHI, ACC_HI,		"nullp",
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
		item->weapon[0].sound		= weapons[i].sound0;

		item->weapon[1].clipItemDef = 0;
		if ( weapons[i].clip1 ) {
			item->weapon[1].clipItemDef = GetItemDef( weapons[i].clip1 )->IsClip();
			GLASSERT( item->weapon[1].clipItemDef );
		}
		item->weapon[1].flags		= weapons[i].flags1;
		item->weapon[1].damage		= weapons[i].damage1;
		item->weapon[1].accuracy	= weapons[i].acc1;
		item->weapon[1].sound		= weapons[i].sound1;

		itemDefArr[nItemDef++] = item;
	}

	struct ItemInit {
		const char* name;
		const char* resName;
		int deco;
		const char* desc;
	};

	static const ItemInit items[] = {		
		{ "Med",	0,				DECO_MEDKIT,	"Medkit" },
		{ "Steel",	0,				DECO_METAL,		"Memsteel" },
		{ "Tech",	0,				DECO_TECH,		"Alien Tech" },
		{ "Gel",	0,				DECO_FUEL,		"Plasma Gel" },
		{ "Aln-0",	0,				DECO_ALIEN,		"Alien 0" },
		{ "Aln-1",	0,				DECO_ALIEN,		"Alien 1" },
		{ "Aln-2",	0,				DECO_ALIEN,		"Alien 2" },
		{ "Aln-3",	0,				DECO_ALIEN,		"Alien 3" },
		{ "Shield",	0,				DECO_SHIELD,	"Energy Shield" },
		{ "Ablate",	0,				DECO_SHIELD,	"Alblative Shield" },
		{ "Fiber",	0,				DECO_SHIELD,	"Kinetic Fiber Weave" },
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

	static const ItemInit armor[] = {		
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
		stats.SetSTR( 40 );
		stats.SetDEX( 40 );
		stats.SetPSY( 40 );
		stats.SetRank( 2 );

		const float range[] = { 4.0f, 8.0f, 16.0f, 0 };
		const int rank[] = { 0, 2, 4, -1 };

//		const float range[] = { 8.0f, 0 };
//		const int rank[] = { 0, 4, -1 };

		for( int rankIt=0; rank[rankIt]>=0; ++rankIt ) {
			stats.SetRank( rank[rankIt] );

			for( int rangeIt=0; range[rangeIt]>0; ++rangeIt ) {
				DumpWeaponInfo( fp, range[rangeIt], stats, 0 );
			}
		}


		for( int i=0; i<5; ++i ) {
			stats.SetSTR( 25+i*10 );
			stats.SetDEX( 25+i*10 );
			stats.SetPSY( 25+i*10 );
			stats.SetRank( i );

			DumpWeaponInfo( fp, 8.0f, stats, 1 );
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


void Game::LoadAtoms()
{
	TextureManager* tm = TextureManager::Instance();

	renderAtoms[ATOM_TEXT].Init( (const void*)UIRenderer::RENDERSTATE_UI_TEXT, (const void*)tm->GetTexture( "stdfont2" ), 0, 0, 1, 1, 256, 128 );
	renderAtoms[ATOM_TEXT_D].Init( (const void*)UIRenderer::RENDERSTATE_UI_TEXT_DISABLED, (const void*)tm->GetTexture( "stdfont2" ), 0, 0, 1, 1, 256, 128 );

	renderAtoms[ATOM_TACTICAL_BACKGROUND].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, (const void*)tm->GetTexture( "intro" ), 0, 0, 1, 1, 64, 64 );
	renderAtoms[ATOM_TACTICAL_BACKGROUND_TEXT].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "introText" ), 0, 0, 1, 1, 256, 64 );

	for( int i=0; i <= (ATOM_RED_BUTTON_UP-ATOM_GREEN_BUTTON_UP); i += 4 ) {
		renderAtoms[ATOM_GREEN_BUTTON_UP+i].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "icons" ), 0, 0, 1, 1, 56, 56 );
		UIRenderer::SetAtomCoordFromPixel( 0+64*(i/4), 193, 62+64*(i/4), 253, 512, 256, &renderAtoms[ATOM_GREEN_BUTTON_UP+i] );
		renderAtoms[ATOM_GREEN_BUTTON_UP_D+i] = renderAtoms[ATOM_GREEN_BUTTON_UP+i];
		renderAtoms[ATOM_GREEN_BUTTON_UP_D+i].renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;

		renderAtoms[ATOM_GREEN_BUTTON_DOWN+i].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "icons" ), 0, 0, 1, 1, 56, 56 );
		UIRenderer::SetAtomCoordFromPixel( 0+64*(i/4), 129, 62+64*(i/4), 189, 512, 256, &renderAtoms[ATOM_GREEN_BUTTON_DOWN+i] );
		renderAtoms[ATOM_GREEN_BUTTON_DOWN_D+i] = renderAtoms[ATOM_GREEN_BUTTON_DOWN+i];
		renderAtoms[ATOM_GREEN_BUTTON_DOWN_D+i].renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;
	}

	renderAtoms[ATOM_BLUE_TAB_BUTTON_UP].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "icons" ), 0, 0, 1, 1, 56, 56 );
	UIRenderer::SetAtomCoordFromPixel( 64*7, 128, 64*8, 192, 512, 256, &renderAtoms[ATOM_BLUE_TAB_BUTTON_UP] );

	renderAtoms[ATOM_BLUE_TAB_BUTTON_UP_D] = renderAtoms[ATOM_BLUE_TAB_BUTTON_UP];
	renderAtoms[ATOM_BLUE_TAB_BUTTON_UP_D].renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;

	renderAtoms[ATOM_BLUE_TAB_BUTTON_DOWN].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "icons" ), 0, 0, 1, 1, 56, 56 );
	UIRenderer::SetAtomCoordFromPixel( 64*6, 128, 64*7, 192, 512, 256, &renderAtoms[ATOM_BLUE_TAB_BUTTON_DOWN] );

	renderAtoms[ATOM_BLUE_TAB_BUTTON_DOWN_D] = renderAtoms[ATOM_BLUE_TAB_BUTTON_DOWN];
	renderAtoms[ATOM_BLUE_TAB_BUTTON_DOWN_D].renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;



	for( int i=0; i<ATOM_COUNT; ++i ) {
		GLASSERT( renderAtoms[i].renderState );
		GLASSERT( renderAtoms[i].textureHandle );
	}


	buttonLooks[GREEN_BUTTON].Init( renderAtoms[ ATOM_GREEN_BUTTON_UP ], renderAtoms[ ATOM_GREEN_BUTTON_UP_D ], renderAtoms[ ATOM_GREEN_BUTTON_DOWN ], renderAtoms[ ATOM_GREEN_BUTTON_DOWN_D ] );
	buttonLooks[BLUE_BUTTON].Init( renderAtoms[ ATOM_BLUE_BUTTON_UP ], renderAtoms[ ATOM_BLUE_BUTTON_UP_D ], renderAtoms[ ATOM_BLUE_BUTTON_DOWN ], renderAtoms[ ATOM_BLUE_BUTTON_DOWN_D ] );
	buttonLooks[RED_BUTTON].Init( renderAtoms[ ATOM_RED_BUTTON_UP ], renderAtoms[ ATOM_RED_BUTTON_UP_D ], renderAtoms[ ATOM_RED_BUTTON_DOWN ], renderAtoms[ ATOM_RED_BUTTON_DOWN_D ] );
	buttonLooks[BLUE_TAB_BUTTON].Init( renderAtoms[ ATOM_BLUE_TAB_BUTTON_UP ], renderAtoms[ ATOM_BLUE_TAB_BUTTON_UP_D ], renderAtoms[ ATOM_BLUE_TAB_BUTTON_DOWN ], renderAtoms[ ATOM_BLUE_TAB_BUTTON_DOWN_D ] );
}
