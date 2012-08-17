#include "geoscene.h"
#include "game.h"
#include "cgame.h"
#include "areaWidget.h"
#include "geomap.h"
#include "geoai.h"
#include "geoendscene.h"
#include "chits.h"
#include "basetradescene.h"
#include "characterscene.h"
#include "buildbasescene.h"
#include "fastbattlescene.h"
#include "tacticalintroscene.h"
#include "tacticalendscene.h"
#include "researchscene.h"
#include "gamesettings.h"
#include "battlescene.h"
#include "helpscene.h"
#include "ufosound.h"

#include "../engine/loosequadtree.h"
#include "../engine/particle.h"
#include "../engine/particleeffect.h"
#include "../engine/ufoutil.h"
#include "../engine/text.h"

#include "../grinliz/glutil.h"

using namespace grinliz;
using namespace gamui;
using namespace tinyxml2;

// t: tundra
// a: farm
// d: desert
// f: forest
// c: city


static const char* MAP =
	"w. 0t 0t 0t 0t 0t 0t w. w. 2c 2t 2t 2t 2t 2t 2t 2t 2t 2t 2t "
	"w. 0t 0t 0t 0t 0t 0t 2c 2c 2a 2a 2t 2t 2t 2t 2t 2t 2t w. w. "
	"w. 0c 0d 0a 0C w. w. 2C 2a 2c 2a 2d 3d 2a 3a 3a 3a 3a w. w. "
	"w. 0c 0d 0a w. w. w. 4c 4d 4d 4d 3d 3d 3a 3a 3a 3a 3c w. w. "
	"w. w. 0c w. 0f w. w. 4f 4f 4d 4C 3d 3c w. 3f w. 3C w. w. w. "
	"w. w. w. 0f 1c w. w. 4f 4f 4f 4d 4f w. w. 3c w. w. w. w. w. "
	"w. w. w. w. 1f 1f 1f w. w. 4f 4f w. w. w. w. w. 3f 3f w. w. "
	"w. w. w. w. 1f 1a 1C w. w. 4f 4f w. w. w. w. w. w. w. 5C w. "
	"w. w. w. w. w. 1f 1f w. w. 4f 4f 4f w. w. w. w. w. 5f 5d 5a "
	"w. w. w. w. w. 1f w. w. w. 4c w. w. w. w. w. w. w. 5f 5d 5c "
;


static const char* gRegionName[GEO_REGIONS] = { "North", "South", "Europe", "Asia", "Africa", "Under" };


grinliz::Vector2I GeoMapData::Capital( int region ) const
{
	grinliz::Vector2I city = { 0, 0 };
	for( int i=0; i<numCities[region]; ++i ) {
		city = City( region, i );
		if ( MAP[city.y*GEO_MAP_X*3 + city.x*3 + 1] == 'C' ) {
			break;
		}
	}
	return city;
}


void GeoMapData::Init( const char* map, Random* random )
{
	numLand = 0;
	for( int i=0; i<GEO_REGIONS; ++i ) {
		bounds[i].SetInvalid();
		numCities[i] = 0;
	}
	const char* p = MAP;
	for( int j=0; j<GEO_MAP_Y; ++j ) {
		for( int i=0; i<GEO_MAP_X; ++i, p += 3 ) {
			if ( *p == 'w' ) {
				data[j*GEO_MAP_X+i] = 0;
			}
			else {
				int region = *p - '0';
				int type = 0;
				switch( *(p+1) ) {
					case 't':	type = TUNDRA;		break;
					case 'a':	type = FARM;		break;
					case 'd':	type = DESERT;		break;
					case 'f':	type = FOREST;		break;
					case 'C':	type = CITY;		break;	// the capital is a city
					case 'c':	type = CITY;		break;
					default: GLASSERT( 0 ); 
				}
				data[j*GEO_MAP_X+i] = type | (region<<8);
				bounds[region].DoUnion( i, j );
				++numLand;
			}
		}
	}

	for( int i=0; i<GEO_REGIONS; ++i ) {
		numCities[i] = Find( &city[i*MAX_CITIES_PER_REGION], MAX_CITIES_PER_REGION, i, CITY, 0 );
	}
}


int GeoMapData::Find( U8* choiceBuffer, int bufSize, int region, int required, int excluded ) const
{
	int count=0;
	for( int j=bounds[region].min.y; j<=bounds[region].max.y; ++j ) {
		for( int i=bounds[region].min.x; i<=bounds[region].max.x; ++i ) {
			int type   = Type( data[j*GEO_MAP_X+i] );
			int r      = Region( data[j*GEO_MAP_X+i] );

			if (	type 
				 && r == region
				 &&	( type & required ) == required 
				 && ( type & excluded ) == 0 ) 
			{
				GLASSERT( count < bufSize );
				if ( count == bufSize )
					return count;

				choiceBuffer[count] = j*GEO_MAP_X+i;
				++count;
			}
		}
	}
	return count;
}


void GeoMapData::Choose( grinliz::Random* random, int region, int required, int excluded, Vector2I* pos ) const 
{
	U8 buffer[GEO_MAP_X*GEO_MAP_Y];

	int count = Find( buffer, GEO_MAP_X*GEO_MAP_Y, region, required, excluded );
	int choice = buffer[ random->Rand( count ) ];
	GLASSERT( count > 0 );

	pos->y = choice / GEO_MAP_X;
	pos->x = choice - pos->y*GEO_MAP_X;
}


void RegionData::Init(  const ItemDefArr& itemDefArr, Random* random )
{ 
	traits = ( 1 << random->Rand( TRAIT_NUM_BITS ) ) | ( 1 << random->Rand( TRAIT_NUM_BITS ) );
	influence=0; 
	for( int i=0; i<HISTORY; ++i ) 
		history[i] = 1; 
}


// Sets up storage, and whether items are available due to
// region, research, etc.
void RegionData::SetStorageNormal( const Research& research, Storage* storage, bool tech, bool manufacture )
{
	int COUNT = 100;

	storage->Clear();
	for( int i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		const ItemDef* itemDef = storage->GetItemDef( i );
		if ( itemDef ) {
			if ( itemDef->IsAlien() ) {
				// do nothing. can not manufacture.
			}
			else {
				// terran items.
				int status = research.GetStatus( itemDef->name );
				if ( status == Research::TECH_FREE ) {
					if ( itemDef->price >= 0 )
						storage->AddItem( itemDef, COUNT );
				}
				else if ( status == Research::TECH_RESEARCH_COMPLETE ) {
					if ( itemDef->IsWeapon() )
					{
						storage->AddItem( itemDef, COUNT );
					}
					else if ( itemDef->IsArmor() )
					{
						storage->AddItem( itemDef, COUNT );
					}
					else {
						storage->AddItem( itemDef, COUNT );
					}
				}
			}
		}
	}
}

void RegionData::Free()
{
}


void RegionData::Save( XMLPrinter* printer )
{
	printer->OpenElement( "Region" );
	printer->PushAttribute( "traits", traits );
	printer->PushAttribute( "influence", influence );

	for( int i=0; i<HISTORY; ++i ) {
		printer->OpenElement( "History" );
		printer->PushAttribute( "value", history[i] );
		printer->CloseElement();
	}
	printer->CloseElement();
}


void RegionData::Load( const XMLElement* doc )
{
	GLASSERT( StrEqual( doc->Value(), "Region" ));
	doc->QueryIntAttribute( "traits", &traits );
	doc->QueryFloatAttribute( "influence", &influence );

	int count=0;
	for( const XMLElement* historyEle = doc->FirstChildElement( "History" ); 
		 historyEle && count<HISTORY; 
		 historyEle=historyEle->NextSiblingElement( "History" ), ++count ) 
	{
		historyEle->QueryIntAttribute( "value", &history[count] );
	}
}



