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

#include "tacmap.h"
#include "item.h"
#include "game.h"

#include "../engine/loosequadtree.h"
#include "../grinliz/glrectangle.h"
#include "../tinyxml/tinyxml.h"

using namespace grinliz;

static const int HP_LOW		= 10;
static const int HP_MEDLOW		= 40;
static const int HP_MED		= 80;
static const int HP_HIGH		= 200;
static const int HP_STEEL		= 400;
static const int INDESTRUCT	= 0xffff;

static const int SLOWBURN = 50;
static const int BURN = 128;
static const int FASTBURN = 255;

static const int OBSCURES = MapItemDef::OBSCURES;
static const int EXPLODES = MapItemDef::EXPLODES;
static const int ROTATES = MapItemDef::ROTATES;

/*
	Totally non-obvious path/visibility coordinates. Some sort of axis flipping goes 
	on in here somewhere. But from the point of view of the modeller (AC3D) the bits are:

				bit	value
	South	-z	1	1
	East	+x	2	2
	North	+z	3	4
	West	-x	4	8
*/
	
const MapItemDef TacMap::itemDefArr[NUM_ITEM_DEF] = 
{
		// model		open			destroyed	cx, cz	hp			material	pather
	{	"tree",			0,				"stump",	1,	1,	HP_MEDLOW,	BURN,		"f", "0", OBSCURES },
	{	"tree2",		0,				"stump",	1,	1,	HP_MEDLOW,	BURN,		"f", "0", OBSCURES },
	{	"tree3",		0,				"stump",	1,	1,	HP_MEDLOW,	BURN,		"f", "0", OBSCURES },
	{	"tree4",		0,				0,			1,	1,	HP_MEDLOW,	BURN,		"f", "0", OBSCURES },
	{	"cactus0",		0,				0,			1,	1,	HP_MEDLOW,	SLOWBURN,	"f", "0", OBSCURES },
	{	"cactus1",		0,				0,			1,	1,	HP_MEDLOW,	SLOWBURN,	"f", "0", OBSCURES },
	{	"cactus2",		0,				0,			1,	1,	HP_MEDLOW,	SLOWBURN,	"f", "0", OBSCURES },

		// model		open			destroyed	cx, cz	hp			flammable	pather visibility
	{	"stonewall_unit",0,	"stonewall_unitD",		1,	1,	HP_MED,		0,			"f", "0" },
	{	"stonewall_join",0,	"stonewall_unitD",		1,	1,	HP_MED,		0,			"f", "0" },
	{	"woodfence",	0,				0,			2,	1,	HP_LOW,		BURN,		"11", "00" },
	{	"oldwell",		0,				"bricks",	1,	1,	HP_MED,		SLOWBURN,	"f", "0", ROTATES },
	{	"haypile",		0,				0,			2,	2,	HP_MED,		FASTBURN,	"ffff", "0000", OBSCURES },
	{	"whitepicketfence",	0,			0,			1,	1,	HP_LOW,		BURN,		"1", "0" },
	{	"barrel0",		0,				0,			1,	1,	HP_LOW,		BURN,		"f", "0" },
	{	"barrel1",		0,				0,			1,	1,	HP_LOW,		BURN,		"f", "0", EXPLODES },
	{	"barrel2",		0,				0,			1,	1,	HP_LOW,		FASTBURN,	"f", "0" },
	{	"crate0",		0,				0,			1,	1,	HP_LOW,		BURN,		"f", "0", 0 },
	{	"crate1",		0,				0,			1,	1,	HP_MED,		BURN,		"f", "f", 0 },
	{	"crate2",		0,				0,			1,	1,	HP_LOW,		BURN,		"f", "0", EXPLODES },	
	{	"cannister0",	0,				0,			1,  1,  HP_LOW,		BURN,		"f", "0", EXPLODES },
	{	"cannister1",	0,				0,			1,  1,  HP_LOW,		FASTBURN,	"f", "0" },
	{	"computerdesk",	0,				0,			1,	1,	HP_LOW,		BURN,		"f", "0" },
	{	"desk",			0,				0,			1,	1,	HP_LOW,		BURN,		"f", "0" },
	{	"bookshelf",	0,				0,			2,	1,	HP_LOW,		BURN,		"ff", "ff" },
	{	"cot",			0,				0,			2,	1,	HP_LOW,		BURN,		"ff",	"00" },
	{	"lockers",		0,				0,			4,	1,	HP_LOW,		0,			"ffff", "ffff" },
	{	"basegun0",		0,				0,			4,	2,	INDESTRUCT,	0,			"ffff" "ffff", "0000" "0000", OBSCURES },
	{	"basegun1",		0,				0,			1,	1,	INDESTRUCT,	0,			"f", "0", OBSCURES },

	{	"rock0",		0,				0,			2,  1,	INDESTRUCT, 0,			"ff", "00", OBSCURES },
	{	"rock1",		0,				0,			4,  1,	INDESTRUCT, 0,			"ff0f", "ff0f" },
	{	"arch",			0,				0,			4,	1,	INDESTRUCT,	0,			"f00f", "0000" },	// should block vision, but looks too weird when it renders.

	{	"woodCrnr",		0,				"woodCrnrD",1,	1,	HP_MED,		BURN,		"3", "3" },
	{	"woodDoorCld",	"woodDoorOpn",	0,			1,	1,	HP_MED,		BURN,		"0", "1" },
	{	"woodWall",		0,				0,			1,	1,	HP_MED,		BURN,		"1", "1" },
	{	"woodWallRed",	0,				0,			1,	1,	HP_MED,		BURN,		"1", "1" },
	{	"woodWallWin",	0,				0,			1,	1,	HP_MED,		BURN,		"1", "0" },

	{	"brickCrnr",		0,			"brickCrnrD",1,	1,	HP_MED,		0,			"3", "3" },
	{	"brickDoorCld",	"brickDoorOpn",	0,			1,	1,	HP_MED,		0,			"0", "1" },
	{	"brickWall",		0,			0,			1,	1,	HP_MED,		0,			"1", "1" },
	{	"brickWallWin",	0,				0,			1,	1,	HP_MED,		0,			"1", "0" },

	{	"baseCrnr",		0,				0,			1,	1,	INDESTRUCT,	0,			"3", "3" },
	{	"baseWall",		0,				0,			1,	1,	INDESTRUCT,	0,			"1", "1" },
	{	"baseWall4",	0,				0,			4,	1,	INDESTRUCT,	0,			"1111", "1111" },
	{	"baseDoorCld",	"baseDoorOpn",	0,			1,	1,	INDESTRUCT,	0,			"0", "1" },

	{	"counter_unit",	0,				0,			1,	1,	HP_MED,		BURN,		"f", "0" },
	{	"counter_join",	0,				0,			1,	1,	HP_MED,		BURN,		"f", "0" },
	{	"counter_unit_reg",	0,			0,			1,	1,	HP_MED,		BURN,		"f", "0" },
	{	"shelf_empty",	0,				0,			1,	1,	HP_MED,		BURN,		"f", "0" },
	{	"shelf_0",		0,				"shelf_0D",	1,	1,	HP_MED,		BURN,		"f", "0", ROTATES },
	{	"shelf_1",		0,				"shelf_1D",	1,	1,	HP_MED,		BURN,		"f", "0", ROTATES },
	{	"shelf_2",		0,				"shelf_2D",	1,	1,	HP_MED,		BURN,		"f", "0", ROTATES },
	{	"sacks",		0,				0,			1,	1,	HP_MED,		BURN,		"f", "0" },
	{	"cafetable",	0,				0,			1,	1,	HP_MED,		0,			"f", "0" },
	{	"lamp0",		0,				0,			1,	1,	HP_MED,		0,			"f", "0" },
	{	"gaspump",		0,				"gaspumpD",	1,	1,	HP_MED,		FASTBURN,	"f", "0", EXPLODES },

	{	"pyramid_2",	0,				0,			2,	2,	INDESTRUCT,	0,			"ffff", "0000" },
	{	"pyramid_4",	0,				0,			4,	4,	INDESTRUCT,	0,			"ffff" "ffff" "ffff" "ffff", "0000" "0ff0" "0ff0" "0000" },
	{	"pyramid_8",	0,				0,			8,	8,	INDESTRUCT,	0,			"fff00fff" "ffffffff" "ffffffff" "0ffffff0" "0ffffff0" "ffffffff" "ffffffff" "fff00fff", 
																					"fff00fff" "fff00fff" "fff00fff" "00000000" "00000000" "fff00fff" "fff00fff" "fff00fff" },
	{	"obelisk",		0,				0,			1,	1,	HP_HIGH,	0,			"f", "0", OBSCURES },
	{	"temple",		0,				0,			2,  1,  INDESTRUCT, 0,			"ff", "ff" },

	// model		open			destroyed	cx, cz	hp				material	pather
	{	"ufo_WallOut",	0,				0,			1,	1,	HP_STEEL,	0,			"1", "1" },
	{	"ufo_WallCurve1I", 0,			0,			1,	1,	INDESTRUCT,	0,			"f", "9" }, 
	{	"ufo_WallCurve4", 0,			0,			4,	4,	INDESTRUCT,	0,			"0002" "0003" "0030" "1300",	// pather
																					"0002" "0003" "0030" "1300" },	// visibility
	{	"ufo_WallCurve8", 0,			0,			8,	8,	INDESTRUCT,	0,			"00000002" "00000002" "0000000f" "0000000f" "000000f0" "00000f00" "0000f000" "11ff0000",
																					"00000002" "00000002" "00000003" "00000008" "00000030" "00000300" "00003000" "11340000" },
	{	"ufo_DoorCld",	"ufo_DoorOpn",	0,			1,	1,	HP_STEEL,	0,			"0", "1" },
	{	"ufo_WallInn",	0,				0,			1,	1,	HP_STEEL,	0,			"1", "1" },
	{	"ufo_CrnrInn",	0,				0,			1,	1,	HP_STEEL,	0,			"3", "3" },
	{	"ufo_controlTable",	0,			0,			1,	1,	HP_HIGH,	0,			"f", "0" },
	{	"ufo_controlScreen",0,			0,			1,	1,	HP_LOW,		0,			"f", "0" },
	{	"ufo_power",	0,				"ufo_powerD",1,	1,	HP_HIGH,	FASTBURN,	"f", "0", EXPLODES | OBSCURES | ROTATES },
	{	"ufo_statue0",	0,				0,			1,	1,	HP_HIGH,	0,			"f", "0", OBSCURES },
	{	"ufo_crate0",	0,				0,			1,	1,	HP_MED,		0,			"f", "0", 0 },
	{	"ufo_crate1",	0,				0,			1,	1,	HP_MED,		0,			"f", "f", 0 },
	{	"ufo_crate2",	0,				0,			1,	1,	HP_LOW,		0,			"f", "0", EXPLODES },
	{	"ufo_tube",		0,				"ufo_tubeD",1,	1,	HP_MED,		BURN,		"f", "0", EXPLODES | OBSCURES | ROTATES },
	{	"ufo_table",	0,				0,			2,	1,	HP_MED,		SLOWBURN,	"ff","00", 0 },
	{	"ufo_pod",		0,				0,			1,	1,	HP_MED,		BURN,		"f", "0", OBSCURES },
	{	"ufo_plant0",	0,				0,			1,	1,	HP_LOW,		BURN,		"f", "0", OBSCURES },
	{	"ufo_plant1",	0,				0,			1,	1,	HP_LOW,		BURN,		"f", "0", OBSCURES },
	{	"ufo_drone",	0,				0,			4,	4,	HP_STEEL,	0,			"ffff" "ffff" "ffff" "ffff", "0000" "0ff0" "0ff0" "0000", OBSCURES },

		// model		open			destroyed	cx, cz	hp			material	pather
	{	"lander",		0,				0,			6,	6,	INDESTRUCT,	0,			"00ff00" "00ff00" "ff00ff" "ff00ff" "ff00ff" "ff00ff",
																					"00ff00" "00ff00" "0f00f0" "0f00f0" "0f00f0" "0f00f0", 
																					0 },
	{	"guard",		0,				0,			1,  1,  INDESTRUCT, 0,			"0", "0" },
	{	"scout",		0,				0,			1,  1,  INDESTRUCT, 0,			"0", "0" },
	{	"pawn",			0,				0,			1,  1,  INDESTRUCT, 0,			"0", "0" },
};


