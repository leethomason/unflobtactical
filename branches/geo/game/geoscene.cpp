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
#include "settings.h"
#include "battlescene.h"

#include "../engine/loosequadtree.h"
#include "../engine/particle.h"
#include "../engine/particleeffect.h"
#include "../engine/ufoutil.h"
#include "../engine/text.h"

#include "../grinliz/glutil.h"

using namespace grinliz;
using namespace gamui;

// t: tundra
// a: farm
// d: desert
// f: forest
// c: city


static const char* MAP =
	"w. 0t 0t 0t 0t 0t 0t w. w. 2c 2t 2t 2t 2t 2t 2t 2t 2t 2t 2t "
	"w. 0t 0t 0t 0t 0t 0t 2c 2c 2a 2a 2t 2t 2t 2t 2t 2t 2t w. w. "
	"w. 0c 0d 0a 0c w. w. 2c 2a 2c 2a 2d 3d 2a 3a 3a 3a 3a w. w. "
	"w. 0c 0d 0a w. w. w. 4c 4d 4d 4d 3d 3d 3a 3a 3a 3a 3c w. w. "
	"w. w. 0c w. 0f w. w. 4f 4f 4d 4c 3d 3c w. 3f w. 3c w. w. w. "
	"w. w. w. 0f 1c w. w. 4f 4f 4f 4d 4f w. w. 3c w. w. w. w. w. "
	"w. w. w. w. 1f 1f 1f w. w. 4f 4f w. w. w. w. w. 3f 3f w. w. "
	"w. w. w. w. 1f 1a 1c w. w. 4f 4f w. w. w. w. w. w. w. 5c w. "
	"w. w. w. w. w. 1f 1f w. w. 4f 4f 4f w. w. w. w. w. 5f 5d 5a "
	"w. w. w. w. w. 1f w. w. w. 4c w. w. w. w. w. w. w. 5f 5d 5c "
;


static const char* gRegionName[GEO_REGIONS] = { "North", "South", "Europe", "Asia", "Africa", "Under" };