GeoScene::GeoScene( Game* _game, const GeoSceneData* data ) : Scene( _game ), research( _game->GetDatabase(), _game->GetItemDefArr(), RESEARCH_SECONDS )
{
	missileTimer[0] = 0;
	missileTimer[1] = 0;
	contextChitID = 0;
	cash = 1000;
	firstBase = true;
	timeline = 0;
	nBattles = 0;
	gameVictory = false;
	loadSlot = 0;
	difficulty = NORMAL;
	if ( data ) {
		difficulty = data->difficulty;
	}

	//const Screenport& port = GetEngine()->GetScreenport();
	random.SetSeedFromTime();

	geoMapData.Init( MAP, &random );
	for( int i=0; i<GEO_REGIONS; ++i ) {
		regionData[i].Init( game->GetItemDefArr(), &random );
	}

	geoMap = new GeoMap( GetEngine()->GetSpaceTree() );
	tree = GetEngine()->GetSpaceTree();
	geoAI = new GeoAI( geoMapData, this );

	alienTimer = 0;
	researchTimer = 0;

	for( int i=0; i<GEO_REGIONS; ++i ) {
		areaWidget[i] = new AreaWidget( _game, &gamui3D, gRegionName[i] );
		areaWidget[i]->SetTraits( regionData[i].traits );
	}

	helpButton.Init(&gamui2D, game->GetButtonLook( Game::GREEN_BUTTON ) );
	helpButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	helpButton.SetDeco(  Game::CalcDecoAtom( DECO_HELP, true ), Game::CalcDecoAtom( DECO_HELP, false ) );	

	researchButton.Init(&gamui2D, game->GetButtonLook( Game::GREEN_BUTTON ) );
	researchButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	researchButton.SetDeco(  Game::CalcDecoAtom( DECO_RESEARCH, true ), Game::CalcDecoAtom( DECO_RESEARCH, false ) );	

	baseButton.Init(&gamui2D, game->GetButtonLook( Game::GREEN_BUTTON ) );
	baseButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	baseButton.SetDeco(  Game::CalcDecoAtom( DECO_BASE, true ), Game::CalcDecoAtom( DECO_BASE, false ) );

	//cashImage.Init( &gamui2D, UIRenderer::CalcIconAtom( ICON_GREEN_STAND_MARK ), false );
	cashImage.Init( &gamui2D, 
		Game::CalcPaletteAtom( Game::PALETTE_GREEN, Game::PALETTE_GREEN, Game::PALETTE_NORM ),
		false );
	cashImage.SetSize( GAME_BUTTON_SIZE_F*1.7f, GAME_BUTTON_SIZE_F );
	cashImage.SetSlice( true );

	cashLabel.Init( &gamui2D );

	for( int i=0; i<MAX_CONTEXT; ++i ) {
		context[i].Init( &gamui2D, game->GetButtonLook( Game::BLUE_BUTTON ) );
		context[i].SetVisible( false );
		context[i].SetSize( GAME_BUTTON_SIZE_F*2.0f, GAME_BUTTON_SIZE_F );
	}
	GenerateCities();
	savedCameraX = -1.0f;
}


GeoScene::~GeoScene()
{
	chitBag.Clear();
	for( int i=0; i<GEO_REGIONS; ++i ) {
		delete areaWidget[i];
	}
	for( int i=0; i<GEO_REGIONS; ++i ) {
		regionData[i].Free();
	}
	delete geoMap;
	delete geoAI;
}


void GeoScene::Resize()
{
	const Screenport& port = GetEngine()->GetScreenport();

	helpButton.SetPos( port.UIWidth()-GAME_BUTTON_SIZE_F, 0 );
	researchButton.SetPos( 0, 0 );
	baseButton.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	cashImage.SetPos( port.UIWidth()-GAME_BUTTON_SIZE_F*1.8f, 
					  port.UIHeight()-GAME_BUTTON_SIZE_F*0.4f );
	cashLabel.SetPos( cashImage.X()+5.0f, cashImage.Y()+5.0f );

	GetEngine()->CameraIso( false, false, (float)GeoMap::MAP_X, (float)GeoMap::MAP_Y );
	if ( savedCameraX >= 0 ) {
		Vector3F cameraPos = GetEngine()->camera.PosWC();
		cameraPos.x = savedCameraX;
		GetEngine()->camera.SetPosWC( cameraPos );
	}
	SetMapLocation();
}


void GeoScene::Activate()
{
	GetEngine()->CameraIso( false, false, (float)GeoMap::MAP_X, (float)GeoMap::MAP_Y );
	if ( savedCameraX >= 0 ) {
		Vector3F cameraPos = GetEngine()->camera.PosWC();
		cameraPos.x = savedCameraX;
		GetEngine()->camera.SetPosWC( cameraPos );
	}
	SetMapLocation();
	GetEngine()->SetIMap( geoMap );
	SetMapLocation();

	GetEngine()->GetSpaceTree()->ShelveAll( false );

	Vector3F light = { 1, 1, -1 };
	GetEngine()->SetLightDirection( &light );
}


void GeoScene::DeActivate()
{
	savedCameraX = GetEngine()->camera.PosWC().x;
	GetEngine()->CameraIso( true, false, 0, 0 );
	GetEngine()->SetIMap( 0 );

	GetEngine()->GetSpaceTree()->ShelveAll( true );
}


void GeoScene::GenerateCities()
{	
	for( Chit* it=chitBag.Begin(); it!=chitBag.End(); it=it->Next() ) {
		if ( it->IsCityChit() ) 
			it->SetDestroyed();
	}
	chitBag.Clean();

	for( int region=0; region<GEO_REGIONS; ++region ) {
		for( int i=0, n=geoMapData.NumCities( region ); i<n; ++i ) {
			Vector2I pos = geoMapData.City( region, i );
			CityChit* chit = new CityChit( GetEngine()->GetSpaceTree(), pos, pos==geoMapData.Capital(region) );
			chitBag.Add( chit );
		}
	}
}


#ifndef IMMEDIATE_BUY
bool GeoScene::CanSendCargoPlane( const Vector2I& pos )
{
	BaseChit* base = chitBag.GetBaseChitAt ( pos );
	if ( base && base->IsFacilityComplete( BaseChit::FACILITY_CARGO ) ) {
		
		CargoChit* cargoChit = chitBag.GetCargoGoingTo( CargoChit::TYPE_CARGO, pos );
		// Make sure no plane is deployed:
		if ( !cargoChit ) {
			int region = geoMapData.GetRegion( pos.x, pos.y );
			if ( !regionData[region].occupied ) {
				return true;
			}
		}
	}
	return false;
}
#endif


void GeoScene::HandleItemTapped( const gamui::UIItem* item )
{
	Chit* contextChit = chitBag.GetChit( contextChitID );

	if ( item == &context[CONTEXT_CARGO] && contextChit && contextChit->IsBaseChit() ) {
		Vector2I mapi = contextChit->MapPos();
		
#ifdef IMMEDIATE_BUY
		if ( contextChit->IsBaseChit()->IsFacilityComplete( BaseChit::FACILITY_CARGO ) ) {
			if ( !game->IsScenePushed() ) {
				PushBaseTradeScene( contextChit->IsBaseChit() );
			}
		}
#else
		if ( CanSendCargoPlane( mapi) ) {
			int region = geoMapData.GetRegion( mapi.x, mapi.y );
			int cityID = random.Rand( geoMapData.NumCities( region ) );
			Vector2I city = geoMapData.City( region, cityID );

			Chit* chit = new CargoChit( tree, CargoChit::TYPE_CARGO, city, contextChit->MapPos() );
			chitBag.Add( chit );
		}
#endif
	}
	else if ( item == &context[CONTEXT_EQUIP] && contextChit && contextChit->IsBaseChit() ) {
		BaseChit* baseChit = contextChit->IsBaseChit();

		if ( !game->IsScenePushed() ) {
			CharacterSceneData* input = new CharacterSceneData();
			input->unit = baseChit->GetUnits();
			input->nUnits = baseChit->NumUnits();
			input->storage = baseChit->GetStorage();
			game->PushScene( Game::CHARACTER_SCENE, input );
		}
	}
	else if ( item == &context[CONTEXT_BUILD] && contextChit && contextChit->IsBaseChit() ) {
		BaseChit* baseChit = contextChit->IsBaseChit();

		if ( !game->IsScenePushed() ) {
			BuildBaseSceneData* data = new BuildBaseSceneData();
			data->baseChit = baseChit;
			data->cash = &cash;

			game->PushScene( Game::BUILDBASE_SCENE, data );
		}
	}
	else if (    ( item == &context[0] || item == &context[1] || item == &context[2] || item == &context[3] )
		      && contextChit
			  && contextChit->IsUFOChit() )
	{
		PushButton* pushButton = (PushButton*)item;
		const char* label = pushButton->GetText();

		BaseChit* base = chitBag.GetBaseChit( label );
		if ( base ) {
			if ( chitBag.GetCargoGoingTo( CargoChit::TYPE_LANDER, contextChit->MapPos() ) ) {
				GLASSERT( 0 );
			}
			else {
				CargoChit* cargoChit = new CargoChit( tree, CargoChit::TYPE_LANDER, base->MapPos(), contextChit->MapPos() );
				chitBag.Add( cargoChit );
			}
		}
	}
	else if ( item == &researchButton ) {
		if ( !game->IsScenePushed() ) {
			ResearchSceneData* data = new ResearchSceneData();
			data->research = &research;
			game->PushScene( Game::RESEARCH_SCENE, data );
		}
	}
	else if ( item == &helpButton ) {
		if ( !game->IsScenePushed() ) {
			game->PushScene( Game::HELP_SCENE, new HelpSceneData( "geoHelp", true ) );
		}
	}
}