TacMap::TacMap( SpaceTree* tree, const ItemDefArr& _itemDefArr ) : Map( tree ),
	gameItemDefArr( _itemDefArr )
{
	lander = 0;
	nLanderPos = 0;

	gamui::RenderAtom borderAtom = Game::CalcPaletteAtom( Game::PALETTE_BLUE, Game::PALETTE_BLUE, Game::PALETTE_DARK, true );
#ifdef DEBUG_VISIBILITY
	borderAtom.renderState = (const void*) RENDERSTATE_MAP_TRANSLUCENT;
#else
	borderAtom.renderState = (const void*) RENDERSTATE_MAP_OPAQUE;
#endif
	for( int i=0; i<4; ++i ) {
		border[i].Init( &overlay[LAYER_UNDER_HIGH], borderAtom, false );
	}
}


TacMap::~TacMap()
{
	for( int i=0; i<debris.Size(); ++i ) {
		delete debris[i].storage;
		if ( debris[i].crate )
			tree->FreeModel( debris[i].crate );
	}
	debris.Clear();
}


void TacMap::SetSize( int w, int h )					
{
	Map::SetSize( w, h );

	border[0].SetPos( -1, -1 );
	border[0].SetSize( (float)(w+2), 1 );

	border[1].SetPos( -1, (float)h );
	border[1].SetSize( (float)(w+2), 1 );

	border[2].SetPos( -1, 0 );
	border[2].SetSize( 1, (float)h );

	border[3].SetPos( (float)w, 0 );
	border[3].SetSize( 1, (float)h );
}


