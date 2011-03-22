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
		const ItemDef* itemDef = itemDefArr.Query( i );
		if ( itemDef == 0 )
			continue;

		const WeaponItemDef* wid = itemDef->IsWeapon();
		if ( wid ) {

			fprintf( fp, "%-10s", wid->name );

			for( int j=0; j<3; ++j ) {
				float fraction, fraction2, damage, dptu;
				WeaponMode mode = (WeaponMode)j;
				if ( wid->HasWeapon( mode )) {
					DamageDesc dd;
					wid->DamageBase( mode, &dd );

					BulletTarget bulletTarget( range );
					wid->FireStatistics( mode, stats.AccuracyArea(), bulletTarget, &fraction, &fraction2, &damage, &dptu );
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
	else if ( StrEqual( t->Name(), "black" ) ) {
		U16 pixels[4] = { 0, 0, 0, 0 };
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
	texman->CreateTexture( "black", 2, 2, Surface::RGB16, Texture::PARAM_NONE, this );

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


void Game::LoadItemResources()
{
	//const float DAM_LOW		=   20.0f;
	//const float DAM_MEDLOW	=   30.0f;
	const float DAM_MED		=   40.0f;
	const float DAM_MEDHI	=   60.0f;
	const float DAM_HI		=   80.0f;
	//const float DAM_HIPLUS	=   90.0f;
	//const float DAM_VHI		=  100.0f;

	//const float EXDAM_LOW   =  50.0f;
	const float EXDAM_MED   = 100.0f;
	const float EXDAM_HI	= 150.0f;
	//const float EXDAM_VHI	= 200.0f;

	// lower is better, and in terms of AREA
	//const float ACC_VLOW	= 2.50f;
	const float ACC_LOW		= 2.00f;
	//const float ACC_MEDLOW	= 1.50f;
	const float ACC_MED		= 1.00f;
	//const float ACC_MEDHI	= 0.80f;
	//const float ACC_HI		= 0.50f;
	const float ACC_VHI		= 0.30f;

	const float SPEED_FAST = 0.8f;
	const float SPEED_NORMAL = 1.0f;
	const float SPEED_SLOW	= 1.3f;
	//const int POW_LOW = 10;
	//const int POW_MED = 20;
	//const int POW_HI  = 50;

	struct ClipInit {
		const char* name;
		char		abbreviation;
		bool		alien;
		int			deco;
		int			rounds;
		U32			price;
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
		{ "Clip",	'c',	false,	DECO_SHELLS,	15,	 5,		{ 1, 0, 0 },
					COLORDEF( 200, 204, 213 ), SPEED*2.0f,	WIDTH*0.5f, BOLT*3.0f, "7mm 15 round auto-clip" },
		{ "Cell",	'e',	true,	DECO_CELL,		12,  20,	{ 0.0f, 0.8f, 0.2f },
					COLORDEF( 199, 216, 6 ),	SPEED*1.5f,	 WIDTH,		BOLT*2.0f,	"10MW Cell" },
		{ "Anti",	't',	true,	DECO_CELL,		4,   50,	{ 0, 0.6f, 0.4f },
					COLORDEF( 227, 125, 220 ),	SPEED,		WIDTH,		BOLT,		"Anti matter field rounds" },
		{ "Flame",	'f',	false,	DECO_SHELLS,	3,	 8,		{ 0, 0, 1 },
					COLORDEF( 213, 63, 63 ),	SPEED*0.7f,	WIDTH,		BOLT,		"Incendiary Heavy Round" },
		{ "RPG",	'r',	false,	DECO_ROCKET,	4,	 8,		{ 0.8f, 0, 0.2f },
					COLORDEF( 200, 204, 213 ),  SPEED*0.8f, WIDTH,		BOLT,		"Grenade Rounds" },
		{ 0 }
	};

#	undef COLORDEF

	for( int i=0; clips[i].name; ++i ) {
		ClipItemDef* item = new ClipItemDef();
		item->InitBase( clips[i].name,
						clips[i].desc,
						clips[i].deco,
						clips[i].price,
						clips[i].alien,
						0 );

		item->defaultRounds = clips[i].rounds;
		item->dd = clips[i].dd;

		item->abbreviation = clips[i].abbreviation;
		item->color = clips[i].color;
		item->speed = clips[i].speed;
		item->width = clips[i].width;
		item->length = clips[i].length;

		itemDefArr.Add( item );
	}

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

	static const float B2 = 1.4f;
	static const float B3 = 1.8f;
	static const float A2 = 0.7f;
	static const float A3 = 0.5f;

	static const WeaponInit weapons[] = {		
		// Terran	resName		deco			description
		//				
		{ "ASLT-1",	"ASLT-1",	DECO_RIFLE,		"Assault Rifle",		{ "Snap", "Auto", "Boom" },		SPEED_NORMAL,
				"Clip",			WEAPON_AUTO,							DAM_MEDHI,		ACC_MED,	"ar3p",
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_MED,		ACC_LOW,	"can" },
		{ "ASLT-2",	"ASLT-2",	DECO_RIFLE,		"Assault Rifle",		{ "Snap", "Auto", "Boom" },		SPEED_NORMAL,
				"Clip",			WEAPON_AUTO,							DAM_MEDHI*B2,	ACC_MED,	"ar3p",
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_MED*B2,	ACC_LOW,	"can" },
		{ "ASLT-3",	"ASLT-3",	DECO_RIFLE,		"Assault Rifle",		{ "Snap", "Auto", "Boom" },		SPEED_NORMAL,
				"Clip",			WEAPON_AUTO,							DAM_MEDHI*B3,	ACC_MED,	"ar3p",
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_MED*B3,	ACC_LOW,	"can" },

		{ "LR-1",	"LR-1",		DECO_RIFLE,		"Long Range",			{ "Snap", "Aimed", 0 },		SPEED_NORMAL,
				"Clip",			0,										DAM_HI,			ACC_VHI,	"ar3l",
				0 },
		{ "LR-2",	"LR-2",		DECO_RIFLE,		"Long Range",			{ "Snap", "Aimed", 0 },		SPEED_NORMAL,
				"Clip",			0,										DAM_HI,			ACC_VHI*A2,	"ar3l",
				0 },
		{ "LR-3",	"LR-3",		DECO_RIFLE,		"Long Range",			{ "Snap", "Aimed", 0 },		SPEED_NORMAL,
				"Clip",			0,										DAM_HI*B2,		ACC_VHI*A3,	"ar3l",
				0 },

		{ "MCAN-1",	"CANON-1",	DECO_PISTOL,	"Mini-Canon",			{ "Snap", "Flame", "Boom" },	SPEED_SLOW,
				"Flame",		WEAPON_EXPLOSIVE,						EXDAM_MED,		ACC_MED,	"can",
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_MED,		ACC_MED,	"can", },
		{ "MCAN-2",	"CANON-2",	DECO_PISTOL,	"Mini-Canon",			{ "Snap", "Flame", "Boom" },	SPEED_SLOW,
				"Flame",		WEAPON_EXPLOSIVE,						EXDAM_MED*B2,	ACC_MED,	"can",
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_MED*B2,	ACC_MED,	"can", },
		{ "MCAN-3",	"CANON-3",	DECO_PISTOL,	"Mini-Canon",			{ "Snap", "Flame", "Boom" },	SPEED_SLOW,
				"Flame",		WEAPON_EXPLOSIVE,						EXDAM_MED*B3,	ACC_MED,	"can",
				"RPG",			WEAPON_EXPLOSIVE,						EXDAM_MED*B3,	ACC_MED,	"can", },

		// Alien
		{ "RAY-1",	"RAY-1",	DECO_RAYGUN,	"Ray Gun",				{ "Snap", "Aimed", 0 },			SPEED_FAST,
				"Cell",			0,										DAM_MED,		ACC_MED,	"ray1",
				0  },
		{ "RAY-2",	"RAY-2",	DECO_RAYGUN,	"Ray Gun",				{ "Snap", "Aimed", 0 },			SPEED_FAST,
				"Cell",			0,										DAM_MED*B3,		ACC_MED*A2,	"ray1",
				0  },
		{ "RAY-3",	"RAY-3",	DECO_RAYGUN,	"Ray Gun",				{ "Snap", "Aimed", 0 },			SPEED_FAST,
				"Cell",			0,										DAM_MED*B2*B3,	ACC_MED*A3,	"ray1",
				0  },

		{ "PLS-1",	"PLS-1",	DECO_RIFLE,		"Plasma Rifle",			{ "Snap", "Auto", "Boom" },		SPEED_NORMAL,
				"Cell",			WEAPON_AUTO,							DAM_HI,			ACC_MED,	"plasma",
				"Anti",			WEAPON_EXPLOSIVE,						EXDAM_HI,		ACC_MED,	"nullp", },
		{ "PLS-2",	"PLS-2",	DECO_RIFLE,		"Plasma Rifle",			{ "Snap", "Auto", "Boom" },		SPEED_NORMAL,
				"Cell",			WEAPON_AUTO,							DAM_HI*B2,		ACC_MED,	"plasma",
				"Anti",			WEAPON_EXPLOSIVE,						EXDAM_HI,		ACC_MED,	"nullp", },
		{ "PLS-3",	"PLS-3",	DECO_RIFLE,		"Plasma Rifle",			{ "Snap", "Auto", "Boom" },		SPEED_NORMAL,
				"Cell",			WEAPON_AUTO,							DAM_HI*B3,		ACC_MED,	"plasma",
				"Anti",	    	WEAPON_EXPLOSIVE,						EXDAM_HI*B2,	ACC_MED,	"nullp", },


		{ "STRM-1",	"STORM-1",	DECO_RAYGUN,	"Fire Storm",			{ "Snap", "Flame", "Boom" },	SPEED_NORMAL,
				"Cell",			WEAPON_EXPLOSIVE | WEAPON_INCENDIARY,	EXDAM_MED,		ACC_LOW,	"nullp",
				0 },
		{ "STRM-2",	"STORM-2",	DECO_RAYGUN,	"Fire Storm",			{ "Snap", "Flame", "Boom" },	SPEED_NORMAL,
				"Cell",			WEAPON_EXPLOSIVE | WEAPON_INCENDIARY,	EXDAM_MED*B2,	ACC_LOW,	"nullp",
				0 },
		{ "STRM-3",	"STORM-3",	DECO_RAYGUN,	"Fire Storm",			{ "Snap", "Flame", "Boom" },	SPEED_NORMAL,
				"Cell",			WEAPON_EXPLOSIVE | WEAPON_INCENDIARY,	EXDAM_MED*B2,	ACC_LOW*A2,	"nullp",
				0 },
		{ 0 }
	};

	bool alien = false;
	for( int i=0; weapons[i].name; ++i ) {
		WeaponItemDef* item = new WeaponItemDef();
		
		// HACK
		if ( weapons[i].deco == DECO_RAYGUN )
			alien = true;

		GLASSERT( !weapons[i].clip0 || ( weapons[i].clip0 != weapons[i].clip1 ) );	// code later get confused.

		item->InitBase( weapons[i].name, 
						weapons[i].desc, 
						weapons[i].deco,
						0,	// set below
						alien,
						ModelResourceManager::Instance()->GetModelResource( weapons[i].resName ) );

		for( int j=0; j<3; ++j ) {
			item->fireDesc[j]		= weapons[i].modes[j];
		}
		item->speed					= weapons[i].speed;

		item->weapon[0].clipItemDef = 0;
		if ( weapons[i].clip0 ) {
			const ItemDef* clipItemDef = itemDefArr.Query( weapons[i].clip0 );
			GLASSERT( clipItemDef );
			item->weapon[0].clipItemDef = clipItemDef->IsClip();
			GLASSERT( item->weapon[0].clipItemDef );
		}
		item->weapon[0].flags		= weapons[i].flags0;
		item->weapon[0].damage		= weapons[i].damage0;
		item->weapon[0].accuracy	= weapons[i].acc0;
		item->weapon[0].sound		= weapons[i].sound0;

		item->weapon[1].clipItemDef = 0;
		if ( weapons[i].clip1 ) {
			const ItemDef* clipItemDef = itemDefArr.Query( weapons[i].clip1 );
			GLASSERT( clipItemDef );
			item->weapon[1].clipItemDef = clipItemDef->IsClip();
			GLASSERT( item->weapon[0].clipItemDef );
		}
		item->weapon[1].flags		= weapons[i].flags1;
		item->weapon[1].damage		= weapons[i].damage1;
		item->weapon[1].accuracy	= weapons[i].acc1;
		item->weapon[1].sound		= weapons[i].sound1;

		{
			BulletTarget bulletTarget( 8.0f );
			Stats stats; stats.SetSTR( 40 ); stats.SetDEX( 40 ); stats.SetPSY( 40 ); stats.SetRank( 2 );
			float fraction, fraction2, damage, dptu;
			item->FireStatistics( kAutoFireMode, stats.AccuracyArea(), bulletTarget, &fraction, &fraction2, &damage, &dptu );

			U32 price = LRintf( dptu );
			if ( item->IsAlien() )
				price = price * 3 / 2;
			item->price = price;
		}
		itemDefArr.Add( item );
	}

	struct ItemInit {
		const char* name;
		const char* resName;
		int deco;
		U32 price;
		bool alien;
		const char* desc;
	};

	static const ItemInit items[] = {		
		{ "Cor:S",	0,				DECO_METAL,		20, true, "Scout tech core" },
		{ "Cor:F",	0,				DECO_METAL,		40, true, "Frigate tech core" },
		{ "Cor:B",	0,				DECO_METAL,		80, true, "Battleship tech core" },
		{ "Green",	0,				DECO_ALIEN,		10, true, "Green" },
		{ "Prime",	0,				DECO_ALIEN,		20, true, "Prime" },
		{ "Hrnet",	0,				DECO_ALIEN,		12, true, "Hornet" },
		{ "Jackl",	0,				DECO_ALIEN,		15, true, "Jackal" },
		{ "Viper",	0,				DECO_ALIEN,		18, true, "Viper" },

		// Special case: used *only* on the base screen.
		{ "Soldr",	0,				DECO_CHARACTER,	-80,	false, "Soldier" },
		{ "Sctst",	0,				DECO_CHARACTER, -120, false, "Scientist" },

		{ "SG:E",	0,				DECO_SHIELD,	80, false, "Energy Shield" },
		{ "SG:I",	0,				DECO_SHIELD,	50, false, "Incendiary Shield" },
		{ "SG:K",	0,				DECO_SHIELD,	40, false, "Kinetic Shield" },

		{ 0 }
	};

	for( int i=0; items[i].name; ++i ) {
		ItemDef* item = new ItemDef();
		item->InitBase( items[i].name,
						items[i].desc,
						items[i].deco,
						items[i].price,
						items[i].alien,
						items[i].resName ? ModelResourceManager::Instance()->GetModelResource( items[i].resName ) : 0 );
		itemDefArr.Add( item );
	}

	static const ItemInit armor[] = {		
		{ "ARM-1",	0,				DECO_ARMOR, 10,		"Memsteel Armor" },
		{ "ARM-2",	0,				DECO_ARMOR, 40,		"Power Armor" },
		{ "ARM-3",	0,				DECO_ARMOR, 80,		"Power Shield" },
		{ 0 }
	};

	for( int i=0; armor[i].name; ++i ) {
		ArmorItemDef* item = new ArmorItemDef();
		item->InitBase( armor[i].name,
						armor[i].desc,
						armor[i].deco,
						armor[i].price,
						false,	// all armor is terran
						armor[i].resName ? ModelResourceManager::Instance()->GetModelResource( items[i].resName ) : 0 );
		itemDefArr.Add( item );
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


void Game::LoadAtoms()
{
	TextureManager* tm = TextureManager::Instance();

	renderAtoms[ATOM_TEXT].Init( (const void*)UIRenderer::RENDERSTATE_UI_TEXT, (const void*)tm->GetTexture( "stdfont2" ), 0, 0, 1, 1, 256, 128 );
	renderAtoms[ATOM_TEXT_D].Init( (const void*)UIRenderer::RENDERSTATE_UI_TEXT_DISABLED, (const void*)tm->GetTexture( "stdfont2" ), 0, 0, 1, 1, 256, 128 );

	renderAtoms[ATOM_TACTICAL_BACKGROUND].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, (const void*)tm->GetTexture( "intro" ), 0, 0, 1, 1, 64, 64 );
	renderAtoms[ATOM_TACTICAL_BACKGROUND_TEXT].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "title" ), 0, 0, 1, 1, 256, 128 );

	renderAtoms[ATOM_GEO_VICTORY].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "victory" ), 0, 0, 1, 1, 256, 128 );
	renderAtoms[ATOM_GEO_DEFEAT].Init(  (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "defeat" ), 0, 0, 1, 1, 256, 128 );

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