void GeoMapData::Init( const char* map, Random* random )
{
	numLand = 0;
	for( int i=0; i<GEO_REGIONS; ++i ) {
		bounds[i].SetInvalid();
		numCities[i] = 0;
		capital[i] = 0;
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
		capital[i] = random->Rand( numCities[i] );
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
	for( int i=0; i<HISTORY; ++i ) history[i] = 1; 
	occupied = false;
}


void RegionData::SetStorageNormal( const Research& research, Storage* storage )
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
					if (    itemDef->IsWeapon() 
						 && ( itemDef->TechLevel() <= 2 || (traits & TRAIT_TECH ) ) ) 
					{
						storage->AddItem( itemDef, COUNT );
					}
					else if (    itemDef->IsArmor()
						      && ( itemDef->TechLevel() <= 2 || (traits & TRAIT_MANUFACTURE ) ) ) 
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


void RegionData::Save( FILE* fp, int depth )
{
	XMLUtil::OpenElement( fp, depth, "Region" );
	XMLUtil::Attribute( fp, "traits", traits );
	XMLUtil::Attribute( fp, "influence", influence );
	XMLUtil::Attribute( fp, "occupied", occupied );
	XMLUtil::SealElement( fp );

	for( int i=0; i<HISTORY; ++i ) {
		XMLUtil::OpenElement( fp, depth+1, "History" );
		XMLUtil::Attribute( fp, "value", history[i] );
		XMLUtil::SealCloseElement( fp );
	}
	XMLUtil::CloseElement( fp, depth, "Region" );
}


void RegionData::Load( const TiXmlElement* doc )
{
	GLASSERT( StrEqual( doc->Value(), "Region" ));
	doc->QueryIntAttribute( "traits", &traits );
	doc->QueryFloatAttribute( "influence", &influence );
	doc->QueryBoolAttribute( "occupied", &occupied );

	int count=0;
	for( const TiXmlElement* historyEle = doc->FirstChildElement( "History" ); 
		 historyEle && count<HISTORY; 
		 historyEle=historyEle->NextSiblingElement( "History" ), ++count ) 
	{
		historyEle->QueryIntAttribute( "value", &history[count] );
	}
}



GeoScene::GeoScene( Game* _game ) : Scene( _game ), research( _game->GetDatabase(), _game->GetItemDefArr(), RESEARCH_SECONDS )
{
	missileTimer[0] = 0;
	missileTimer[1] = 0;
	contextChitID = 0;
	cash = 1000;
	firstBase = true;
	timeline = 0;
	nBattles = 0;

	const Screenport& port = GetEngine()->GetScreenport();
	random.SetSeedFromTime();

	geoMapData.Init( MAP, &random );
	for( int i=0; i<GEO_REGIONS; ++i ) {
		regionData[i].Init( game->GetItemDefArr(), &random );
	}

	geoMap = new GeoMap( GetEngine()->GetSpaceTree() );
	tree = GetEngine()->GetSpaceTree();
	geoAI = new GeoAI( geoMapData );

	alienTimer = 0;
	researchTimer = 0;

	for( int i=0; i<GEO_REGIONS; ++i ) {
		areaWidget[i] = new AreaWidget( _game, &gamui3D, gRegionName[i] );
		areaWidget[i]->SetTraits( regionData[i].traits );
	}

	helpButton.Init(&gamui2D, game->GetButtonLook( Game::GREEN_BUTTON ) );
	helpButton.SetPos( port.UIWidth()-GAME_BUTTON_SIZE_F, 0 );
	helpButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	helpButton.SetDeco(  UIRenderer::CalcDecoAtom( DECO_HELP, true ), UIRenderer::CalcDecoAtom( DECO_HELP, false ) );	

	researchButton.Init(&gamui2D, game->GetButtonLook( Game::GREEN_BUTTON ) );
	researchButton.SetPos( 0, 0 );
	researchButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	researchButton.SetDeco(  UIRenderer::CalcDecoAtom( DECO_RESEARCH, true ), UIRenderer::CalcDecoAtom( DECO_RESEARCH, false ) );	

	baseButton.Init(&gamui2D, game->GetButtonLook( Game::GREEN_BUTTON ) );
	baseButton.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	baseButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	baseButton.SetDeco(  UIRenderer::CalcDecoAtom( DECO_BASE, true ), UIRenderer::CalcDecoAtom( DECO_BASE, false ) );

	cashImage.Init( &gamui2D, UIRenderer::CalcIconAtom( ICON_GREEN_STAND_MARK ), false );
	cashImage.SetPos( port.UIWidth()-GAME_BUTTON_SIZE_F*2.0f, port.UIHeight()-GAME_BUTTON_SIZE_F*0.5f );
	cashImage.SetSize( GAME_BUTTON_SIZE_F*2.0f, GAME_BUTTON_SIZE_F );
	cashImage.SetSlice( true );

	cashLabel.Init( &gamui2D );
	cashLabel.SetPos( cashImage.X()+10.0f, cashImage.Y()+10.0f );

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
	geoMap->SetVisible( true );
	chitBag.SetVisible( true );
}


void GeoScene::DeActivate()
{
	savedCameraX = GetEngine()->camera.PosWC().x;
	GetEngine()->CameraIso( true, false, 0, 0 );
	GetEngine()->SetIMap( 0 );
	geoMap->SetVisible( false );
	chitBag.SetVisible( false );
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
			CityChit* chit = new CityChit( GetEngine()->GetSpaceTree(), pos, (i==geoMapData.CapitalID(region) ) );
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

	for( int i=0; i<MAX_CONTEXT; ++i )
		context[i].SetVisible( false );

	if ( type == CM_NONE ) {
		for( int i=0; i<MAX_CONTEXT; ++i )
			context[i].SetVisible( false );
		contextChitID = 0;
	}
	else if ( type == CM_BASE ) {
		BaseChit* baseChit = chit->IsBaseChit();

		static const char* base[MAX_CONTEXT] = { "Cargo", "Equip", "Build", 0, 0, 0 };
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

				if (     baseChit->IsFacilityComplete( BaseChit::FACILITY_LANDER )
					  && Unit::Count( baseChit->GetUnits(), MAX_TERRANS, Unit::STATUS_ALIVE )
					  && !chitBag.GetCargoComingFrom( CargoChit::TYPE_LANDER, baseChit->MapPos() ) )
				{
					context[i].SetEnabled( true );
				}
				else {
					context[i].SetEnabled( false );
				}
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
	if ( context[0].Visible() && contextChit ) {

		int size=0;
		while( context[size].Visible() && size < MAX_CONTEXT )
			++size;
		
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
			Chit* chit = new BaseChit( tree, map, index, game->GetItemDefArr(), firstBase );
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

	static const Vector2F pos[GEO_REGIONS] = {
		{ 64.f/1000.f, 9.f/500.f },
		{ 97.f/1000.f, 455.f/500.f },
		{ 502.f/1000.f, 9.f/500.f },
		{ 718.f/1000.f, 106.f/500.f },
		{ 503.f/1000.f, 459.f/500.f },
		{ 744.f/1000.f, 459.f/500.f },
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
						ufo->DoDamage( 1.0f );
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
		// Lander attacks UFO
		Chit* chitAt = chitBag.GetParkedChitAt( landerChit->MapPos() );
		ufoChit = chitAt ? chitAt->IsUFOChit() : 0;
		baseChit = chitBag.GetBaseChitAt( landerChit->Origin() );
	}

	if ( ufoChit ) {
		GLASSERT( baseChit );	// now sure how this is possible, although not destructive
		if ( baseChit ) {
			Unit* units = baseChit->GetUnits();

			TimeState state;
			CalcTimeState( timeline, &state );

			int scenario = TacticalIntroScene::FARM_SCOUT;
			if ( baseAttack ) {
				scenario = TacticalIntroScene::TERRAN_BASE;
			}
			else if ( ufoChit->Type() == UFOChit::BATTLESHIP ) {
				scenario = TacticalIntroScene::BATTLESHIP;
			}
			else {
				int type = geoMapData.GetType( ufoChit->MapPos().x, ufoChit->MapPos().y );
				switch ( type ) {
				case GeoMapData::CITY:		scenario = TacticalIntroScene::CITY;	break;
				case GeoMapData::FARM:		scenario = (ufoChit->Type() == UFOChit::SCOUT ) ? TacticalIntroScene::FARM_SCOUT : TacticalIntroScene::FARM_DESTROYER;	break;
				case GeoMapData::FOREST:	scenario = (ufoChit->Type() == UFOChit::SCOUT ) ? TacticalIntroScene::FRST_SCOUT : TacticalIntroScene::FRST_DESTROYER;	break;
				case GeoMapData::DESERT:	scenario = (ufoChit->Type() == UFOChit::SCOUT ) ? TacticalIntroScene::DSRT_SCOUT : TacticalIntroScene::DSRT_DESTROYER;	break;
				case GeoMapData::TUNDRA:	scenario = (ufoChit->Type() == UFOChit::SCOUT ) ? TacticalIntroScene::TNDR_SCOUT : TacticalIntroScene::TNDR_DESTROYER;	break;
				default: GLASSERT( 0 ); break;
				}
			}

			float rank = state.alienRank;
			if ( scenario == TacticalIntroScene::ALIEN_BASE )
				rank = (float)(NUM_RANKS-1);

			// Bad idea: difficulty of UFOs plenty broad already.
			//if ( TacticalIntroScene::IsScoutScenario( scenario ) )
			//	rank -= 0.5f;	// lower ranks here
			//if ( scenario == TacticalIntroScene::BATTLESHIP )	// battleships are hard enough
			//	rank += 0.5f;
			rank = Clamp( rank, 0.0f, (float)(NUM_RANKS-1) );

			BattleSceneData* data = new BattleSceneData();
			data->seed			= baseChit->MapPos().x + baseChit->MapPos().y*GEO_MAP_X + scenario*37;
			data->scenario		= scenario;
			data->crash			= ( ufoChit->AI() == UFOChit::AI_CRASHED );
			data->soldierUnits	= units;
			data->dayTime		= geoMap->GetDayTime( ufoChit->Pos().x );
			data->alienRank		= rank;
			data->storage		= baseChit->GetStorage();
			chitBag.SetBattle( ufoChit->ID(), landerChit ? landerChit->ID() : 0, scenario );
			game->Save();

			if ( SettingsManager::Instance()->GetUseFastBattle() )
				game->PushScene( Game::FASTBATTLE_SCENE, data );
			else
				game->PushScene( Game::BATTLE_SCENE, data );

		}
	}
}



void GeoScene::ChildActivated( int childID, Scene* childScene, SceneData* data )
{
	if ( childID == Game::BATTLE_SCENE ) {
		TiXmlDocument doc;

		if ( !data ) {
			// Load existing map
			GLASSERT( game->HasSaveFile( SAVEPATH_TACTICAL ));
			if ( game->HasSaveFile( SAVEPATH_TACTICAL ) ) {
				FILE* fp = game->GameSavePath( SAVEPATH_TACTICAL, SAVEPATH_READ );
				GLASSERT( fp );
				if ( fp ) {
					doc.LoadFile( fp );
					fclose( fp );
				}
			}
		}
		else {
			// Create new map.
			GLASSERT( !game->HasSaveFile( SAVEPATH_TACTICAL ) );
			FILE* fp = game->GameSavePath( SAVEPATH_TACTICAL, SAVEPATH_WRITE );
			if ( fp ) {
				TacticalIntroScene::WriteXML( fp, (const BattleSceneData*)data, game->GetItemDefArr(), game->GetDatabase() );
				fclose( fp );

				fp = game->GameSavePath( SAVEPATH_TACTICAL, SAVEPATH_READ );
				if ( fp ) {
					doc.LoadFile( fp );
					fclose( fp );
				}
			}
		}
		GLASSERT( doc.FirstChildElement() );
		GLASSERT( !doc.Error() );
		if ( !doc.Error() ) {
			TiXmlElement* game = doc.FirstChildElement( "Game" );
			GLASSERT( game );
			if ( game ) {
				TiXmlElement* scene = game->FirstChildElement( "BattleScene" );
				GLASSERT( scene );
				if ( scene ) {
					((BattleScene*)childScene)->Load( scene );
				}
			}
		}
	}
}


void GeoScene::SceneResult( int sceneID, int result )
{
	research.KickResearch();
	if ( sceneID == Game::FASTBATTLE_SCENE || sceneID == Game::BATTLE_SCENE ) {

		UFOChit*	ufoChit = chitBag.GetBattleUFO();			// possibly null if UFO destroyed
		CargoChit*	landerChit = chitBag.GetBattleLander();		// will be null if Battleship attacking base

		if ( ufoChit ) {
			BaseChit* baseChit = landerChit ? chitBag.GetBaseChitAt( landerChit->Origin() ) : chitBag.GetBaseChitAt( ufoChit->MapPos() );
			GLASSERT( baseChit );

			if ( baseChit ) {
				Unit* units = baseChit->GetUnits();
				Storage* baseStorage = baseChit->GetStorage();
				Unit newUnits[MAX_TERRANS];
				memset( newUnits, 0, sizeof(Unit)*MAX_TERRANS );

				FILE* fp = game->GameSavePath( SAVEPATH_TACTICAL, SAVEPATH_READ );
				GLASSERT( fp );
				if ( fp ) {

					TiXmlDocument doc;
					doc.LoadFile( fp );
					GLASSERT( !doc.Error() );
					TiXmlHandle docHandle( &doc );

					int scenario = 0;
					const TiXmlElement* battleScene = docHandle.FirstChildElement( "Game" ).FirstChildElement( "BattleScene" ).ToElement();
					if ( battleScene ) {
						battleScene->QueryIntAttribute( "scenario", &scenario );
					}
					// Special case: can't tie a base attack.
					if ( !landerChit && (result == TacticalEndSceneData::TIE) ) {
						result = TacticalEndSceneData::DEFEAT;
					}

					int unitCount=0;
					int nAliensAlive = 0;
					int nAliensDead = 0;
					int nCivsAlive = 0;
					int nCivsDead = 0;
					Storage foundStorage( 0, 0, game->GetItemDefArr() );

					// Scan and load all the units.
					const TiXmlElement* unitsEle = docHandle.FirstChild( "Game" ).FirstChild( "BattleScene" ).FirstChild( "Units" ).ToElement();
					if ( unitsEle && ( result != TacticalEndSceneData::DEFEAT ) ) {
						for( const TiXmlElement* unitEle=unitsEle->FirstChildElement( "Unit" ); unitEle; unitEle=unitEle->NextSiblingElement( "Unit" ) ) {

							int team, status;
							unitEle->QueryIntAttribute( "team", &team );
							unitEle->QueryIntAttribute( "status", &status );

							if ( team == TERRAN_TEAM ) {
								if ( status == Unit::STATUS_ALIVE || status == Unit::STATUS_UNCONSCIOUS ) {
									newUnits[unitCount].Load( unitEle, game->GetItemDefArr() );
									newUnits[unitCount].Heal();
									newUnits[unitCount].GetInventory()->RestoreClips();
									unitCount++;
								}
							}
							else if ( team == CIV_TEAM ) {
								if ( status == Unit::STATUS_ALIVE )		++nCivsAlive;
								else if ( status ==  Unit::STATUS_KIA ) ++nCivsDead;
							}
							else if ( team == ALIEN_TEAM ) {
								if ( status == Unit::STATUS_ALIVE )		++nAliensAlive;
								else if ( status ==  Unit::STATUS_KIA ) ++nAliensDead;
							}
						}
					}

					if ( result != TacticalEndSceneData::DEFEAT ) {
						// Go through items, and collect them up. If the debris is at a unit location, add
						// it back to the unit.
						const TiXmlElement* groundDebris = docHandle.FirstChild( "Game" ).FirstChild( "BattleScene" ).FirstChild( "GroundDebris" ).ToElement();
						if ( groundDebris ) {
							for( const TiXmlElement* debris = groundDebris->FirstChildElement( "Debris" ); debris; debris=debris->NextSiblingElement( "Debris" ) ) {
								const TiXmlElement* storageEle = debris->FirstChildElement( "Storage" );
								if ( storageEle ) {
									Storage storage( 0, 0, game->GetItemDefArr() );
									storage.Load( debris );
									storage.SetFullRounds();	// refill clips

									foundStorage.AddStorage( storage );
								}
							}
						}
					}

					for( int i=0; i<unitCount; ++i ) {
						if ( newUnits[i].GetInventory()->Empty() ) {
							const Unit* oldUnit = Unit::Find( units, MAX_TERRANS, newUnits[unitCount].Body() );
							if ( oldUnit ) {
								for( int k=0; k<Inventory::NUM_SLOTS; ++k ) {
									const ItemDef* itemDef = oldUnit->GetInventory()->GetItemDef( k );
									if ( itemDef ) {
										Item item;
										foundStorage.RemoveItem( itemDef, &item );
										newUnits[unitCount].GetInventory()->AddItem( k, item );
									}
								}
							}
						}
						unitCount++;
					}


					for( int i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
						const ItemDef* itemDef = foundStorage.GetItemDef(i);
						if ( itemDef && foundStorage.GetCount( i )) {
							research.SetItemAcquired( itemDef->name );
						}
					}
					research.KickResearch();

					if ( result == TacticalEndSceneData::VICTORY ) {

						// Apply penalty for lost civs:
						int	region = geoMapData.GetRegion( baseChit->MapPos().x, baseChit->MapPos().y );
						if ( (nCivsAlive+nCivsDead) > 0 && region >= 0 ) {
							float ratio = 1.0f - (float)nCivsAlive / (float)(nCivsAlive+nCivsDead);
							if ( chitBag.GetBattleScenario() == TacticalIntroScene::CITY ) { 
								ratio *= 2.0f; 
							}
							regionData[region].influence += ratio;
							regionData[region].influence = Min( regionData[region].influence, (float)MAX_INFLUENCE );
							areaWidget[region]->SetInfluence( regionData[region].influence );
						}

						delete ufoChit;
					}

					if ( result == TacticalEndSceneData::DEFEAT ) {
						// Did the terrans lose a base attack?
						if ( !landerChit ) {
							baseChit->SetDestroyed();
							ufoChit->SetAI( UFOChit::AI_ORBIT );
						}
					}

					// Copy units over.
					for( int i=0; i<MAX_TERRANS; ++i ) {
						units[i].Free();
					}
					memset( units, 0, MAX_TERRANS*sizeof(Unit) );
					memcpy( units, newUnits, unitCount*sizeof(Unit) );
				}
			}
		}
		game->Save();
		game->DeleteSaveFile( SAVEPATH_TACTICAL );
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
	regionData[region].SetStorageNormal( research, &data->region );
	data->cash		 = &cash;
	data->costMult	 = regionData[region].traits & RegionData::TRAIT_CAPATALIST ? COST_MULT_CAP : COST_MULT_STD;
	data->soldierBoost = regionData[region].traits & RegionData::TRAIT_MILITARISTIC ? true : false;
	data->soldiers	 = baseChit->GetUnits();
	data->scientists = baseChit->IsFacilityComplete( BaseChit::FACILITY_SCILAB ) ? baseChit->GetResearcherPtr() : 0;
	game->PushScene( Game::BASETRADE_SCENE, data );
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
				nResearchers += chitBag.GetBaseChit(i)->NumResearchers();				
				
				int region = geoMapData.GetRegion( baseChit->MapPos() );
				if ( regionData[region].traits & RegionData::TRAIT_SCIENTIFIC ) {
					nResearchers += chitBag.GetBaseChit(i)->NumResearchers() * 3 / 2;	// 50% bonus
				}
			}
		}
		research.DoResearch( nResearchers );
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

	if ( SettingsManager::Instance()->GetBattleShipParty() ) {
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
		Vector2I pos = chitIt->MapPos();

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
				int region = geoMapData.GetRegion( pos.x, pos.y );
				int type = geoMapData.GetType( pos.x, pos.y );

				bool returnToOrbit = false;

				// If the destination space is taken, return to orbit.
				Chit* parked = chitBag.GetParkedChitAt( pos );
				if ( parked ) {
					returnToOrbit = true;
				}

				if ( returnToOrbit ) {
					ufoChit->SetAI( UFOChit::AI_ORBIT );
				}
				else if (      ufoChit->Type() == UFOChit::BATTLESHIP 
							&& chitBag.GetBaseChitAt( pos )
							&& regionData[region].influence >= MIN_BASE_ATTACK_INFLUENCE )
				{
					if ( !game->IsScenePushed() ) {
						DoBattle( 0, chitIt->IsUFOChit() );
					}
					ufoChit->SetAI( UFOChit::AI_ORBIT );
				}
				else if (    ufoChit->Type() == UFOChit::BATTLESHIP 
						    && !regionData[region].occupied
						    && geoMapData.Capital( region ) == pos 
							&& regionData[region].influence >= MIN_OCCUPATION_INFLUENCE )
				{
					regionData[region].influence = MAX_INFLUENCE;
					regionData[region].occupied = true;
					areaWidget[region]->SetInfluence( regionData[region].influence );
					ufoChit->SetAI( UFOChit::AI_OCCUPATION );
				}
				else if (    ( ufoChit->Type() == UFOChit::BATTLESHIP || ufoChit->Type() == UFOChit::FRIGATE )
						    && !regionData[region].occupied
						    && geoMapData.IsCity( pos.x, pos.y )
							&& regionData[region].influence >= MIN_CITY_ATTACK_INFLUENCE ) 
				{
					ufoChit->SetAI( UFOChit::AI_CITY_ATTACK );
				}
				else if (    !geoMapData.IsCity( pos.x, pos.y ) 
						   && !chitBag.GetBaseChitAt( pos )) 
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
				Chit *chit = new CropCircle( tree, pos, random.Rand() );
				chitBag.Add( chit );

				int region = geoMapData.GetRegion( pos.x, pos.y );
				if ( regionData[region].influence < MAX_CROP_CIRCLE_INFLUENCE ) {
					regionData[region].influence += (float)CROP_CIRCLE_INFLUENCE * influenceModifier;
					regionData[region].influence = Min( regionData[region].influence, (float)MAX_INFLUENCE );
					areaWidget[region]->SetInfluence( regionData[region].influence );
				}

				if ( !geoMapData.IsWater( pos.x, pos.y ) ) {
					int region = geoMapData.GetRegion( pos.x, pos.y );
					regionData[region].Success();
				}
			}
			break;

			case Chit::MSG_UFO_CRASHED:
			{
				// Can only crash on open space.
				// Check for UFOs, bases.
				bool parked = false;
				Chit* parkedChit = chitBag.GetParkedChitAt( pos );
				BaseChit* baseChit = chitBag.GetBaseChitAt( pos );

				if( ( parkedChit && (parkedChit != chitIt ) ) || baseChit ) {
					parked = true;
				}
				// check for water, cities
				int mapType = geoMapData.GetType( pos.x, pos.y );

				if ( mapType == 0 || mapType == GeoMapData::CITY ) {
					parked = true;
				}

				if ( !parked ) {
					chitIt->IsUFOChit()->SetAI( UFOChit::AI_CRASHED );
					chitIt->SetMapPos( pos.x, pos.y );
				}
				else {
					// Find bases with units. Award "gunner" XP.
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


					chitIt->SetDestroyed();
				}

				if ( !geoMapData.IsWater( pos.x, pos.y ) ) {
					int region = geoMapData.GetRegion( pos.x, pos.y );
					regionData[region].UFODestroyed();
				}
			}
			break;


			case Chit::MSG_CITY_ATTACK_COMPLETE:
			{
				int region = geoMapData.GetRegion( pos.x, pos.y );
				regionData[region].influence += (float)CITY_ATTACK_INFLUENCE * influenceModifier;
				regionData[region].influence = Min( regionData[region].influence, (float)MAX_INFLUENCE );
				areaWidget[region]->SetInfluence( regionData[region].influence );

				if ( !geoMapData.IsWater( pos.x, pos.y ) ) {
					int region = geoMapData.GetRegion( pos.x, pos.y );
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
	if ( contextChitID && !chitBag.GetChit( contextChitID ) ) {
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
			if ( !regionData[i].occupied )
				break;
		}
		if ( i == GEO_REGIONS ) {
			GeoEndSceneData* data = new GeoEndSceneData();
			data->victory = false;
			game->PushScene( Game::GEO_END_SCENE, data );
		}
	}
}


void GeoScene::Debug3D()
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
	TimeState ts;
	CalcTimeState( timeline, &ts );
	UFOText::Draw( 50, 0, "nBat=%d tmr=%.1fm alTm=%.1f rank=%.1f type=%.1f,%.1f,%.1f",
		nBattles,
		(float)(timeline / 1000) / 60.0f,
		(float)ts.alienTime/1000.0f,
		ts.alienRank,
		ts.alienType[0], ts.alienType[1], ts.alienType[2] );
}


void GeoScene::CalcTimeState( U32 msec, TimeState* state )
{
	// Times in seconds.
	const static U32 FIRST_UFO      = 10 *SECOND;
	const static U32 TIME_BETWEEN_0	= 20 *SECOND;
	const static U32 TIME_BETWEEN_1 = 5  *SECOND;

	const static U32 DESTROYER0		= 2  *MINUTE;
	const static U32 DESTROYER1		= 5  *MINUTE;
	const static U32 BATTLESHIP		= 10 *MINUTE;

	state->alienTime = (U32)Interpolate( 0.0, (double)(TIME_BETWEEN_0), (double)FULL_OUT_UFO, (double)(TIME_BETWEEN_1), (double)msec );
	if ( state->alienTime < TIME_BETWEEN_1 )
		state->alienTime = TIME_BETWEEN_1;
	if ( msec <= FIRST_UFO+1000 )	// rounding so we aren't racing this timer
		state->alienTime = FIRST_UFO;	

	state->alienRank = (float)Interpolate( 0.0, 0.0, (double)FULL_OUT_RANK, (double)(NUM_RANKS)-1.5, (double)msec );	// the minus one is float/int conversion, the 0.5 is not maxing out the alien rank
	if ( state->alienRank > (float)(NUM_RANKS-1) )
		state->alienRank = (float)(NUM_RANKS-1);

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


void GeoScene::Save( FILE* fp, int depth )
{
	// Scene
	// GeoScene
	//		RegionData
	//		Chits
	//		Research
	// Units

	// ----------- main scene --------- //
	XMLUtil::OpenElement( fp, depth, "GeoScene" );
	XMLUtil::Attribute( fp, "timeline", timeline );
	XMLUtil::Attribute( fp, "alienTimer", alienTimer );
	XMLUtil::Attribute( fp, "missileTimer0", missileTimer[0] );
	XMLUtil::Attribute( fp, "missileTimer1", missileTimer[1] );
	XMLUtil::Attribute( fp, "researchTimer", researchTimer );
	XMLUtil::Attribute( fp, "cash", cash );
	XMLUtil::Attribute( fp, "firstBase", firstBase );
	XMLUtil::Attribute( fp, "nBattles", nBattles );
	XMLUtil::SealElement( fp );

	// ---------- regions ----------- //
	XMLUtil::OpenElement( fp, depth+1, "RegionData" );
	XMLUtil::SealElement( fp );
	for( int i=0; i<GEO_REGIONS; ++i ) {
		regionData[i].Save( fp, depth+2 );
	}
	XMLUtil::CloseElement( fp, depth+1, "RegionData" );

	// ----------- chits ---------- //
	chitBag.Save( fp, depth+1 );
	// ----------- research ------- //
	research.Save( fp, depth+1 );

	XMLUtil::CloseElement( fp, depth, "GeoScene" );
}


void GeoScene::Load( const TiXmlElement* scene )
{
	GLASSERT( StrEqual( scene->Value(), "GeoScene" ) );
	scene->QueryUnsignedAttribute( "timeline", &timeline );
	scene->QueryUnsignedAttribute( "alienTimer", &alienTimer );
	scene->QueryUnsignedAttribute( "missileTimer0", &missileTimer[0] );
	scene->QueryUnsignedAttribute( "missileTimer1", &missileTimer[1] );
	scene->QueryUnsignedAttribute( "researchTimer", &researchTimer );
	scene->QueryIntAttribute( "cash", &cash );
	scene->QueryBoolAttribute( "firstBase", &firstBase );
	scene->QueryIntAttribute( "nBattles", &nBattles );

	int i=0;
	const TiXmlElement* rdEle = scene->FirstChildElement( "RegionData" );
	if ( rdEle ) {
		for( const TiXmlElement* region=rdEle->FirstChildElement("Region"); region; ++i, region=region->NextSiblingElement( "Region" ) ) {
			regionData[i].Load( region );
			areaWidget[i]->SetInfluence( (float)regionData[i].influence );
			areaWidget[i]->SetTraits( regionData[i].traits );
		}
	}
	chitBag.Load( scene, tree, game->GetItemDefArr(), game );
	research.Load( scene->FirstChildElement( "Research" ) );

	GenerateCities();

	if ( game->HasSaveFile( SAVEPATH_TACTICAL ) ) {
		// Push here. Do load in ChildActivated(). The data
		// must be null, and the file has to exist.
		game->PushScene( Game::BATTLE_SCENE, 0 );
	}
}