void TacMap::InitWalkingMapAtoms( gamui::RenderAtom* atom, int nWalkingMaps )
{
	atom[0] = Game::CalcIconAtom( ICON_GREEN_WALK_MARK );
	atom[1] = Game::CalcIconAtom( ICON_YELLOW_WALK_MARK ); 
	atom[2] = Game::CalcIconAtom( ICON_ORANGE_WALK_MARK ); 
	atom[3] = Game::CalcIconAtom( ICON_GREEN_WALK_MARK ); 
	atom[4] = Game::CalcIconAtom( ICON_YELLOW_WALK_MARK ); 
	atom[5] = Game::CalcIconAtom( ICON_ORANGE_WALK_MARK ); 

	if ( nWalkingMaps == 1 ) {
		atom[0].renderState = (const void*) RENDERSTATE_MAP_TRANSLUCENT;
		atom[1].renderState = (const void*) RENDERSTATE_MAP_TRANSLUCENT;
		atom[2].renderState = (const void*) RENDERSTATE_MAP_TRANSLUCENT;
	}
	else {
		atom[0].renderState = (const void*) RENDERSTATE_MAP_MORE_TRANSLUCENT;
		atom[1].renderState = (const void*) RENDERSTATE_MAP_MORE_TRANSLUCENT;
		atom[2].renderState = (const void*) RENDERSTATE_MAP_MORE_TRANSLUCENT;
		atom[3].renderState = (const void*) RENDERSTATE_MAP_MORE_TRANSLUCENT;
		atom[4].renderState = (const void*) RENDERSTATE_MAP_MORE_TRANSLUCENT;
		atom[5].renderState = (const void*) RENDERSTATE_MAP_MORE_TRANSLUCENT;
	}

}