void GeoScene::Tap(	int action, 
					const grinliz::Vector2F& view,
					const grinliz::Ray& world )
{
	Vector2F ui;
	const Screenport& port = GetEngine()->GetScreenport();
	port.ViewToUI( view, &ui );
	const UIItem* itemTapped = 0;

	const Vector3F& cameraPos = GetEngine()->camera.PosWC();
	float d = cameraPos.y * tanf( ToRadian( EL_FOV*0.5f ) );
	float widthInMap = d*2.0f;
	Vector3F mapTap = { -1, -1, -1 };

	if ( action == GAME_TAP_DOWN || action == GAME_TAP_DOWN_PANNING ) {
		if ( action == GAME_TAP_DOWN ) {
			// First check buttons.
			gamui2D.TapDown( ui.x, ui.y );
		}

		dragStart.Set( -1, -1 );
		dragLast.Set( -1, -1 );

		if ( !gamui2D.TapCaptured() ) {
			dragStart = ui;
			dragLast = ui;
		}
	}
	else if ( action == GAME_TAP_MOVE || action == GAME_TAP_MOVE_PANNING ) {
		if ( dragLast.x >= 0 ) {
			GetEngine()->camera.DeltaPosWC( -(ui.x - dragLast.x)*widthInMap/port.UIWidth(), 0, 0 );
			dragLast = ui;
			SetMapLocation();
		}
	}
	else if ( action == GAME_TAP_UP || action == GAME_TAP_UP_PANNING ) {
		if ( dragLast.x >= 0 ) {
			GetEngine()->camera.DeltaPosWC( (ui.x - dragLast.x)*widthInMap/port.UIWidth(), 0, 0 );
			dragLast = ui;
			SetMapLocation();
		}
		
		if ( gamui2D.TapCaptured() ) {
			itemTapped = gamui2D.TapUp( ui.x, ui.y );

			HandleItemTapped( itemTapped );
			// Whatever it was, on this path, closes the context menu.
			InitContextMenu( CM_NONE, 0 );
		}
		else if ( (dragLast-dragStart).Length() < GAME_BUTTON_SIZE_F / 2.0f ) {
			Matrix4 mvpi;
			Ray ray;

			port.ViewProjectionInverse3D( &mvpi );
			GetEngine()->RayFromViewToYPlane( view, mvpi, &ray, &mapTap );

			// normalize
			if ( mapTap.x < 0 ) mapTap.x += (float)GEO_MAP_X;
			if ( mapTap.x >= (float)GEO_MAP_X ) mapTap.x -= (float)GEO_MAP_X;
			if ( mapTap.x >= 0 && mapTap.x < (float)GEO_MAP_X && mapTap.z >= 0 && mapTap.z < (float)GEO_MAP_Y ) {
				// all is well.
			}
			else {
				mapTap.Set( -1, -1, -1 );
			}
		}
	}
	else if ( action == GAME_TAP_CANCEL || action == GAME_TAP_CANCEL_PANNING ) {
		gamui2D.TapUp( ui.x, ui.y );
		dragLast.Set( -1, -1 );
		dragStart.Set( -1, -1 );
	}


	if ( mapTap.x >= 0 ) {
		Vector2I mapi = { (int)mapTap.x, (int)mapTap.z };

		// Are we placing a base?
		// Checks should be rendundant to button being enabled,
		// but be sure. The order of events in this code is subtle.
		if (    baseButton.Down()
			 && chitBag.NumBaseChits() < MAX_BASES
			 && cash >= BASE_COST[ chitBag.NumBaseChits() ] ) 
		{
			if ( PlaceBase( mapi ) ) {
				cash -= BASE_COST[ chitBag.NumBaseChits()-1 ];
			}			
		}
		else {
			// Search for what was clicked on. Go a little
			// fuzzy to account for the small size of the 
			// map tiles on a phone screen.
			Vector2F mapTap2 = { mapTap.x, mapTap.z };
			float bestRadius = 1.5f;	// in map units
			Chit* bestChit = 0;

			for( Chit* it = chitBag.Begin(); it != chitBag.End(); it = it->Next() ) {
				if (    it->IsBaseChit() 
					 || ( it->IsUFOChit() && it->IsUFOChit()->CanSendLander( research.GetStatus( "BattleshipAttack" ) == Research::TECH_RESEARCH_COMPLETE ) ) )
				{
					float len = Cylinder<float>::Length( it->Pos(), mapTap2 );
					if ( len < bestRadius ) {
						bestRadius = len;
						bestChit = it;
					}
				}
			}

			if ( bestChit && bestChit->IsBaseChit() ) {
				InitContextMenu( CM_BASE, bestChit );
				UpdateContextMenu();
			}
			else if ( bestChit && bestChit->IsUFOChit() ) {
				InitContextMenu( CM_UFO, bestChit );
				UpdateContextMenu();
			}
			else {
				InitContextMenu( CM_NONE, bestChit );
			};
		}
	}
}


void GeoScene::InitContextMenu( int type, Chit* chit )
{
	contextChitID = chit ? chit->ID() : 0;
	gamui::RenderAtom nullAtom;

	for( int i=0; i<MAX_CONTEXT; ++i ) {
		context[i].SetVisible( false );
		context[i].SetDeco( nullAtom, nullAtom );
		context[i].SetEnabled( true );
	}

	if ( type == CM_NONE ) {
		for( int i=0; i<MAX_CONTEXT; ++i )
			context[i].SetVisible( false );
		contextChitID = 0;
	}
	else if ( type == CM_BASE ) {
		BaseChit* baseChit = chit->IsBaseChit();

		static const char* base[MAX_CONTEXT] = { "Supply", "Equip", "Build", 0, 0, 0 };
		for( int i=0; i<MAX_CONTEXT; ++i ) {
			if ( base[i] ) {
				context[i].SetVisible( true );
				context[i].SetText( base[i] );
			}
			else {
				context[i].SetVisible( false );
			}
		}
#ifdef IMMEDIATE_BUY
		context[ CONTEXT_CARGO ].SetEnabled( baseChit->IsFacilityComplete( BaseChit::FACILITY_CARGO ));
#else
		context[ CONTEXT_CARGO ].SetEnabled( CanSendCargoPlane( chit->MapPos() ) );
#endif
		context[ CONTEXT_EQUIP ].SetEnabled( Unit::Count( baseChit->GetUnits(), MAX_TERRANS, Unit::STATUS_ALIVE ) > 0 );
	}
	else if ( type == CM_UFO ) {
		for( int i=0; i<MAX_BASES; ++i ) {
			BaseChit* baseChit = chitBag.GetBaseChit( i );

			if ( baseChit ) {
				context[i].SetVisible( true );
				context[i].SetText( baseChit->Name() );
				context[i].SetDeco( nullAtom, nullAtom );

				if (     baseChit->IsFacilityComplete( BaseChit::FACILITY_LANDER )
					  && Unit::Count( baseChit->GetUnits(), MAX_TERRANS, Unit::STATUS_ALIVE )
					  && !chitBag.GetCargoComingFrom( CargoChit::TYPE_LANDER, baseChit->MapPos() ) )
				{
					if ( baseChit->IssueUnitWarning() ) {
						RenderAtom atom = Game::CalcIcon2Atom( ICON2_WARNING, true );
						context[i].SetDeco( atom, atom );
						context[i].SetDecoLayout( gamui::Button::RIGHT, context[i].Height()/2, 0 );
					}
					context[i].SetEnabled( true );
				}
				else {
					context[i].SetEnabled( false );
				}
			}
			else {
				context[i].SetVisible( false );
				context[i].SetText( "none" );
			}
		}
		for( int i=MAX_BASES; i<MAX_CONTEXT; ++i ) {
			context[i].SetVisible( false );
		}
	}
	else {
		GLASSERT( 0 );
	}
}


void GeoScene::UpdateContextMenu()
{
	Chit* contextChit = chitBag.GetChit( contextChitID );
	int size = 0;
	for( int i=0; i<MAX_CONTEXT; ++i ) {
		if ( context[i].Visible() && contextChit ) {
			size = i+1;
		}
	}

	if ( size > 0 ) {
		Vector2F pos = contextChit->Pos();
		Vector3F pos3 = { pos.x, 0, pos.y };
		Vector2F ui0, ui1;

		const Screenport& port = GetEngine()->GetScreenport();
		
		port.WorldToUI( pos3, &ui0 );
		pos3.x += GEO_MAP_XF;
		port.WorldToUI( pos3, &ui1 );

		Vector2F ui = ( port.UIBoundsClipped3D().Contains( ui0 )) ? ui0 : ui1;
		UIRenderer::LayoutListOnScreen( context, size, sizeof(context[0]), ui.x, ui.y, 2.0f, port );
	}
}


bool GeoScene::PlaceBase( const grinliz::Vector2I& map )
{
	int index = chitBag.AllocBaseChitIndex();

	if (    geoMapData.GetType( map.x, map.y ) 
		 && !geoMapData.IsCity( map.x, map.y )
		 && index < MAX_BASES ) 
	{
		bool parked = false;
		if ( chitBag.GetBaseChitAt( map ) )
			parked = true;

		if ( !parked ) {
			int	region = geoMapData.GetRegion( map.x, map.y);
			Chit* chit = new BaseChit( tree, map, index, game->GetItemDefArr(), 
									   firstBase, 
									   (regionData[region].traits & RegionData::TRAIT_MILITARISTIC ) != 0 );
			chitBag.Add( chit );
			firstBase = false;
			baseButton.SetUp();
			research.KickResearch();
			return true;
		}
	}
	return false;
}


