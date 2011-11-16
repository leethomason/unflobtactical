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
#include "../faces/faces.h"

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

			for( int mode=0; mode<3; ++mode ) {
				float fraction, fraction2, damage, dptu;
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
		GLASSERT( t->Format() == Surface::RGB16 );
		t->Upload( pixels, 8 );
	}
	else if ( StrEqual( t->Name(), "black" ) ) {
		U16 pixels[4] = { 0, 0, 0, 0 };
		GLASSERT( t->Format() == Surface::RGB16 );
		t->Upload( pixels, 8 );
	}
	else if ( StrEqual( t->Name(), "faces" ) ) {
		t->Upload( faceSurface.Pixels(), faceSurface.Width()*faceSurface.Height()*2 );
	}
	else {
		GLASSERT( 0 );
	}
}


void Game::LoadPalettes()
{
	const gamedb::Item* parent = database0->Root()->Child( "data" )->Child( "palettes" );
	for( int i=0; i<parent->NumChildren(); ++i ) {
		const gamedb::Item* child = parent->Child( i );
		child = database0->ChainItem( child );

		Palette* p = 0;
		if ( palettes.Size() <= i ) 
			p = palettes.Push();
		else
			p = &palettes[i];
		p->name = child->Name();
		p->dx = child->GetInt( "dx" );
		p->dy = child->GetInt( "dy" );
		p->colors.Clear();
		GLASSERT( (int)p->colors.Capacity() >= p->dx*p->dy );
		p->colors.PushArr( p->dx*p->dy );
		GLASSERT( child->GetDataSize( "colors" ) == (int)(p->dx*p->dy*sizeof(Color4U8)) );
		child->GetData( "colors", (void*)p->colors.Mem(), p->dx*p->dy*sizeof(Color4U8) );
	}
}


const Game::Palette* Game::GetPalette( const char* name ) const
{
	for( int i=0; i<palettes.Size(); ++i ) {
		if ( StrEqual( name, palettes[i].name ) ) {
			return &palettes[i];
		}
	}
	return 0;
}


grinliz::Color4U8 Game::MainPaletteColor( int x, int y )
{
	if ( mainPalette == 0 ) {
		mainPalette = GetPalette( "mainPalette" );
	}
	GLASSERT( x >= 0 && x < mainPalette->dx );
	GLASSERT( y >= 0 && y < mainPalette->dy );
	return mainPalette->colors[ y*mainPalette->dx + x ];
}


void Game::LoadTextures()
{
	TextureManager* texman = TextureManager::Instance();
	texman->CreateTexture( "white", 2, 2, Surface::RGB16, Texture::PARAM_NONE, this );
	texman->CreateTexture( "black", 2, 2, Surface::RGB16, Texture::PARAM_NONE, this );
	texman->CreateTexture( "faces", 64*MAX_TERRANS, 64, Surface::RGBA16, Texture::PARAM_NONE, this );
}


Texture* Game::CalcFaceTexture( const Unit* unit, grinliz::Rectangle2F* uv )
{
	Texture* tex = TextureManager::Instance()->GetTexture( "faces" );
	GLASSERT( tex );

	// Find and return.
	for( int i=0; i<MAX_TERRANS; ++i ) {
		if ( faceCache[i].seed == unit->Body() ) {
			//GLOUTPUT(( "Cache hit %d\n", i ));
			uv->Set( (float)i/(float)MAX_TERRANS, 0,
					 (float)(i+1)/(float)MAX_TERRANS, 1.0f );
			return tex;
		}
	}

	// Didn't find. Create. Need this to work so that MAX_TERRANS in a row
	// will return non-replacing results.
	++faceCacheSlot;
	if ( faceCacheSlot == MAX_TERRANS ) faceCacheSlot = 0;
	//GLOUTPUT(( "Cache miss %d\n", faceCacheSlot ));

	faceCache[faceCacheSlot].seed = unit->Body();
			
	FaceGenerator::FaceParam param;
	param.Generate( unit->Gender(), unit->Body() );

	U32 index = unit->GetValue( Unit::APPEARANCE );
	const Game::Palette* palette = GetPalette( "characterPalette" );

	param.skinColor = palette->colors[index + (1+unit->Gender()*2)*palette->dx]; 
	param.hairColor = palette->colors[index + (0+unit->Gender()*2)*palette->dx];

	// Terrible randomization in this range. Almost
	// always even. Use a secondary lookup.
	int glasses = unit->GetValue( Unit::APPEARANCE_EXT );
	Random random( glasses );
	if ( random.Boolean() ) 
		param.glassesColor = MainPaletteColor( 5, 5 );
	else 
		param.glassesColor = MainPaletteColor( 1, 5 );

	faceGen.GenerateFace( param, &oneFaceSurface );

	// may need to flip:
	int x = faceCacheSlot*64;
	for( int y=0; y<FaceGenerator::SIZE; ++y ) {
		memcpy( faceSurface.Pixels() + y*faceSurface.Pitch() + x*faceSurface.BytesPerPixel(),
				oneFaceSurface.Pixels() + (FaceGenerator::SIZE-1-y)*oneFaceSurface.Pitch(),
				FaceGenerator::BPP * FaceGenerator::SIZE );
	}
	tex->Upload( faceSurface );
	uv->Set( (float)faceCacheSlot/(float)MAX_TERRANS, 0,
			 (float)(faceCacheSlot+1)/(float)MAX_TERRANS, 1.0f );
	return tex;
}