const char* TacMap::GetItemDefName( int i )
{
	const char* result = "";
	if ( i >= 0 && i < NUM_ITEM_DEF ) {
		result = itemDefArr[i].Name();
	}
	return result;
}


const MapItemDef* TacMap::GetItemDef( const char* name )
{
	if ( itemDefMap.Empty() ) {
		for( int i=0; i<NUM_ITEM_DEF; ++i ) {
			if ( itemDefArr[i].Name() ) {
				itemDefMap.Add( itemDefArr[i].Name(), itemDefArr+i );
			}
		}
	}
	const MapItemDef* item=0;
	itemDefMap.Query( name, &item );
	return item;
}


const MapItemDef* TacMap::StaticGetItemDef( const char* name )
{
	for( int i=0; i<NUM_ITEM_DEF; ++i ) {
		if ( StrEqual( itemDefArr[i].Name(), name ) ) {
			return &itemDefArr[i];
		}
	}
	return 0;
}


const Map::MapItem* TacMap::FindLander()
{
	Rectangle2I bounds = Bounds();

	if ( !lander ) {
		for( MapItem* root = quadTree.FindItems( bounds, 0, 0 ); root; root = root->next ) {
			if ( StrEqual( root->def->Name(), "lander" ) ) {
				lander = root;
				break;
			}
		}
	}
	return lander;
}


const Model* TacMap::GetLanderModel()
{
	FindLander();
	return ( lander ) ? lander->model : 0;
}