void GeoScene::SetMapLocation()
{
	const Screenport& port = GetEngine()->GetScreenport();

	const Vector3F& cameraPos = GetEngine()->camera.PosWC();
	float d = cameraPos.y * tanf( ToRadian( EL_FOV*0.5f ) );
	float leftEdge = cameraPos.x - d;

	if ( leftEdge < 0 ) {
		GetEngine()->camera.DeltaPosWC( GeoMap::MAP_X, 0, 0 );
	}
	if ( leftEdge > (float)GeoMap::MAP_X ) {
		GetEngine()->camera.DeltaPosWC( -GeoMap::MAP_X, 0, 0 );
	}

	static const Vector2F pos[GEO_REGIONS] = {	// "North", "South", "Europe", "Asia", "Africa", "Under"
		{ 64.f/1000.f, 9.f/500.f },
		{ 97.f/1000.f, 455.f/500.f },
		{ 502.f/1000.f, 9.f/500.f },
		{ 718.f/1000.f, 106.f/500.f },
		{ 520.f/1000.f, 459.f/500.f },
		{ 720.f/1000.f, 459.f/500.f },
	};

	// The camera moved in the code above.
	Screenport aPort = port;
	// Not quite sure the SetPerspective and SetUI is needed. Initializes some state.
	aPort.SetPerspective( 0 );
	aPort.SetUI( 0 );
	aPort.SetViewMatrices( GetEngine()->camera.ViewMatrix() );

	for( int i=0; i<GEO_REGIONS; ++i ) {
		Vector3F world = { pos[i].x*(float)GeoMap::MAP_X, 0, pos[i].y*(float)GeoMap::MAP_Y };
		Vector2F ui;
		aPort.WorldToUI( world, &ui );

		const float WIDTH_IN_UI = 2.0f*port.UIHeight();
		while ( ui.x < 0 ) ui.x += WIDTH_IN_UI;
		while ( ui.x > port.UIWidth() ) ui.x -= WIDTH_IN_UI;

		areaWidget[i]->SetOrigin( ui.x, ui.y );
	}
	UpdateContextMenu();
}


void GeoScene::FireBaseWeapons()
{
	float best2 = FLT_MAX;
	const UFOChit* bestUFO = 0;
	
	for( int type=0; type<2; ++type ) {
		while ( missileTimer[type] > MissileFreq( type ) ) {
			missileTimer[type] -= MissileFreq( type );

			for( int i=0; i<MAX_BASES; ++i ) {
				BaseChit* base = chitBag.GetBaseChit( i );
				if ( !base )
					continue;

				// Check if the base has the correct weapon:
				if ( !base->IsFacilityComplete( type==0 ? BaseChit::FACILITY_GUN : BaseChit::FACILITY_MISSILE ))
					continue;

				float range2 = MissileRange( type ) * MissileRange( type );

				for( Chit* it=chitBag.Begin(); it != chitBag.End(); it=it->Next() ) {
					UFOChit* ufo = it->IsUFOChit();
					if ( ufo && ufo->Flying() ) {
						float len2 = Cylinder<float>::LengthSquared( ufo->Pos(), base->Pos() );
						if ( len2 < range2 && len2 < best2 ) {
							best2 = len2;
							bestUFO = ufo;
						}
					}
				}
				if ( bestUFO ) 
				{
					// FIRE! compute the "best" shot. Simple, but not trivial, math. Stack overflow, you are mighty:
					// http://stackoverflow.com/questions/2248876/2d-game-fire-at-a-moving-target-by-predicting-intersection-of-projectile-and-un
					// a := sqr(target.velocityX) + sqr(target.velocityY) - sqr(projectile_speed)
					// b := 2 * (target.velocityX * (target.startX - cannon.X) + target.velocityY * (target.startY - cannon.Y))
					// c := sqr(target.startX - cannon.X) + sqr(target.startY - cannon.Y)
					//
					// Note that there are bugs around the seam of cylindrical coordinates, that I don't properly account for.
					// (The scrolling world is a pain.)

					float a = SquareF( bestUFO->Velocity().x ) + SquareF(  bestUFO->Velocity().y ) - SquareF( MissileSpeed( type ) );
					float b = 2.0f*(bestUFO->Velocity().x * (bestUFO->Pos().x - base->Pos().x) + bestUFO->Velocity().y * (bestUFO->Pos().y - base->Pos().y));
					float c = SquareF( bestUFO->Pos().x - base->Pos().x ) + SquareF( bestUFO->Pos().y - base->Pos().y);

					float disc = b*b - 4.0f*a*c;

					// Give some numerical space
					if ( disc > 0.01f ) {
						float t1 = ( -b + sqrtf( disc ) ) / (2.0f * a );
						float t2 = ( -b - sqrtf( disc ) ) / (2.0f * a );

						float t = 0;
						if ( t1 > 0 && t2 > 0 ) t = Min( t1, t2 );
						else if ( t1 > 0 ) t = t1;
						else t = t2;

						Vector2F aimAt = bestUFO->Pos() + t * bestUFO->Velocity();
						Vector2F range = aimAt - base->Pos();
						if ( range.LengthSquared() < (range2*1.1f) ) {
							Vector2F normal = range;;
							normal.Normalize();
						
							Missile* missile = missileArr.Push();
							missile->type = type;
							missile->pos = base->Pos();
							missile->velocity = normal * MissileSpeed(type);
							missile->time = 0;
							missile->lifetime = MissileLifetime( type );
						}
					}
				}
			}
		}
	}
}


void GeoScene::UpdateMissiles( U32 deltaTime )
{
	ParticleSystem* system = ParticleSystem::Instance();
	static const float INV = 1.0f/255.0f;

	static const Color4F color[2] = {
		{ 248.f*INV, 228.f*INV, 8.f*INV, 1.0f },
		{ 59.f*INV, 98.f*INV, 209.f*INV, 1.0f },
	};
	static const Color4F cvel = { 0, 0, 0, -1.0f };
	static const Vector3F vel0 = { 0, 0, 0 };
	static const Vector3F velUP = { 0, 1, 0 };

	for( int i=0; i<missileArr.Size(); ) {
		static const U32 UPDATE = 20;
		static const float STEP = (float)UPDATE / 1000.0f;

		Missile* m = &missileArr[i];

		m->time += deltaTime;
		bool done = false;
		while ( m->time >= UPDATE ) {
			m->time -= UPDATE;
			if ( m->lifetime < UPDATE ) {
				done = true;
				break;
			}
			else {
				m->lifetime -= UPDATE;
			}

			m->pos += m->velocity * STEP;
			if ( m->pos.x < 0 )
				m->pos.x += (float)GEO_MAP_X;
			if ( m->pos.x > (float)GEO_MAP_X )
				m->pos.x -= (float)GEO_MAP_X;

			for( int z=0; z<2; ++z ) {
				Vector3F pos3 = { m->pos.x + (float)(GEO_MAP_X*z), MISSILE_HEIGHT, m->pos.y };
				system->EmitQuad( ParticleSystem::CIRCLE, color[m->type], cvel, pos3, 0, vel0, 0, 0.08f, 0.1f );
			}

			// Check for impact!
			for( Chit* it=chitBag.Begin(); it != chitBag.End(); it=it->Next() ) {
				UFOChit* ufo = it->IsUFOChit();
				if ( ufo && ufo->Flying() ) {

					float d2 = Cylinder<float>::LengthSquared( ufo->Pos(), m->pos );
					if ( d2 < ufo->Radius() ) {
						for( int z=0; z<2; ++z ) {
							Vector3F pos3 = { m->pos.x + (float)(GEO_MAP_X*z), MISSILE_HEIGHT, m->pos.y };
							system->EmitPoint( 40, ParticleSystem::PARTICLE_HEMISPHERE, color[m->type], cvel, pos3, 0.1f, velUP, 0.2f );
						}
						switch ( difficulty ) {
						case EASY:		ufo->DoDamage( 1.5f );	break;
						case NORMAL:	ufo->DoDamage( 1.0f );	break;
						case HARD:		ufo->DoDamage( 0.8f );	break;
						case VERY_HARD:	ufo->DoDamage( 0.7f );	break;
						};
						SoundManager::Instance()->QueueSound( "geo_ufo_hit" );
						done = true;
					}
				}
			}
		}

		if ( done )
			missileArr.SwapRemove( i );
		else
			++i;
	}
}