void Game::LoadModel( const char* name )
{
	GLASSERT( modelLoader );

	const gamedb::Item* item = database0->Root()->Child( "models" )->Child( name );
	GLASSERT( item );
//	item = database0->ChainItem( item );

	ModelResource* res = new ModelResource();
	modelLoader->Load( item, res );
	ModelResourceManager::Instance()->AddModelResource( res );
}


void Game::LoadModels()
{
	// Run through the database, and load all the models.
	const gamedb::Item* parent = database0->Root()->Child( "models" );
	GLASSERT( parent );

	for( int i=0; i<parent->NumChildren(); ++i )
	{
		const gamedb::Item* node = parent->Child( i );
		LoadModel( node->Name() );
	}
}


void Game::LoadItemResources()
{
	static const float DAM_LOW		=   30.0f;
	static const float DAM_MED		=   40.0f;
	static const float DAM_MEDHI	=   60.0f;
	static const float DAM_HI		=   80.0f;

	static const float EXDAM_VLOW   =  20.0f;
	static const float EXDAM_MED    = 100.0f;
	static const float EXDAM_HI		= 150.0f;

	// lower is better, and in terms of AREA
	static const float ACC_LOW		= 2.00f;
	static const float ACC_MED		= 1.00f;
	static const float ACC_VHI		= 0.30f;

	static const float SPEED_FAST = 0.8f;
	static const float SPEED_NORMAL = 1.0f;
	static const float SPEED_SLOW	= 1.3f;

	static const float SNAP = ACC_SNAP_SHOT_MULTIPLIER;
	static const float AUTO = ACC_AUTO_SHOT_MULTIPLIER;
	static const float AIM  = ACC_AIMED_SHOT_MULTIPLIER;

	struct ClipInit {
		const char* name;
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
		{ "Clip",	false,	DECO_SHELLS,	15,	 5,		{ 1, 0, 0 },
					COLORDEF( 200, 204, 213 ), SPEED*2.0f,	WIDTH*0.5f, BOLT*3.0f,  "Bullet clip" },
		{ "Cell",	true,	DECO_CELL,		12,  12,	{ 0.0f, 0.8f, 0.2f },
					COLORDEF( 199, 216, 6 ),	SPEED*1.5f,	 WIDTH,		BOLT*2.0f,	"Alien Cell clip" },
		{ "Anti",	true,	DECO_CELL,		4,   20,	{ 0, 0.6f, 0.4f },
					COLORDEF( 227, 125, 220 ),	SPEED,		WIDTH,		BOLT,		"Anti-matter clip" },
		{ "Flame",	false,	DECO_SHELLS,	3,	 8,		{ 0, 0, 1 },
					COLORDEF( 213, 63, 63 ),	SPEED*0.7f,	WIDTH,		BOLT,		"Flame clip" },
		{ "RPG",	false,	DECO_ROCKET,	4,	 8,		{ 0.8f, 0, 0.2f },
					COLORDEF( 200, 204, 213 ),  SPEED*0.8f, WIDTH,		BOLT,		"RPG (grenade) clip" },
		{ "Flare",	false,	DECO_ROCKET,	4,	 5,		{ 0,    0, 1.0f },
					COLORDEF( 213, 63, 63 ),	SPEED*0.7f, WIDTH,		BOLT,		"Flare grenade clip" },
		{ "Smoke",	false,	DECO_ROCKET,	4,	 5,		{ 0,    0, 1.0f },
					COLORDEF( 144, 152, 171 ),	SPEED*0.7f, WIDTH,		BOLT,		"Smoke grenade clip" },
		{ "Spit-Clip",true,	DECO_NONE,	100,	 1,		{ 0.5f, 0, 0.5f },
					COLORDEF( 66, 203, 3 ),		SPEED*0.8f, WIDTH*1.2f,	BOLT*2.5f,	"Spit-Clip-Hidden" },
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

		WeaponItemDef::Weapon mode[WeaponItemDef::MAX_MODE];
	};

	static const float B2 = 1.4f;
	static const float B3 = 1.8f;
	static const float A2 = 0.7f;
	static const float A3 = 0.5f;

	static const WeaponInit weapons[] = {		
		// Terran	resName		deco			description
		//				
		{ "ASLT-1",	"ASLT-1",	DECO_RIFLE,		"Assault Rifle",
			{{ "Snap", 	"Clip",		0,					DAM_MEDHI,		ACC_MED*SNAP,	SPEED_NORMAL*TU_SNAP_SHOT,	"ar3p" },
			{ "Auto", 	"Clip",		WEAPON_AUTO,		DAM_MEDHI,		ACC_MED*AUTO,	SPEED_NORMAL*TU_AUTO_SHOT,	"ar3p" },
			{ "Boom",	"RPG",		WEAPON_EXPLOSIVE,	EXDAM_MED,		ACC_LOW*AIM,	SPEED_NORMAL*TU_SECONDARY_SHOT, "can" },
			{ 0 }, { 0 }}
		},
		{ "ASLT-2",	"ASLT-2",	DECO_RIFLE,		"Assault Rifle",
			{{ "Snap", 	"Clip",		0,					DAM_MEDHI*B2,	ACC_MED*SNAP,	SPEED_NORMAL*TU_SNAP_SHOT,	"ar3p" },
			{ "Auto", 	"Clip",		WEAPON_AUTO,		DAM_MEDHI*B2,	ACC_MED*AUTO,	SPEED_NORMAL*TU_AUTO_SHOT,	"ar3p" },
			{ "Boom",	"RPG",		WEAPON_EXPLOSIVE,	EXDAM_MED*B2,	ACC_LOW*AIM,	SPEED_NORMAL*TU_SECONDARY_SHOT, "can" },
			{ 0 }, { 0 }}
		},
		{ "ASLT-3",	"ASLT-3",	DECO_RIFLE,		"Pulse Rifle",			
			{{ "Snap",	"Clip",		0,					DAM_MEDHI*B3,	ACC_MED*SNAP,	SPEED_NORMAL*TU_SNAP_SHOT,	"ar3p" },
			{ "Auto",	"Clip",		WEAPON_AUTO,		DAM_MEDHI*B3,	ACC_MED*AUTO,	SPEED_NORMAL*TU_AUTO_SHOT,	"ar3p" },
			{ "Boom",	"RPG",		WEAPON_EXPLOSIVE,	EXDAM_MED*B3,	ACC_LOW*AIM,	SPEED_NORMAL*TU_SECONDARY_SHOT,	"can" },
			{ 0 }, { 0 }}
		},

		{ "LR-1",	"LR-1",		DECO_RIFLE,		"Long Range",		
			{{ "Snap", 	"Clip",		0,					DAM_HI,			ACC_VHI*SNAP,	SPEED_NORMAL*TU_SNAP_SHOT,	"ar3l" },
			{ "Aimed", "Clip",		0,					DAM_HI,			ACC_VHI*AIM,	SPEED_NORMAL*TU_AIMED_SHOT,	"ar3l" },
			{ 0 }, { 0 }}
		},
		{ "LR-2",	"LR-2",		DECO_RIFLE,		"Long Range",	
			{{ "Snap", 	"Clip",		0,					DAM_HI,			ACC_VHI*SNAP*A2,	SPEED_NORMAL*TU_SNAP_SHOT,	"ar3l" },
			{ "Aimed", "Clip",		0,					DAM_HI,			ACC_VHI*AIM*A2,		SPEED_NORMAL*TU_AIMED_SHOT,	"ar3l" },
			{ 0 }, { 0 }}
		},
		{ "LR-3",	"LR-3",		DECO_RIFLE,		"Callahan LR",
			{{ "Snap", 	"Clip",		0,					DAM_HI*B2,		ACC_VHI*SNAP*A3,	SPEED_NORMAL*TU_SNAP_SHOT,	"ar3l" },
			{ "Aimed", "Clip",		0,					DAM_HI*B2,		ACC_VHI*AIM*A3,		SPEED_NORMAL*TU_AIMED_SHOT,	"ar3l" },
			{ 0 }, { 0 }}
		},
		{ "MCAN-1",	"CANON-1",	DECO_PISTOL,	"Mini-Canon",
			{{ "Snap", "Flame", WEAPON_EXPLOSIVE,		EXDAM_MED,		ACC_MED*SNAP,		SPEED_SLOW*TU_SNAP_SHOT,	"can" },
			{ "Flame", "Flame", WEAPON_EXPLOSIVE,		EXDAM_MED,		ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" },
			{ "RPG",   "RPG",	WEAPON_EXPLOSIVE,		EXDAM_MED,		ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" },
			{ "Flare", "Flare",	WEAPON_EXPLOSIVE | WEAPON_FLARE | WEAPON_DISTANCE,		EXDAM_VLOW,		ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" },
			{ "Smoke", "Smoke",	WEAPON_EXPLOSIVE | WEAPON_SMOKE | WEAPON_DISTANCE,		EXDAM_VLOW,		ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" }},
		},
		{ "MCAN-2",	"CANON-2",	DECO_PISTOL,	"Mini-Canon",
			{{ "Snap", "Flame", WEAPON_EXPLOSIVE,		EXDAM_MED*B2,	ACC_MED*SNAP,		SPEED_SLOW*TU_SNAP_SHOT,	"can" },
			{ "Flame", "Flame", WEAPON_EXPLOSIVE,		EXDAM_MED*B2,	ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" },
			{ "RPG",   "RPG",	WEAPON_EXPLOSIVE,		EXDAM_MED*B2,	ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" },
			{ "Flare", "Flare",	WEAPON_EXPLOSIVE | WEAPON_FLARE | WEAPON_DISTANCE,		EXDAM_VLOW,		ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" },
			{ "Smoke", "Smoke",	WEAPON_EXPLOSIVE | WEAPON_SMOKE | WEAPON_DISTANCE,		EXDAM_VLOW,		ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" }},
		},
		{ "MCAN-3",	"CANON-3",	DECO_PISTOL,	"Dauntless Canon",
			{{ "Snap", "Flame", WEAPON_EXPLOSIVE,		EXDAM_MED*B3,	ACC_MED*SNAP,		SPEED_SLOW*TU_SNAP_SHOT,	"can" },
			{ "Flame", "Flame", WEAPON_EXPLOSIVE,		EXDAM_MED*B3,	ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" },
			{ "RPG",   "RPG",	WEAPON_EXPLOSIVE,		EXDAM_MED*B3,	ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" },
			{ "Flare", "Flare",	WEAPON_EXPLOSIVE | WEAPON_FLARE | WEAPON_DISTANCE,		EXDAM_VLOW,		ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" },
			{ "Smoke", "Smoke",	WEAPON_EXPLOSIVE | WEAPON_SMOKE | WEAPON_DISTANCE,		EXDAM_VLOW,		ACC_MED*AIM,		SPEED_SLOW*TU_AIMED_SHOT,	"can" }},
		},

		// Alien
		{ "RAY-1",	"RAY-1",	DECO_RAYGUN,	"Ray Gun",		
			{{ "Snap", "Cell",	0,						DAM_MED,		ACC_MED*SNAP,	SPEED_FAST*TU_SNAP_SHOT,	"ray1" },
			{ "Aimed", "Cell",	0,						DAM_MED,		ACC_MED*AIM,	SPEED_FAST*TU_AIMED_SHOT,	"ray1" },
			{ 0 }, { 0 }, { 0 }}
		},
		{ "RAY-2",	"RAY-2",	DECO_RAYGUN,	"Ray Gun",	
			{{ "Snap", "Cell",	0,						DAM_MED*B3,		ACC_MED*SNAP*A2,	SPEED_FAST*TU_SNAP_SHOT,	"ray1" },
			{ "Aimed", "Cell",	0,						DAM_MED*B3,		ACC_MED*AIM*A2,		SPEED_FAST*TU_AIMED_SHOT,	"ray1" },
			{ 0 }, { 0 }, { 0 }}
		},
		{ "RAY-3",	"RAY-3",	DECO_RAYGUN,	"Ray Gun",		
			{{ "Snap", "Cell",	0,						DAM_MED*B2*B3,	ACC_MED*SNAP*A3,	SPEED_FAST*TU_SNAP_SHOT,	"ray1" },
			{ "Aimed", "Cell",	0,						DAM_MED*B2*B3,	ACC_MED*AIM*A3,		SPEED_FAST*TU_AIMED_SHOT,	"ray1" },
			{ 0 }, { 0 }, { 0 }}
		},

		{ "PLS-1",	"PLS-1",	DECO_RIFLE,		"Plasma Rifle",
			{{	"Snap", "Cell",	0,						DAM_HI,			ACC_MED*SNAP,	SPEED_NORMAL*TU_SNAP_SHOT,		"plasma" },
			{	"Auto", "Cell",	WEAPON_AUTO,			DAM_HI,			ACC_MED*AUTO,	SPEED_NORMAL*TU_AUTO_SHOT,		"plasma" },
			{	"Boom", "Anti",	WEAPON_EXPLOSIVE,		EXDAM_HI,		ACC_MED*AIM,	SPEED_NORMAL*TU_SECONDARY_SHOT,	"nullp", },
			{ 0 }, { 0 }}
		},
		{ "PLS-2",	"PLS-2",	DECO_RIFLE,		"Plasma Rifle",
			{{	"Snap", "Cell",	0,						DAM_HI*B2,		ACC_MED*SNAP,	SPEED_NORMAL*TU_SNAP_SHOT,		"plasma" },
			{	"Auto", "Cell",	WEAPON_AUTO,			DAM_HI*B2,		ACC_MED*AUTO,	SPEED_NORMAL*TU_AUTO_SHOT,		"plasma" },
			{	"Boom", "Anti",	WEAPON_EXPLOSIVE,		EXDAM_HI,		ACC_MED*AIM,	SPEED_NORMAL*TU_SECONDARY_SHOT,	"nullp", },
			{ 0 }, { 0 }}
		},
		{ "PLS-3",	"PLS-3",	DECO_RIFLE,		"Plasma Rifle",
			{{	"Snap", "Cell",	0,						DAM_HI*B3,		ACC_MED*SNAP,	SPEED_NORMAL*TU_SNAP_SHOT,		"plasma" },
			{	"Auto", "Cell",	WEAPON_AUTO,			DAM_HI*B3,		ACC_MED*AUTO,	SPEED_NORMAL*TU_AUTO_SHOT,		"plasma" },
			{	"Boom", "Anti",	WEAPON_EXPLOSIVE,		EXDAM_HI*B2,	ACC_MED*AIM,	SPEED_NORMAL*TU_SECONDARY_SHOT,	"nullp", },
			{ 0 }, { 0 }}
		},

		{ "STRM-1",	"STORM-1",	DECO_RAYGUN,	"Fire Storm",
			{{ "Snap", "Cell",	WEAPON_EXPLOSIVE | WEAPON_INCENDIARY,	EXDAM_MED,		ACC_LOW*SNAP,	SPEED_NORMAL*TU_SNAP_SHOT,	"nullp" },
			{ "Aimed","Cell",	WEAPON_EXPLOSIVE | WEAPON_INCENDIARY,	EXDAM_MED,		ACC_LOW*AIM,	SPEED_NORMAL*TU_AIMED_SHOT,	"nullp" },
			{ 0 }, { 0 }, { 0 }}
		},
		{ "STRM-2",	"STORM-2",	DECO_RAYGUN,	"Fire Storm",
			{{ "Snap", "Cell",	WEAPON_EXPLOSIVE | WEAPON_INCENDIARY,	EXDAM_MED*B2,	ACC_LOW*SNAP,	SPEED_NORMAL*TU_SNAP_SHOT,	"nullp" },
			{ "Aimed","Cell",	WEAPON_EXPLOSIVE | WEAPON_INCENDIARY,	EXDAM_MED*B2,	ACC_LOW*AIM,	SPEED_NORMAL*TU_AIMED_SHOT,	"nullp" },
			{ 0 }, { 0 }, { 0 }}
		},
		{ "STRM-3",	"STORM-3",	DECO_RAYGUN,	"Fire Storm",
			{{ "Snap", "Cell",	WEAPON_EXPLOSIVE | WEAPON_INCENDIARY,	EXDAM_MED*B2,	ACC_LOW*A2*SNAP,	SPEED_NORMAL*TU_SNAP_SHOT,	"nullp" },
			{ "Aimed","Cell",	WEAPON_EXPLOSIVE | WEAPON_INCENDIARY,	EXDAM_MED*B2,	ACC_LOW*A2*AIM,		SPEED_NORMAL*TU_AIMED_SHOT,	"nullp" },
			{ 0 }, { 0 }, { 0 }}
		},

		// Intrinsic:
		{ "Spit-1",	0,	DECO_NONE,	"Spit-Hidden",	
			{{ "Snap", "Spit-Clip",		0,								DAM_LOW,		ACC_MED*SNAP*A2,	SPEED_NORMAL*TU_SNAP_SHOT,	0 },
			{ "Aimed", "Spit-Clip",		0,								DAM_MED,		ACC_MED*AIM*A2,		SPEED_NORMAL*TU_AIMED_SHOT,	0 },
			{ 0 }, { 0 }, { 0 }}
		},
		{ "Spit-2",	0,	DECO_NONE,	"Spit-Hidden",	
			{{ "Snap", "Spit-Clip",		0,								DAM_LOW*B2,		ACC_MED*SNAP*A2,	SPEED_NORMAL*TU_SNAP_SHOT,	0 },
			{ "Aimed", "Spit-Clip",		0,								DAM_MED*B2,		ACC_MED*AIM*A2,		SPEED_NORMAL*TU_AIMED_SHOT,	0 },
			{ 0 }, { 0 }, { 0 }}
		},
		{ "Spit-3",	0,	DECO_NONE,	"Spit-Hidden",	
			{{ "Snap", "Spit-Clip",		0,								DAM_LOW*B3,		ACC_MED*SNAP*A2,	SPEED_NORMAL*TU_SNAP_SHOT,	0 },
			{ "Aimed", "Spit-Clip",		0,								DAM_MED*B3,		ACC_MED*AIM*A2,		SPEED_NORMAL*TU_AIMED_SHOT,	0 },
			{ 0 }, { 0 }, { 0 }}
		},
	};

	//GLOUTPUT(( "Start loading weapons.\n" ));
	bool alien = false;
	int len=sizeof(weapons)/sizeof(WeaponInit);
	for( int i=0; i<len; ++i ) {
		WeaponItemDef* item = new WeaponItemDef();
		
		if (    StrEqual( weapons[i].mode[0].clipItemDefName, "Cell" )
			 || StrEqual( weapons[i].mode[0].clipItemDefName, "Anti" ) ) 
		{
			alien = true;
		}

		item->InitBase( weapons[i].name, 
						weapons[i].desc, 
						weapons[i].deco,
						0,	// set below
						alien,
						weapons[i].resName ? ModelResourceManager::Instance()->GetModelResource( weapons[i].resName ) : 0 );

		for( int j=0; j<WeaponItemDef::MAX_MODE; ++j ) {
			//GLOUTPUT(( "weapon %d %d\n", i, j ));
			item->weapon[j] = 0;
			item->clipItemDef[j] = 0;

			if ( weapons[i].mode[j].InUse() ) {
				item->weapon[j] = &weapons[i].mode[j];
				item->clipItemDef[j] = itemDefArr.Query( weapons[i].mode[j].clipItemDefName )->IsClip();
				GLASSERT( item->clipItemDef[j] );
			}
		}

		{
			BulletTarget bulletTarget( 8.0f );
			Stats stats; stats.SetSTR( 40 ); stats.SetDEX( 40 ); stats.SetPSY( 40 ); stats.SetRank( 2 );
			float fraction, fraction2, damage, dptu;
			item->FireStatistics( 1, stats.AccuracyArea(), bulletTarget, &fraction, &fraction2, &damage, &dptu );

			U32 price = LRintf( dptu );
			if ( item->IsAlien() )
				price = price * 3 / 2;
			item->price = price;
		}
		itemDefArr.Add( item );
	}
	//GLOUTPUT(( "End loading weapons.\n" ));

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
		{ "SG:P",	0,				DECO_SHIELD,	40, false, "Psi Shield" },

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
		{ "ARM-1",	0,				DECO_ARMOR, 10,	false,	"Armor" },
		{ "ARM-2",	0,				DECO_ARMOR, 30,	false, 	"Armor" },
		{ "ARM-3",	0,				DECO_ARMOR, 60,	false,	"Armor" },
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

	renderAtoms[ATOM_TEXT].Init( (const void*)UIRenderer::RENDERSTATE_UI_TEXT, (const void*)tm->GetTexture( "font" ), 0, 0, 1, 1 );
	renderAtoms[ATOM_TEXT_D].Init( (const void*)UIRenderer::RENDERSTATE_UI_TEXT_DISABLED, (const void*)tm->GetTexture( "font" ), 0, 0, 1, 1 );

	renderAtoms[ATOM_TACTICAL_BACKGROUND].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, (const void*)tm->GetTexture( "intro" ), 0, 0, 1, 1 );
	renderAtoms[ATOM_TACTICAL_BACKGROUND_TEXT].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "title" ), 0, 0, 1, 1 );

	renderAtoms[ATOM_GEO_VICTORY].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "victory" ), 0, 0, 1, 1 );
	renderAtoms[ATOM_GEO_DEFEAT].Init(  (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "defeat" ), 0, 0, 1, 1 );

	for( int i=0; i <= (ATOM_RED_BUTTON_UP-ATOM_GREEN_BUTTON_UP); i += 4 ) {
		renderAtoms[ATOM_GREEN_BUTTON_UP+i].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "icons" ), 0, 0, 1, 1 );
		UIRenderer::SetAtomCoordFromPixel( 0+64*(i/4), 193, 62+64*(i/4), 253, 512, 256, &renderAtoms[ATOM_GREEN_BUTTON_UP+i] );
		renderAtoms[ATOM_GREEN_BUTTON_UP_D+i] = renderAtoms[ATOM_GREEN_BUTTON_UP+i];
		renderAtoms[ATOM_GREEN_BUTTON_UP_D+i].renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;

		renderAtoms[ATOM_GREEN_BUTTON_DOWN+i].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "icons" ), 0, 0, 1, 1 );
		UIRenderer::SetAtomCoordFromPixel( 0+64*(i/4), 129, 62+64*(i/4), 189, 512, 256, &renderAtoms[ATOM_GREEN_BUTTON_DOWN+i] );
		renderAtoms[ATOM_GREEN_BUTTON_DOWN_D+i] = renderAtoms[ATOM_GREEN_BUTTON_DOWN+i];
		renderAtoms[ATOM_GREEN_BUTTON_DOWN_D+i].renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;
	}

	renderAtoms[ATOM_BLUE_TAB_BUTTON_UP].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "icons" ), 0, 0, 1, 1 );
	UIRenderer::SetAtomCoordFromPixel( 64*7, 128, 64*8, 192, 512, 256, &renderAtoms[ATOM_BLUE_TAB_BUTTON_UP] );

	renderAtoms[ATOM_BLUE_TAB_BUTTON_UP_D] = renderAtoms[ATOM_BLUE_TAB_BUTTON_UP];
	renderAtoms[ATOM_BLUE_TAB_BUTTON_UP_D].renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;

	renderAtoms[ATOM_BLUE_TAB_BUTTON_DOWN].Init( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)tm->GetTexture( "icons" ), 0, 0, 1, 1 );
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