void TacMap::PopLocation( int team, bool guard, grinliz::Vector2I* pos, float* rotation )
{
	*rotation = 0;

	if ( team == TERRAN_TEAM ) {
		// put 'em in the lander.
		FindLander();
		Matrix2I xform = lander->XForm();

		Vector2I obj = { 2 + (nLanderPos & 1), 5 - (nLanderPos / 2) };
		Vector2I world = xform * obj;
		
		*pos = world;
		*rotation = lander->ModelRot();
		++nLanderPos;
	}
	else if ( team == ALIEN_TEAM ) {
		bool found = false;

		if ( guard && !guardPos.Empty() ) {
			int i = random.Rand( guardPos.Size() );
			*pos = guardPos[i];
			guardPos.SwapRemove( i );
			found = true;
		}
		if ( !found ) { // or scout
			GLRELASSERT( !scoutPos.Empty() );
			int i = random.Rand( scoutPos.Size() );
			*pos = scoutPos[i];
			scoutPos.SwapRemove( i );

			// FIXME: the scout should always succeed. Need to make sure this can never
			// fail. Each tile needs a minimum scout positions.
		}
	}
	else if ( team == CIV_TEAM ) {
		GLASSERT( !civPos.Empty() );
		int i = random.Rand( civPos.Size() );
		*pos = civPos[i];
		civPos.SwapRemove( i );
	}
	else {
		GLRELASSERT( 0 );
		pos->Set( 0, 0 );
	}
}



void TacMap::SaveDebris( const Debris& d, FILE* fp, int depth )
{
	XMLUtil::OpenElement( fp, depth, "Debris" );
	XMLUtil::Attribute( fp, "x", d.storage->X() );
	XMLUtil::Attribute( fp, "y", d.storage->Y() );
	XMLUtil::SealElement( fp );

	d.storage->Save( fp, depth+1 );

	XMLUtil::CloseElement( fp, depth, "Debris" );
}


void TacMap::LoadDebris( const TiXmlElement* debrisElement )
{
	GLASSERT( StrEqual( debrisElement->Value(), "Debris" ) );
	GLRELASSERT( debrisElement );
	if ( debrisElement ) {
		int x=0, y=0;

		debrisElement->QueryIntAttribute( "x", &x );
		debrisElement->QueryIntAttribute( "y", &y );

		Storage* storage = LockStorage( x, y );
		storage->Load( debrisElement );
		ReleaseStorage( storage );
	}
}


void TacMap::SubSave( FILE* fp, int depth )
{
	XMLUtil::OpenElement( fp, depth+1, "Items" );
	XMLUtil::SealElement( fp );

	Rectangle2I b = Bounds();
	MapItem* item = quadTree.FindItems( b, 0, MapItem::MI_NOT_IN_DATABASE );

	for( ; item; item=item->next ) {
		XMLUtil::OpenElement( fp, depth+2, "Item" );

		int x, y, r;
		WorldToXYR( item->XForm(), &x, &y, &r, true );

		XMLUtil::Attribute( fp, "x", x );
		XMLUtil::Attribute( fp, "y", y );

		if ( r != 0 )
			XMLUtil::Attribute( fp, "rot", r );
		XMLUtil::Attribute( fp, "name", item->def->Name() );

		if ( item->hp != item->def->hp )
			XMLUtil::Attribute( fp, "hp", item->hp );
		if ( item->flags )
			XMLUtil::Attribute( fp, "flags", item->flags );
		XMLUtil::SealCloseElement( fp );
	}
	XMLUtil::CloseElement( fp, depth+1, "Items" );


		XMLUtil::OpenElement( fp, depth+1, "GroundDebris" );
	XMLUtil::SealElement( fp );
	for( int i=0; i<debris.Size(); ++i ) {
		SaveDebris( debris[i], fp, depth+2 );
	}
	XMLUtil::CloseElement( fp, depth+1, "GroundDebris" );
}


void TacMap::SubLoad( const TiXmlElement* mapElement )
{
	const TiXmlElement* itemsElement = mapElement->FirstChildElement( "Items" );
	if ( itemsElement ) {
		for(	const TiXmlElement* item = itemsElement->FirstChildElement( "Item" );
				item;
				item = item->NextSiblingElement( "Item" ) )
		{
			int x=0;
			int y=0;
			int rot = 0;
			int index = -1;
			int hp = 0xffff;
			int flags = 0;

			item->QueryIntAttribute( "x", &x );
			item->QueryIntAttribute( "y", &y );
			item->QueryIntAttribute( "rot", &rot );
			item->QueryIntAttribute( "flags", &flags );

			if ( item->QueryIntAttribute( "index", &index ) != TIXML_NO_ATTRIBUTE ) {
				// good to go - have the deprecated 'index' value.
			}
			else if ( item->Attribute( "name" ) ) {
				const char* name = item->Attribute( "name" );
				const MapItemDef* mid = GetItemDef( name );
				if ( mid ) {
					index = mid - itemDefArr;
				} 
				else {
					GLOUTPUT(( "Could not load item '%s'\n", name ));
				}
			}

			if ( index >= 0 ) {
				// Use the default hp if not provided.
				hp = itemDefArr[index].hp;
				item->QueryIntAttribute( "hp", &hp );

				AddItem( x, y, rot, &itemDefArr[index], hp, flags );
			}
		}
	}

	const TiXmlElement* groundDebrisElement = mapElement->FirstChildElement( "GroundDebris" );
	if ( groundDebrisElement ) {
		for( const TiXmlElement* debrisElement = groundDebrisElement->FirstChildElement( "Debris" );
			 debrisElement;
			 debrisElement = debrisElement->NextSiblingElement( "Debris" ) )
		{
			LoadDebris( debrisElement );
		}
	}

}