void GeoScene::DoBattle( CargoChit* landerChit, UFOChit* ufoChit )
{
	GLASSERT( !landerChit || landerChit->Type() == CargoChit::TYPE_LANDER );
	GLASSERT( ufoChit || landerChit );

	BaseChit* baseChit = 0;
	bool baseAttack = false;
	++nBattles;

	if ( ufoChit ) {
		// BattleShip attacks base
		baseChit = chitBag.GetBaseChitAt( ufoChit->MapPos() );
		baseAttack = true;
	}
	if ( landerChit ) {
		if ( landerChit->IsDestroyed() )
			return;	// something went wrong, but don't keep going!

		// Lander attacks UFO
		Chit* chitAt = chitBag.GetParkedChitAt( landerChit->MapPos() );
		ufoChit = chitAt ? chitAt->IsUFOChit() : 0;
		baseChit = chitBag.GetBaseChitAt( landerChit->Origin() );
	}

	if ( ufoChit ) {
		GLASSERT( baseChit );	// now sure how this is possible, although not destructive

		if ( baseChit && !baseChit->IsDestroyed() ) {
			Unit* units = baseChit->GetUnits();

			TimeState state;
			CalcTimeState( timeline, &state );

			int scenario = FARM_SCOUT;
			if ( baseAttack ) {
				scenario = TERRAN_BASE;
			}
			else if ( ufoChit->Type() == UFOChit::BATTLESHIP ) {
				scenario = BATTLESHIP;
			}
			else if ( ufoChit->Type() == UFOChit::BASE ) {
				scenario = ALIEN_BASE;
			}
			else {
				int type = geoMapData.GetType( ufoChit->MapPos().x, ufoChit->MapPos().y );
				switch ( type ) {
				case GeoMapData::CITY:		scenario = CITY;	break;
				case GeoMapData::FARM:		scenario = (ufoChit->Type() == UFOChit::SCOUT ) ? FARM_SCOUT : FARM_DESTROYER;	break;
				case GeoMapData::FOREST:	scenario = (ufoChit->Type() == UFOChit::SCOUT ) ? FRST_SCOUT : FRST_DESTROYER;	break;
				case GeoMapData::DESERT:	scenario = (ufoChit->Type() == UFOChit::SCOUT ) ? DSRT_SCOUT : DSRT_DESTROYER;	break;
				case GeoMapData::TUNDRA:	scenario = (ufoChit->Type() == UFOChit::SCOUT ) ? TNDR_SCOUT : TNDR_DESTROYER;	break;
				default: GLASSERT( 0 ); break;
				}
			}

			float alienRank = state.alienRank;
			if ( scenario == ALIEN_BASE ) {
				alienRank = (float)(DEFAULT_NUM_ALIEN_RANKS-2+difficulty);
			}
			// Bad idea: difficulty of UFOs plenty broad already.
			//if ( IsScoutScenario( scenario ) )
			//	rank -= 0.5f;	// lower ranks here
			//if ( scenario == BATTLESHIP )	// battleships are hard enough
			//	rank += 0.5f;
			alienRank = Clamp( alienRank, 0.0f, (float)(NUM_ALIEN_RANKS-1) );

			static const Vector3F zero = { 0, 0, 0 };
			for( int i=0; i<MAX_TERRANS; ++i ) {
				if ( units[i].InUse() ) {
					units[i].Heal();
					units[i].SetPos( zero, 0 );		// no location
				}
			}

			BattleSceneData* data = new BattleSceneData();
			data->seed			= baseChit->MapPos().x + baseChit->MapPos().y*GEO_MAP_X + scenario*37;
			data->scenario		= scenario;
			data->crash			= ( ufoChit->AI() == UFOChit::AI_CRASHED );
			data->soldierUnits	= units;
			data->nScientists	= baseChit->NumScientists();
			data->dayTime		= geoMap->GetDayTime( ufoChit->Pos().x );
			data->alienRank		= alienRank;
			data->storage		= baseChit->GetStorage();
			chitBag.SetBattle( ufoChit, landerChit, scenario );
			
			game->DeleteSaveFile( SAVEPATH_TACTICAL, 0 );
			game->Save( 0, true, false );

			game->PushScene( Game::BATTLE_SCENE, data );

		}
	}
}



void GeoScene::ChildActivated( int childID, Scene* childScene, SceneData* data )
{
	if ( childID == Game::BATTLE_SCENE ) {
		XMLDocument doc;

		if ( !data ) {
			// Load existing map
			GLASSERT( game->HasSaveFile( SAVEPATH_TACTICAL, loadSlot ));
			if ( game->HasSaveFile( SAVEPATH_TACTICAL, loadSlot ) ) {
				FILE* fp = game->GameSavePath( SAVEPATH_TACTICAL, SAVEPATH_READ, loadSlot );
				GLASSERT( fp );
				if ( fp ) {
					doc.LoadFile( fp );
					fclose( fp );
				}
			}
		}
		else {
			// Create new map.
			GLASSERT( !game->HasSaveFile( SAVEPATH_TACTICAL, loadSlot ) );
			FILE* fp = game->GameSavePath( SAVEPATH_TACTICAL, SAVEPATH_WRITE, loadSlot );
			if ( fp ) {
				TacticalIntroScene::WriteXML( fp, (const BattleSceneData*)data, game->GetItemDefArr(), game->GetDatabase() );
				fclose( fp );

				fp = game->GameSavePath( SAVEPATH_TACTICAL, SAVEPATH_READ, loadSlot );
				if ( fp ) {
					doc.LoadFile( fp );
					fclose( fp );
				}
			}
		}
		GLASSERT( doc.FirstChildElement() );
		GLASSERT( !doc.Error() );
		if ( !doc.Error() ) {
			XMLElement* game = doc.FirstChildElement( "Game" );
			GLASSERT( game );
			if ( game ) {
				XMLElement* scene = game->FirstChildElement( "BattleScene" );
				GLASSERT( scene );
				if ( scene ) {
					((BattleScene*)childScene)->Load( scene );
				}
			}
		}
	}
	loadSlot = 0;
}


void GeoScene::SceneResult( int sceneID, int result )
{
	research.KickResearch();
	if ( sceneID == Game::FASTBATTLE_SCENE || sceneID == Game::BATTLE_SCENE ) {

		UFOChit*	ufoChit = chitBag.GetBattleUFO();	// possibly null if UFO destroyed
														// or null for some bug. :(
		CargoChit*	landerChit = chitBag.GetBattleLander();		// will be null if Battleship attacking base

		BaseChit* baseChit = 0;
		if ( landerChit ) {
			baseChit = chitBag.GetBaseChitAt( landerChit->Origin() );
		}
		else {
			if ( ufoChit ) baseChit = chitBag.GetBaseChitAt( ufoChit->MapPos() );
		}
		GLASSERT( baseChit );
		if ( baseChit ) {
			Unit* baseUnits = baseChit->GetUnits();

			// The battlescene has written (or loaded) the BattleData. It is current and
			// in memory. All the stuff dropped on the battlefield is in the battleData.storage,
			// if the battle was a victory. (For TIE or DEFEAT, it is empty, and the storage
			// manipulation below will do nothing.)

			int result = game->battleData.CalcResult();
			if ( game->battleData.GetScenario() == TERRAN_BASE ) {
				if ( result == BattleData::TIE )
					result = BattleData::DEFEAT;	// can't tie on base attack
			}
			Unit* battleUnits = game->battleData.UnitsPtr();

			// Inform the research system of found items.
			for( int i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
				const ItemDef* itemDef = game->battleData.GetStorage().GetItemDef(i);
				if ( itemDef && game->battleData.GetStorage().GetCount( i )) {
					research.SetItemAcquired( itemDef->name );
				}
			}
			research.KickResearch();

			for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i ) {
				if ( battleUnits[i].IsAlive() || battleUnits[i].IsUnconscious() ) {
					battleUnits[i].DoMissionEnd();
					battleUnits[i].Heal();
					battleUnits[i].GetInventory()->RestoreClips();

					/* This code use to essentially restore used clips, which 
					   is now fixed by not destroying clips. This code echoes
					   some bug reports and is of dubious value.

					// Try to restore items to downed units, by comparing
					// old inventory to new inventory
					if ( battleUnits[i].GetInventory()->Empty() ) {
						const Unit* oldUnit = Unit::Find( baseUnits, MAX_TERRANS, battleUnits[i].Body() );
						if ( oldUnit ) {
							for( int k=0; k<Inventory::NUM_SLOTS; ++k ) {
								const ItemDef* itemDef = oldUnit->GetInventory()->GetItemDef( k );
								if ( itemDef ) {
									Item item;
									game->battleData.storage.RemoveItem( itemDef, &item );
									battleUnits[i].GetInventory()->AddItem( item, 0 );
								}
							}
						}
					}
					*/
				}
			}

			// Copy the units over.
			// Should work, but there is a bug. Need to re-write unit code.
			//for( int i=0; i<MAX_TERRANS; ++i ) {
			//	units[i].Free();
			//}
			// Since the base can't render, can free
			memset( baseUnits, 0, sizeof(Unit)*MAX_TERRANS );
			{
				int unitCount=0;
				for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i ) {
					if ( battleUnits[i].IsAlive() || battleUnits[i].IsUnconscious() ) {
						baseUnits[unitCount++] = battleUnits[i];
					}
				}
			}


			int nCivsAlive = Unit::Count( game->battleData.Units(CIV_UNITS_START), MAX_CIVS, Unit::STATUS_ALIVE );
			int nCivsDead  = Unit::Count( game->battleData.Units(CIV_UNITS_START), MAX_CIVS, Unit::STATUS_KIA );

			if ( result == BattleData::VICTORY ) {
				// Apply penalty for lost civs:
				if ( game->battleData.GetScenario() == CITY ) {
					int	region = geoMapData.GetRegion( baseChit->MapPos().x, baseChit->MapPos().y );
					if ( (nCivsAlive+nCivsDead) > 0 && region >= 0 ) {
						float ratio = 1.0f - (float)nCivsAlive / (float)(nCivsAlive+nCivsDead);
						if ( chitBag.GetBattleScenario() == CITY ) { 
							ratio *= 2.0f; 
						}
						regionData[region].influence += ratio;
						regionData[region].influence = Min( regionData[region].influence, (float)MAX_INFLUENCE );
						areaWidget[region]->SetInfluence( regionData[region].influence );
					}
				}

				// Did the aliens lose the base?
				if ( ufoChit && ufoChit->Type() == UFOChit::BASE ) {
					gameVictory = true;
				}
				delete ufoChit;
			}

			if ( !landerChit ) {
				// Base attack - really good or really bad.
				if ( result == BattleData::DEFEAT ) {
					// Make sure to destroy base and deployed lander.
					CargoChit* baseLander = chitBag.GetCargoComingFrom( CargoChit::TYPE_LANDER, baseChit->MapPos() );
					if ( baseLander ) { 
						baseLander->SetDestroyed();
					}
					baseChit->SetDestroyed();
					if ( ufoChit )
						ufoChit->SetAI( UFOChit::AI_ORBIT );						
				}
				else if ( result == BattleData::VICTORY ) {
					// de-occupies
					int region = geoMapData.GetRegion( baseChit->MapPos() );
					if ( region >= 0 ) {
						Vector2I capitalPos = geoMapData.Capital( region );
						UFOChit* ufo = chitBag.GetLandedUFOChitAt( capitalPos );
						if ( ufo ) {
							ufo->SetAI( UFOChit::AI_ORBIT );
						}
						regionData[region].influence -= BASE_WON_INFLUENCE;
						regionData[region].influence = Clamp( regionData[region].influence, 0.0f, (float)MAX_INFLUENCE );
						areaWidget[region]->SetInfluence( regionData[region].influence );
					}
				}
			}

			// Don't want pointers floating around.
			memset( battleUnits, 0, sizeof(Unit)*MAX_UNITS );

			// Merge storage
			baseChit->GetStorage()->AddStorage( game->battleData.GetStorage() );
			game->battleData.ClearStorage();

			// If the tech isn't high enough, can't use cells and anti. Makes sure
			// to clear from the base storage to filter out picked up weapons.
			static const char* remove[2] = { "Cell", "Anti" };
			for( int i=0; i<2; ++i ) {
				if ( research.GetStatus( remove[i] ) != Research::TECH_RESEARCH_COMPLETE ) {
					baseChit->GetStorage()->ClearItem( remove[i] );
					for( int j=TERRAN_UNITS_START; j<TERRAN_UNITS_END; ++j ) {
						baseUnits[j].GetInventory()->ClearItem( remove[i] );
					}
				}
			}

			if ( game->battleData.GetScenario() == TERRAN_BASE ) {
				baseChit->SetNumScientists( nCivsAlive );
			}
		}
		game->DeleteSaveFile( SAVEPATH_TACTICAL, 0 );
		game->Save( 0, true, false );
	}
}


void GeoScene::PushBaseTradeScene( BaseChit* baseChit )
{
	GLASSERT( baseChit );
	int region = geoMapData.GetRegion( baseChit->MapPos() );

	BaseTradeSceneData* data = new BaseTradeSceneData( game->GetItemDefArr() );			
	data->baseName   = baseChit->Name();
	data->regionName = gRegionName[region];
	data->base		 = baseChit->GetStorage();
	regionData[region].SetStorageNormal( research, &data->region,
		                                 (regionData[region].traits & RegionData::TRAIT_TECH) != 0,
										 (regionData[region].traits & RegionData::TRAIT_MANUFACTURE) != 0 );
	data->cash		 = &cash;
	data->costFlag   = COST_IN_USE;
	if ( regionData[region].traits & RegionData::TRAIT_CAPATALIST )
		data->costFlag |= COST_FLAG_CAPATALISM;
	if ( regionData[region].traits & RegionData::TRAIT_MANUFACTURE )
		data->costFlag |= COST_FLAG_ARMOR_T3;
	if ( regionData[region].traits & RegionData::TRAIT_TECH )
		data->costFlag |= COST_FLAG_WEAPON_T3;

	data->soldierBoost = regionData[region].traits & RegionData::TRAIT_MILITARISTIC ? true : false;
	data->soldiers	 = baseChit->CanUseSoldiers() ? baseChit->GetUnits() : 0;
	data->scientists = baseChit->CanUseScientists() ? baseChit->GetScientstPtr() : 0;
	game->PushScene( Game::BASETRADE_SCENE, data );
}


bool GeoScene::RegionOccupied( int region ) const
{
	Vector2I capitalLoc = geoMapData.Capital( region );
	UFOChit* ufo = chitBag.GetLandedUFOChitAt( capitalLoc );
	if ( ufo && ( ufo->AI() == UFOChit::AI_OCCUPATION ) ) {
		return true;
	}
	return false;
}