const Storage* TacMap::GetStorage( int x, int y ) const
{
	for( int i=0; i<debris.Size(); ++i ) {
		if ( debris[i].storage->X() == x && debris[i].storage->Y() ==y ) {
			return debris[i].storage;
		}
	}
	return 0;
}


grinliz::Vector2I TacMap::FindStorage( const ItemDef* itemDef, const Vector2I& pos )
{
	Vector2I loc = { -1, -1 };
	int close2 = INT_MAX;

	// [Sun, 6:19 pm 	  	Exception version=470 device=passion]
	// Wasn't handling itemDef being null (no weapon/weapon destroyed)

	// 1st pass: look for re-supply.
	for( int i=0; i<debris.Size(); ++i ) {
		if ( debris[i].storage->IsResupply( itemDef ? itemDef->IsWeapon() : 0 ) ) {

			Vector2I storeLoc = { debris[i].storage->X(), debris[i].storage->Y() };
			int dist2 = ( storeLoc - pos ).LengthSquared();
			if ( dist2 < close2 ) {
				loc = storeLoc;
				close2 = dist2;
			}
		}
	}
	return loc;
}


Storage* TacMap::LockStorage( int x, int y )
{
	Storage* storage = 0;
	for( int i=0; i<debris.Size(); ++i ) {
		if ( debris[i].storage->X() == x && debris[i].storage->Y() == y ) {
			storage = debris[i].storage;
			tree->FreeModel( debris[i].crate );
			debris[i].crate = 0;
			break;
		}
	}
	if ( !storage ) {
		storage = new Storage( x, y, gameItemDefArr );
		Debris d;
		d.crate = 0;
		d.storage = storage;
		debris.Push( d );
	}
	return storage;
}


void TacMap::ReleaseStorage( Storage* storage )
{
	int index = -1;
	for( int i=0; i<debris.Size(); ++i ) {
		if ( debris[i].storage == storage ) {
			index = i;
			break;
		}
	}
	GLRELASSERT( index >= 0 );	// should always be found...

	storage->ClearHidden();
	if ( storage->Empty() ) {
		delete storage;
		GLRELASSERT( debris[index].crate == 0 );
		debris.SwapRemove( index );
		return;
	}

	bool zRotate = false;
	const ModelResource* res = storage->VisualRep( &zRotate );

	Model* model = tree->AllocModel( res );
	Vector2I v = { storage->X(), storage->Y() };
	if ( zRotate ) {
		model->SetRotation( 90.0f, 2 );
		
		int yRot = Random::Hash( &v, sizeof(v) ) % 360;	// generate a random yet consistent rotation

		model->SetRotation( (float)yRot, 1 );
		model->SetPos( (float)v.x+0.5f, 0.05f, (float)v.y+0.5f );
	}
	else {
		model->SetPos( (float)v.x+0.5f, 0.0f, (float)v.y+0.5f );
	}

	// Don't set: model->SetFlag( Model::MODEL_OWNED_BY_MAP );	not really owned by map, in the sense of mapBounds, etc.
	debris[index].crate = model;
}


Storage* TacMap::CollectAllStorage()
{
	for( int i=1; i<debris.Size(); ++i ) {
		const Debris& d = debris[i];
		const Storage* s = d.storage;

		debris[0].storage->AddStorage( *s );
	}
	while( debris.Size() > 1 ) {
		delete debris[1].storage;
		if ( debris[1].crate ) {
			tree->FreeModel( debris[1].crate );
		}
		debris.SwapRemove( 1 );
	}
	if ( debris.Empty() ) {
		Storage* storage = new Storage( 0, 0, gameItemDefArr );
		Debris d;
		d.crate = 0;
		d.storage = storage;
		debris.Push( d );
	}
	return debris[0].storage;
}