void GeoScene::DoTick( U32 currentTime, U32 deltaTime )
{
	timeline += deltaTime;
	researchTimer += deltaTime;
	if ( researchTimer > 1000 ) {
		researchTimer -= 1000;

		int nResearchers = 0;
		for( int i=0; i<MAX_BASES; ++i ) {
			BaseChit* baseChit = chitBag.GetBaseChit(i);
			if ( baseChit ) {
				nResearchers += chitBag.GetBaseChit(i)->NumScientists();				
				
				int region = geoMapData.GetRegion( baseChit->MapPos() );
				if ( regionData[region].traits & RegionData::TRAIT_SCIENTIFIC ) {
					nResearchers += chitBag.GetBaseChit(i)->NumScientists() * 3 / 2;	// 50% bonus
				}
			}
		}
		research.DoResearch( nResearchers );

		if ( research.GetStatus( "Beacon" ) == Research::TECH_RESEARCH_COMPLETE ) {
			if ( !chitBag.GetUFOBase() ) {
				// Just run through everything to find the tundra.
				const static int MAX=32;
				int nLoc=0;
				Vector2I loc[MAX];

				for( int y=0; y<GEO_MAP_Y && nLoc < MAX; ++y ) {
					for( int x=0; x<GEO_MAP_X && nLoc < MAX; ++x ) {
						Vector2I p = { x, y };

						if (    geoMapData.GetType( x, 0 ) == GeoMapData::TUNDRA
							&& chitBag.GetParkedChitAt( p ) == 0
							&& chitBag.GetBaseChitAt( p ) == 0 ) 
						{
							loc[nLoc++] = p;
						}
					}
				}
				if ( nLoc > 0 ) {
					Vector2I p = loc[ random.Rand( nLoc ) ];
					Vector2F pf = { (float)p.x+0.5f, (float)p.y+0.5f };
					UFOChit* chit = new UFOChit( GetEngine()->GetSpaceTree(), UFOChit::BASE, pf, pf );
					chit->SetAI( UFOChit::AI_PARKED );
					chitBag.Add( chit );
				}
			}
		}
	}

	char cashBuf[16];
	SNPrintf( cashBuf, 16, "$%d", cash );
	cashLabel.SetText( cashBuf );

	alienTimer += deltaTime;

	const int CYCLE = 5000;
	float rotation = (float)(game->CurrentTime() % CYCLE)*(360.0f/(float)CYCLE);
	if ( rotation > 90 && rotation < 270 )
		rotation += 180;

	if ( firstBase ) {
		baseButton.SetDecoRotationY( rotation );
	}
	else {
		baseButton.SetDecoRotationY( 0 );
	}

	if ( GameSettingsManager::Instance()->GetBattleShipParty() ) {
		if ( alienTimer > 5000 ) {
			alienTimer -= 5000;
			Vector2F start, dest;

			for( int i=0; i<3; ++i ) {
				geoAI->GenerateAlienShip( UFOChit::BATTLESHIP, &start, &dest, regionData, chitBag );
				Chit *test = new UFOChit( tree, UFOChit::BATTLESHIP, start, dest );
				chitBag.Add( test );
			}
		}
	}
	else {
		TimeState state;
		CalcTimeState( timeline, &state );
		if ( alienTimer > state.alienTime ) {
			alienTimer -= state.alienTime;

			Vector2F start, dest;

			int type = UFOChit::SCOUT + random.Select( state.alienType, 3 );

			geoAI->GenerateAlienShip( type, &start, &dest, regionData, chitBag );
			Chit *test = new UFOChit( tree, type, start, dest );
			SoundManager::Instance()->QueueSound( "geo_ufo_atmo" );
			chitBag.Add( test );
		}
	}
	missileTimer[0] += deltaTime;
	missileTimer[1] += deltaTime;
	FireBaseWeapons();

	geoMap->DoTick( currentTime, deltaTime );

	UpdateMissiles( deltaTime );

	for( Chit* chitIt=chitBag.Begin(); chitIt != chitBag.End(); chitIt=chitIt->Next() ) 
	{
		int message = chitIt->DoTick( deltaTime );

		// Special case: Defer base attacks until lander arrives
		// Removed: nice, but hard to test.
//		if (    chitIt->IsUFOChit() 
//			 && chitIt->IsUFOChit()->AI() == UFOChit::AI_BASE_ATTACK
//			 && !chitBag.GetCargoComingFrom( CargoChit::TYPE_LANDER, pos ) ) 
//		{
//			if ( !game->IsScenePushed() )
//				DoBattle( 0, chitIt->IsUFOChit() );
//			break;
//		}

		// Special case: send lander home if no target!
		if ( chitIt->IsCargoChit() ) {
			chitIt->IsCargoChit()->CheckDest( chitBag );
		}

		float influenceModifier = 1.0f;
		int region = geoMapData.GetRegion( chitIt->MapPos() );
		if ( region >= 0 && ( regionData[region].traits & RegionData::TRAIT_NATIONALIST ) ) {
			influenceModifier = 0.6f;
		}

		switch ( message ) {

		case Chit::MSG_NONE:
			break;

		case Chit::MSG_DONE:
			chitIt->SetDestroyed();
			break;

		case Chit::MSG_LANDER_ARRIVED:
			if ( !game->IsScenePushed() ) {
				GLASSERT( chitIt->IsCargoChit() );
				DoBattle( chitIt->IsCargoChit(), 0  );
			}
			break;

#ifndef IMMEDIATE_BUY
		case Chit::MSG_CARGO_ARRIVED:
			if ( !game->IsScenePushed() ) {

				Vector2I basei = chitIt->MapPos();
				BaseChit* baseChit = chitBag.GetBaseChitAt( basei );
				int region = geoMapData.GetRegion( basei.x, basei.y );

				if ( !regionData[region].occupied && baseChit ) {
					PushBaseTradeScene( baseChit );
				}
			}
			break;
#endif

		case Chit::MSG_UFO_AT_DESTINATION:
			{

				// Battleship, at a capital, it can occupy.
				// Battleship, at base, can attack it
				// Battleship or Destroyer, at city, can city attack
				// Any ship, not at city, can crop circle

				UFOChit* ufoChit = chitIt->IsUFOChit();
				int region = geoMapData.GetRegion( ufoChit->MapPos() );
				//int type = geoMapData.GetType( ufoChit->MapPos() );

				bool returnToOrbit = false;

				// If the destination space is taken, return to orbit.
				Chit* parked = chitBag.GetParkedChitAt( ufoChit->MapPos() );
				if ( parked ) {
					returnToOrbit = true;
				}

				if ( returnToOrbit ) {
					ufoChit->SetAI( UFOChit::AI_ORBIT );
				}
				else if (      ufoChit->Type() == UFOChit::BATTLESHIP 
							&& chitBag.GetBaseChitAt( ufoChit->MapPos() )
							&& regionData[region].influence >= MIN_BASE_ATTACK_INFLUENCE )
				{
					if ( !game->IsScenePushed() ) {
						DoBattle( 0, chitIt->IsUFOChit() );
					}
					ufoChit->SetAI( UFOChit::AI_ORBIT );
				}
				else if (      ufoChit->Type() == UFOChit::BATTLESHIP 
						    && !RegionOccupied( region )
						    && geoMapData.Capital( region ) == ufoChit->MapPos() 
							&& regionData[region].influence >= MIN_OCCUPATION_INFLUENCE )
				{
					regionData[region].influence = MAX_INFLUENCE;
					areaWidget[region]->SetInfluence( regionData[region].influence );
					ufoChit->SetAI( UFOChit::AI_OCCUPATION );
				}
				else if (    ( ufoChit->Type() == UFOChit::BATTLESHIP || ufoChit->Type() == UFOChit::FRIGATE )
						    && !RegionOccupied( region )
						    && geoMapData.IsCity( ufoChit->MapPos() )
							&& regionData[region].influence >= MIN_CITY_ATTACK_INFLUENCE ) 
				{
					ufoChit->SetAI( UFOChit::AI_CITY_ATTACK );
					SoundManager::Instance()->QueueSound( "geo_ufo_citybase" );
				}
				else if (    !geoMapData.IsCity( ufoChit->MapPos() ) 
						   && !chitBag.GetBaseChitAt( ufoChit->MapPos() )) 
				{
					ufoChit->SetAI( UFOChit::AI_CROP_CIRCLE );
				}
				else {
					// Er, total failure. Collision of AI goals.
					ufoChit->SetAI( UFOChit::AI_ORBIT );
				}
			}
			break;

			case Chit::MSG_CROP_CIRCLE_COMPLETE:
			{
				SoundManager::Instance()->QueueSound( "geo_ufo_cropcircle" );
				Chit *chit = new CropCircle( tree, chitIt->MapPos(), random.Rand() );
				chitBag.Add( chit );

				int region = geoMapData.GetRegion( chitIt->MapPos() );
				if ( regionData[region].influence < MAX_CROP_CIRCLE_INFLUENCE ) {
					regionData[region].influence += (float)CROP_CIRCLE_INFLUENCE * influenceModifier;
					regionData[region].influence = Min( regionData[region].influence, (float)MAX_INFLUENCE );
					areaWidget[region]->SetInfluence( regionData[region].influence );
				}

				if ( !geoMapData.IsWater( chitIt->MapPos() ) ) {
					int region = geoMapData.GetRegion( chitIt->MapPos() );
					regionData[region].Success();
				}
			}
			break;

			case Chit::MSG_BATTLESHIP_DESTROYED:
				cash += 50;
				break;

			case Chit::MSG_UFO_CRASHED:
			{
				SoundManager::Instance()->QueueSound( "geo_ufo_crash" );

				// Battleships use a different message, above.
				if ( chitIt->IsUFOChit()->Type() == UFOChit::FRIGATE ) {
					cash += 25;
				}
				else {
					cash += 10;
				}

				// Can only crash on open space.
				// Check for UFOs, bases.
				bool parked = false;
				Chit* parkedChit = chitBag.GetParkedChitAt( chitIt->MapPos() );
				BaseChit* baseChit = chitBag.GetBaseChitAt( chitIt->MapPos() );

				if( ( parkedChit && (parkedChit != chitIt ) ) || baseChit ) {
					parked = true;
				}
				// check for water, cities
				int mapType = geoMapData.GetType( chitIt->MapPos() );

				if ( mapType == 0 || mapType == GeoMapData::CITY ) {
					parked = true;
				}

				if ( !parked ) {
					chitIt->IsUFOChit()->SetAI( UFOChit::AI_CRASHED );
					chitIt->SetMapPos( chitIt->MapPos() );
				}
				else {
					// Find bases with units. Award "gunner" XP.
					// Used to award xp. Not a bad thing,
					// but awarding cash seems to make
					// more sense to the player, and you
					// don't get high level units who never
					// run missions.
					/*
					for( int i=0; i<MAX_BASES; ++i ) {
						BaseChit* baseChit = chitBag.GetBaseChit( i );
						if ( baseChit ) {
							Unit* units = baseChit->GetUnits();
							if ( units ) {
								for( int i=0; i<MAX_TERRANS; ++i ) {
									if ( units[i].IsAlive() ) {
										units[i].CreditGunner();
									}
								}
							}
						}
					}
					*/
					chitIt->SetDestroyed();
				}

				if ( !geoMapData.IsWater( chitIt->MapPos() ) ) {
					int region = geoMapData.GetRegion( chitIt->MapPos() );
					regionData[region].UFODestroyed();
				}
			}
			break;


			case Chit::MSG_CITY_ATTACK_COMPLETE:
			{
				int region = geoMapData.GetRegion( chitIt->MapPos() );
				regionData[region].influence += (float)CITY_ATTACK_INFLUENCE * influenceModifier;
				regionData[region].influence = Min( regionData[region].influence, (float)MAX_INFLUENCE );
				areaWidget[region]->SetInfluence( regionData[region].influence );

				if ( !geoMapData.IsWater( chitIt->MapPos() ) ) {
					int region = geoMapData.GetRegion( chitIt->MapPos() );
					regionData[region].Success();
				}
			}
			break;

			/*
			case Chit::MSG_BASE_ATTACK_COMPLETE:
			{
				BaseChit* base = chitBag.GetBaseChitAt( pos );
				if ( chitBag.GetCargoComingFrom( CargoChit::TYPE_LANDER, pos ) ) {
					// There is a lander deployed. Don't destroy.
				}
				else {
					chitIt->IsUFOChit()->SetAI( UFOChit::AI_ORBIT );
					base->SetDestroyed();	// can't delete in this loop
				}
			}
			break;
			*/

			default:
				GLASSERT( 0 );
		}
	}

	if ( !game->IsScenePushed() && research.ResearchReady() ) {
		ResearchSceneData* data = new ResearchSceneData();
		data->research = &research;
		game->PushScene( Game::RESEARCH_SCENE, data );
	}

	// Check for deferred chit destruction.
	for( Chit* chitIt=chitBag.Begin(); chitIt != chitBag.End(); chitIt=chitIt->Next() ) {
		if ( chitIt->IsDestroyed() && chitIt->IsBaseChit() ) {
			static const float INV = 1.f/255.f;
			static const Color4F particleColor = { 59.f*INV, 98.f*INV, 209.f*INV, 1.0f };
			static const Color4F colorVec	= { 0.0f, -0.1f, -0.1f, -0.3f };
			static const Vector3F particleVel = { 2.0f, 0, 0 };

			for( int k=0; k<2; ++k ) {
				Vector3F pos = { chitIt->Pos().x + (float)(GEO_MAP_X*k), 1.0f, chitIt->Pos().y };

				ParticleSystem::Instance()->EmitPoint( 
					80, ParticleSystem::PARTICLE_HEMISPHERE, 
					particleColor, colorVec,
					pos, 
					0.2f,
					particleVel, 0.1f );
			}
			Vector2I mapi = chitIt->MapPos();

			// Very important to clean up cargo and lander!
			CargoChit* cargoChit = 0;
			cargoChit = chitBag.GetCargoComingFrom( CargoChit::TYPE_LANDER, chitIt->MapPos() );
			if ( cargoChit )
				cargoChit->SetDestroyed();
			cargoChit = chitBag.GetCargoGoingTo( CargoChit::TYPE_CARGO, chitIt->MapPos() );
			if ( cargoChit )
				cargoChit->SetDestroyed();
		}
	}
	chitBag.Clean();
	if ( contextChitID && !chitBag.IsChit( contextChitID ) ) {
		contextChitID = 0;
		InitContextMenu( CM_NONE, 0 );
	}

	if ( chitBag.NumBaseChits() < MAX_BASES ) {
		int n = chitBag.NumBaseChits();
		baseButton.SetEnabled( cash >= BASE_COST[n] );
		CStr<6> buf = BASE_COST[n];
		baseButton.SetText( buf.c_str() );
	}
	else {
		baseButton.SetEnabled( true );
		baseButton.SetText( "" );
	}
	

	// Check for end game
	{
		int i=0;
		for( ; i<GEO_REGIONS; ++i ) {
			if ( !RegionOccupied( i ) )
				break;
		}
		if ( i == GEO_REGIONS ) {
			GeoEndSceneData* data = new GeoEndSceneData();
			data->victory = false;
			game->PushScene( Game::GEO_END_SCENE, data );
			game->PopScene();
		}
	}
	if ( gameVictory ) {
		GeoEndSceneData* data = new GeoEndSceneData();
		data->victory = true;
		game->PushScene( Game::GEO_END_SCENE, data );
		game->PopScene();
	}
}


void GeoScene::Draw3D()
{
#if 0
	// Show locations of the Regions
	static const U8 ALPHA = 128;
	CompositingShader shader( true );
	static const Color4U colorArr[GEO_REGIONS] = {
		{ 255, 97, 106, ALPHA },
		{ 94, 255, 86, ALPHA },
		{ 91, 108, 255, ALPHA },
		{ 255, 54, 103, ALPHA },
		{ 255, 98, 22, ALPHA },
		{ 255, 177, 88, ALPHA }
	};

	for( int j=0; j<GEO_MAP_Y; ++j ) {
		for( int i=0; i<GEO_MAP_X; ++i ) {
			if ( !geoMapData.IsWater( i, j ) ) {
				Vector3F p0 = { (float)i, 0.1f, (float)j };
				Vector3F p1 = { (float)(i+1), 0.1f, (float)(j+1) };

				int region = geoMapData.GetRegion( i, j );
				//int type   = geoMap->GetType( i, j );

				shader.SetColor( colorArr[region] );
				shader.Debug_DrawQuad( p0, p1 );
			}
		}
	}
#endif
}


void GeoScene::DrawHUD()
{
#if 0
	TimeState ts;
	CalcTimeState( timeline, &ts );
	UFOText::Instance()->
		Draw( 50, 0, "nBat=%d diff=%d tmr=%.1fm alTm=%.1f rank=%.1f type=%.1f,%.1f,%.1f",
		nBattles,
		difficulty,
		(float)(timeline / 1000) / 60.0f,
		(float)ts.alienTime/1000.0f,
		ts.alienRank,
		ts.alienType[0], ts.alienType[1], ts.alienType[2] );
#endif
}


void GeoScene::CalcTimeState( U32 msec, TimeState* state )
{
	// Times in seconds.
	const static U32 FIRST_UFO      = 10 *SECOND;
	const static U32 TIME_BETWEEN_0	= 20 *SECOND;
	const static U32 TIME_BETWEEN_1 = 10 *SECOND;

	const static U32 DESTROYER0		= 2  *MINUTE;
	const static U32 DESTROYER1		= 5  *MINUTE;
	const static U32 BATTLESHIP		= 10 *MINUTE;

	state->alienTime = (U32)Interpolate( 0.0, (double)(TIME_BETWEEN_0), (double)FULL_OUT_UFO, (double)(TIME_BETWEEN_1), (double)msec );
	if ( state->alienTime < TIME_BETWEEN_1 )
		state->alienTime = TIME_BETWEEN_1;
	if ( msec <= FIRST_UFO+1000 )	// rounding so we aren't racing this timer
		state->alienTime = FIRST_UFO;	

	state->alienRank = (float)Interpolate( 0.0, 0.0, 
						(double)FULL_OUT_RANK, (double)(DEFAULT_NUM_ALIEN_RANKS)-1.5, 
						(double)msec );	// the minus one is float/int conversion, the 0.5 is not maxing out the alien rank

	int maxAlienRank = DEFAULT_NUM_ALIEN_RANKS + difficulty - 2;
	GLASSERT( maxAlienRank <= NUM_ALIEN_RANKS-1 );
	state->alienRank += (float)(difficulty-1);

	state->alienRank = Clamp( state->alienRank, 0.0f, (float)maxAlienRank );

	state->alienType[ UFOChit::SCOUT ] = 1.0f;
	state->alienType[ UFOChit::FRIGATE ] = 0.0f;
	state->alienType[ UFOChit::BATTLESHIP ] = 0.0f;

	if ( msec > DESTROYER0 )
		state->alienType[ UFOChit::FRIGATE ] = 0.3f;
	if ( msec > DESTROYER1 )
		state->alienType[ UFOChit::FRIGATE ] = 1.0f;
	if ( msec > BATTLESHIP )
		state->alienType[ UFOChit::BATTLESHIP ] = 1.0f;
}


void GeoScene::Save( XMLPrinter* printer )
{
	// Scene
	// GeoScene
	//		RegionData
	//		Chits
	//		Research
	// Units

	// ----------- main scene --------- //
	printer->OpenElement( "GeoScene" );
	XML_PUSH_ATTRIB( printer, timeline );
	XML_PUSH_ATTRIB( printer, difficulty );
	XML_PUSH_ATTRIB( printer, alienTimer );
	printer->PushAttribute( "missileTimer0", missileTimer[0] );
	printer->PushAttribute( "missileTimer1", missileTimer[1] );
	XML_PUSH_ATTRIB( printer, researchTimer );
	XML_PUSH_ATTRIB( printer, cash );
	XML_PUSH_ATTRIB( printer, firstBase );
	XML_PUSH_ATTRIB( printer, nBattles );

	// ---------- regions ----------- //
	printer->OpenElement( "RegionData" );
	for( int i=0; i<GEO_REGIONS; ++i ) {
		regionData[i].Save( printer );
	}
	printer->CloseElement();	// RegionData

	// ----------- chits ---------- //
	chitBag.Save( printer );
	// ----------- research ------- //
	research.Save( printer );

	printer->CloseElement();	// GeoScene
}


void GeoScene::Load( const XMLElement* scene )
{
	GLASSERT( StrEqual( scene->Value(), "GeoScene" ) );
	scene->QueryUnsignedAttribute( "timeline", &timeline );
	difficulty = NORMAL;
	scene->QueryIntAttribute( "difficulty", &difficulty );
	scene->QueryUnsignedAttribute( "alienTimer", &alienTimer );
	scene->QueryUnsignedAttribute( "missileTimer0", &missileTimer[0] );
	scene->QueryUnsignedAttribute( "missileTimer1", &missileTimer[1] );
	scene->QueryUnsignedAttribute( "researchTimer", &researchTimer );
	scene->QueryIntAttribute( "cash", &cash );
	scene->QueryBoolAttribute( "firstBase", &firstBase );
	scene->QueryIntAttribute( "nBattles", &nBattles );

	int i=0;
	const XMLElement* rdEle = scene->FirstChildElement( "RegionData" );
	if ( rdEle ) {
		for( const XMLElement* region=rdEle->FirstChildElement("Region"); region; ++i, region=region->NextSiblingElement( "Region" ) ) {
			regionData[i].Load( region );
			areaWidget[i]->SetInfluence( (float)regionData[i].influence );
			areaWidget[i]->SetTraits( regionData[i].traits );
		}
	}
	chitBag.Load( scene, tree, game->GetItemDefArr(), game );
	research.Load( scene->FirstChildElement( "Research" ) );

	GenerateCities();

	if ( game->HasSaveFile( SAVEPATH_TACTICAL, game->LoadSlot() ) ) {
		loadSlot = game->LoadSlot();
		// Push here. Do load in ChildActivated(). The data
		// must be null, and the file has to exist.
		game->PushScene( Game::BATTLE_SCENE, 0 );
	}
}

