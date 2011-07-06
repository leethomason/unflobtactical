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

#include <time.h>

#include "../grinliz/glbitarray.h"
#include "../grinliz/glstringutil.h"
#include "tacticalintroscene.h"
#include "../engine/uirendering.h"
#include "../engine/engine.h"
#include "game.h"
#include "cgame.h"
#include "helpscene.h"
#include "settings.h"
#include "ai.h"
#include "../version.h"
#include "tacmap.h"


using namespace grinliz;
using namespace gamui;


TacticalIntroScene::TacticalIntroScene( Game* _game ) : Scene( _game )
{
	Engine* engine = GetEngine();
	const Screenport& port = engine->GetScreenport();
	random.SetSeedFromTime();

	GLOUTPUT(( "TacticalIntroScene screen=%.1f,%.1f\n", engine->GetScreenport().UIWidth(), engine->GetScreenport().UIHeight() ));

	// -- Background -- //
	gamui::RenderAtom nullAtom;

	backgroundUI.Init( game, &gamui2D, true );
	//backgroundUI.backgroundText.SetPos( 200, backgroundUI.background.Y() );

	const gamui::ButtonLook& green = game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );
	//const gamui::ButtonLook& red = game->GetButtonLook( Game::RED_BUTTON );

	static const float BORDER = 25;

	continueButton.Init( &gamui2D, green );
	continueButton.SetSizeByScale( 2.2f, 1 );
	continueButton.SetPos( BORDER, 320-BORDER-continueButton.Height() );
	continueButton.SetText( "Continue" );

	newTactical.Init( &gamui2D, green );
	newTactical.SetSizeByScale( 2.2f, 1 );
	newTactical.SetPos( port.UIWidth()-BORDER-newTactical.Width(), 320-BORDER-continueButton.Height() );
	newTactical.SetText( "New Tactical" );
	
	newGeo.Init( &gamui2D, green );
	newGeo.SetPos( port.UIWidth()-BORDER*2-newTactical.Width()*2, 320-BORDER-continueButton.Height() );
	newGeo.SetSizeByScale( 2.2f, 1 );
	newGeo.SetText( "New Geo" );
	
	// Same place as new geo
	newGame.Init( &gamui2D, blue );
	newGame.SetSizeByScale( 2.2f, 1 );
	newGame.SetPos( newGeo.X(), newGeo.Y() );
	newGame.SetText( "New Game" );

	newGameWarning.Init( &gamui2D );
	newGameWarning.SetPos( newGeo.X()+5, newGeo.Y() + newGeo.Height() + 5 );
	
	if ( game->HasSaveFile( SAVEPATH_GEO ) ) {
		newGeo.SetVisible( false );
		newTactical.SetVisible( false );
		newGame.SetVisible( true );
		//newGameWarning.SetText( "Geo game in progress." );
		newGameWarning.SetText( "'New' deletes current Geo game." );
	}
	else if ( game->HasSaveFile( SAVEPATH_TACTICAL ) ) {
		newGeo.SetVisible( false );
		newTactical.SetVisible( false );
		//newGame.SetVisible( true );
		newGameWarning.SetText( "'New' deletes current Tactical game." );
	}
	else {
		newGame.SetVisible( false );
		newGameWarning.SetVisible( false );
	}

	helpButton.Init( &gamui2D, green );
	helpButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_HELP, true ),
						UIRenderer::CalcDecoAtom( DECO_HELP, false ) );	

	audioButton.Init( &gamui2D, green );
	SettingsManager* settings = SettingsManager::Instance();
	if ( settings->GetAudioOn() ) {
		audioButton.SetDown();
		audioButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_AUDIO, true ),
							 UIRenderer::CalcDecoAtom( DECO_AUDIO, false ) );	
	}
	else {
		audioButton.SetUp();
		audioButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_MUTE, true ),
							 UIRenderer::CalcDecoAtom( DECO_MUTE, false ) );	
	}

	//infoButton.Init( &gamui2D, green );
	//infoButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_INFO, true ),
	//					UIRenderer::CalcDecoAtom( DECO_INFO, false ) );	

	settingButton.Init( &gamui2D, green );
	settingButton.SetDeco(	UIRenderer::CalcDecoAtom( DECO_SETTINGS, true ),
							UIRenderer::CalcDecoAtom( DECO_SETTINGS, false ) );	

	static const int NUM_ITEMS=3;
	UIItem* items[NUM_ITEMS] = { &helpButton, &audioButton, &settingButton };
	Gamui::Layout( items, NUM_ITEMS, 1, NUM_ITEMS, port.UIWidth() - helpButton.Width() - BORDER, BORDER, helpButton.Width(), helpButton.Height()*(float)NUM_ITEMS+BORDER );


	static const char* toggleLabel[TOGGLE_COUNT] = { "4", "6", "8", "Low", "Med", "Hi", 
													 //"8", "12", "16", 
													 "Low", "Med", "Hi", "Day", "Night",
													 "Fa-S", "T-S", "Fo-S", "D-S", "Fa-D", "T-D", "Fo-D", "D-D",
													 "City", "BattleShip",	"AlienBase", "TerranBase",
													 //"Civs", 
													 "Crash" };
	for( int i=0; i<TOGGLE_COUNT; ++i ) {
		GLASSERT( toggleLabel[i] );
		toggles[i].Init( &gamui2D, ( i < UFO_CRASH ) ? green : blue );
		toggles[i].SetText( toggleLabel[i] );
		toggles[i].SetVisible( false );
		toggles[i].SetSize( 50, 50 );
	}
	toggles[BATTLESHIP].SetSize( 100, 50 );
	toggles[ALIEN_BASE].SetSize( 100, 50 );
	toggles[TERRAN_BASE].SetSize( 100, 50 );

	toggles[SQUAD_4].AddToToggleGroup( &toggles[SQUAD_6] );
	toggles[SQUAD_4].AddToToggleGroup( &toggles[SQUAD_8] );

	toggles[TERRAN_LOW].AddToToggleGroup( &toggles[TERRAN_MED] );
	toggles[TERRAN_LOW].AddToToggleGroup( &toggles[TERRAN_HIGH] );

	toggles[ALIEN_LOW].AddToToggleGroup( &toggles[ALIEN_MED] );
	toggles[ALIEN_LOW].AddToToggleGroup( &toggles[ALIEN_HIGH] );

	toggles[TIME_DAY].AddToToggleGroup( &toggles[TIME_NIGHT] );

	toggles[SQUAD_8].SetDown();
	toggles[TERRAN_MED].SetDown();
	toggles[ALIEN_MED].SetDown();
	toggles[TIME_DAY].SetDown();
	toggles[FARM_SCOUT].SetDown();

	terranLabel.Init( &gamui2D );
	terranLabel.SetVisible( false );
	terranLabel.SetText( "Terran Squad" );
	terranLabel.SetPos( 20, 25-20 );

	UIItem* squadItems[] = { &toggles[SQUAD_4], &toggles[SQUAD_6], &toggles[SQUAD_8] };
	Gamui::Layout(	squadItems, 3,			// squad #
					4, 1, 
					20, 25,
					150, 50 );

	UIItem* squadStrItems[] = { &toggles[TERRAN_LOW], &toggles[TERRAN_MED], &toggles[TERRAN_HIGH] };
	Gamui::Layout(	squadStrItems, 3,			// squad strength
					4, 1, 
					20, 75,
					150, 50 );

	alienLabel.Init( &gamui2D );
	alienLabel.SetVisible( false );
	alienLabel.SetText( "Aliens" );
	alienLabel.SetPos( 20, 150-20 );

	//UIItem* alienItems[] = { &toggles[ALIEN_8], &toggles[ALIEN_12], &toggles[ALIEN_16] };
	//Gamui::Layout(	alienItems, 3,			// alien #
	//				4, 1, 
	//				20, 150,
	//				150, 50 );
	UIItem* alienStrItems[] = { &toggles[ALIEN_LOW], &toggles[ALIEN_MED], &toggles[ALIEN_HIGH] };
	Gamui::Layout(	alienStrItems, 3,			// alien strength
					4, 1, 
					20, 150,
					150, 50 );

	timeLabel.Init( &gamui2D );
	timeLabel.SetVisible( false );
	timeLabel.SetText( "Time" );
	timeLabel.SetPos( 20, 270-20 );
	UIItem* weatherItems[] = { &toggles[TIME_DAY], &toggles[TIME_NIGHT] };
	Gamui::Layout(	weatherItems, 2,		// weather
					2, 1, 
					20, 270,
					100, 50 );

	for( int i=0; i<3; ++i ) {
		static const char* row[] = { "Scout", "Frigate", "Special" };
		rowLabel[i].Init( &gamui2D );
		rowLabel[i].SetVisible( false );
		rowLabel[i].SetText( row[i] );
		rowLabel[i].SetPos( 425, 25.0f + 50.0f*i+15.0f );
	}

	static const float SIZE = 50;
	{
		scenarioLabel.Init( &gamui2D );
		scenarioLabel.SetVisible( false );
		scenarioLabel.SetText( "Farm Tundra Forest Desert" );
		scenarioLabel.SetPos( 215, 5 );
	}

	for( int i=0; i<4; ++i ) {
		toggles[FARM_SCOUT+i].SetPos( 215+SIZE*i, 25 );
		toggles[FARM_DESTROYER+i].SetPos( 215+SIZE*i, 75 );
	}
	toggles[CITY].SetPos( 215+SIZE*0, 125 );
	toggles[BATTLESHIP].SetPos( 215+SIZE*1, 125 );
	toggles[ALIEN_BASE].SetPos( 215+SIZE*0, 175 );
	toggles[TERRAN_BASE].SetPos( 215+SIZE*2, 175 );

	for( int i=FIRST_SCENARIO; i<=LAST_SCENARIO; ++i )
		toggles[FARM_SCOUT].AddToToggleGroup( &toggles[i] );

	toggles[FARM_SCOUT].SetEnabled(		true );
	toggles[TNDR_SCOUT].SetEnabled(		true );
	toggles[FRST_SCOUT].SetEnabled(		true );
	toggles[DSRT_SCOUT].SetEnabled(		true );
	toggles[FARM_DESTROYER].SetEnabled( true );
	toggles[TNDR_DESTROYER].SetEnabled( true );
	toggles[FRST_DESTROYER].SetEnabled( true );
	toggles[DSRT_DESTROYER].SetEnabled( true );
	toggles[CITY].SetEnabled(			true );
	toggles[BATTLESHIP].SetEnabled(		true );


	UIItem* scenItems[] = { /*&toggles[CIVS_PRESENT],*/ &toggles[UFO_CRASH] };
	Gamui::Layout( scenItems, 1,
				   4, 2,
				   215, 225,
				   100, 100 );
							
	goButton.Init( &gamui2D, blue );
	goButton.SetPos( 360, 270 );
	goButton.SetSize( 100, 50 );
	goButton.SetText( "Go!" );
	goButton.SetVisible( false );

	// Is there a current game?
	continueButton.SetEnabled( game->HasSaveFile( SAVEPATH_GEO ) || game->HasSaveFile( SAVEPATH_TACTICAL ) );
}


TacticalIntroScene::~TacticalIntroScene()
{
}


void TacticalIntroScene::DrawHUD()
{
}


void TacticalIntroScene::Tap(	int action, 
								const Vector2F& screen,
								const Ray& world )
{
	int onToNext = -1;
	Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( screen, &ui );
	
	const gamui::UIItem* item = 0;

	if ( action == GAME_TAP_DOWN ) {
		gamui2D.TapDown( ui.x, ui.y );
		return;
	}
	else if ( action == GAME_TAP_MOVE ) {
		return;
	}
	else if ( action == GAME_TAP_UP ) {
		item = gamui2D.TapUp( ui.x, ui.y );
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		return;
	}

	if ( item == &newGame ) {
		newGame.SetVisible( false );
		newGameWarning.SetVisible( false );
		newGeo.SetVisible( true );
		newTactical.SetVisible( true );
	}
	else if ( item == &newTactical ) {
		newTactical.SetVisible( false );
		newGeo.SetVisible( false );
		continueButton.SetVisible( false );
		for( int i=0; i<TOGGLE_COUNT; ++i ) {
			toggles[i].SetVisible( true );
		}
		goButton.SetVisible( true );
		backgroundUI.backgroundText.SetVisible( false );

		terranLabel.SetVisible( true );
		alienLabel.SetVisible( true );
		timeLabel.SetVisible( true );
		scenarioLabel.SetVisible( true );
		for( int i=0; i<3; ++i )
			rowLabel[i].SetVisible( true );
		
		helpButton.SetVisible( false );
		audioButton.SetVisible( false );
		//infoButton.SetVisible( false );
		settingButton.SetVisible( false );

		game->DeleteSaveFile( SAVEPATH_TACTICAL );
		game->DeleteSaveFile( SAVEPATH_GEO );
	}
	else if ( item == &continueButton ) {
		if ( game->HasSaveFile( SAVEPATH_GEO ) )
			onToNext = Game::GEO_SCENE;
		else if ( game->HasSaveFile( SAVEPATH_TACTICAL ) )
			onToNext = Game::BATTLE_SCENE;
	}
	else if ( item == &goButton ) {
		FILE* fp = game->GameSavePath( SAVEPATH_TACTICAL, SAVEPATH_WRITE );
		GLASSERT( fp );
		if ( fp ) {
			onToNext = Game::BATTLE_SCENE;

			BattleSceneData data;
			data.seed = random.Rand();
			data.scenario = FARM_SCOUT;
			for( int i=FIRST_SCENARIO; i<=LAST_SCENARIO; ++i ) {
				if ( toggles[i].Down() ) {
					data.scenario = i;
					break;
				}
			}
			data.crash = toggles[UFO_CRASH].Down() ? true : false;

			Unit units[MAX_TERRANS];
			int rank = 0;
			int count = 8;

			// Terran units
			if ( toggles[TERRAN_LOW].Down() )
				rank = 0;
			else if ( toggles[TERRAN_MED].Down() )
				rank = 2;
			else if ( toggles[TERRAN_HIGH].Down() )
				rank = 4;

			if ( toggles[SQUAD_4].Down() )
				count = 4;
			else if ( toggles[SQUAD_6].Down() )
				count = 6;
			else if ( toggles[SQUAD_8].Down() )
				count = 8;

			GenerateTerranTeam( units, count, (float)rank, game->GetItemDefArr(), random.Rand() );
			data.soldierUnits = units;
			data.nScientists = 8;

			data.dayTime = toggles[TIME_DAY].Down() ? true : false;
			data.alienRank = 0.0f;
			if ( toggles[ALIEN_LOW].Down() ) {
				data.alienRank = 0;
			}
			else if ( toggles[ALIEN_MED].Down() ) {
				data.alienRank = 2;
			}
			else if ( toggles[ALIEN_HIGH].Down() ) {
				data.alienRank = 4;
			}
			data.storage = 0;

			WriteXML( fp, &data, game->GetItemDefArr(), game->GetDatabase() );
			fclose( fp );
		}
	}
	else if ( item == &helpButton ) {
		game->PushScene( Game::HELP_SCENE, new HelpSceneData("introHelp" ));
	}
	else if ( item == &audioButton ) {
		SettingsManager* settings = SettingsManager::Instance();
		if ( audioButton.Down() ) {
			settings->SetAudioOn( true );
			audioButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_AUDIO, true ),
								 UIRenderer::CalcDecoAtom( DECO_AUDIO, false ) );	
		}
		else {
			settings->SetAudioOn( false );
			audioButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_MUTE, true ),
								 UIRenderer::CalcDecoAtom( DECO_MUTE, false ) );	
		}
	}
//	else if ( item == &infoButton ) {
//		game->SetDebugLevel( (game->GetDebugLevel() + 1)%4 );
//	}
	else if ( item == &settingButton ) {
		game->PushScene( Game::SETTING_SCENE, 0 );
	}
	else if ( item == &newGeo ) {
		game->DeleteSaveFile( SAVEPATH_GEO );
		game->DeleteSaveFile( SAVEPATH_TACTICAL );
		onToNext = Game::GEO_SCENE;
	}

	if ( onToNext >= 0 ) {
		game->PopScene();
		game->PushScene( onToNext, 0 );
	}
}


/*static*/ void TacticalIntroScene::WriteXML( FILE* fp, const BattleSceneData* data, const ItemDefArr& itemDefArr, const gamedb::Reader* database )
 {
	//	Game
	//		BattleScene
	//		Map
	//			Seen
	//			Items
	//			Images
	//			GroundDebris
	//		Units
	//			Unit
	
	XMLUtil::OpenElement( fp, 0, "Game" );
	XMLUtil::Attribute( fp, "version", VERSION );
	XMLUtil::Attribute( fp, "sceneID", Game::BATTLE_SCENE );
	XMLUtil::SealElement( fp );

	XMLUtil::OpenElement( fp, 1, "BattleScene" );
	XMLUtil::Attribute( fp, "dayTime", data->dayTime ? 1 : 0 );
	XMLUtil::Attribute( fp, "scenario", data->scenario );
	XMLUtil::SealElement( fp );

	Random random;
	random.SetSeedFromTime();

	int nCivs = ( data->scenario == TacticalIntroScene::TERRAN_BASE ) ? data->nScientists : CivsInScenario( data->scenario );
	SceneInfo info( data->scenario, data->crash, nCivs );

	CreateMap( fp, random.Rand(), info, database );
	fprintf( fp, "\n" );

	BattleData battleData( itemDefArr );
	battleData.dayTime = data->dayTime;
	battleData.scenario = data->scenario;

	// Terran soldier units.
	//memcpy( &battleData.units[TERRAN_UNITS_START], data->soldierUnits, sizeof(Unit)*MAX_TERRANS );
	for( int i=0; i<MAX_TERRANS; ++i ) {
		battleData.units[TERRAN_UNITS_START+i] = data->soldierUnits[i];
	}

	// Alien units
	GenerateAlienTeamUpper( data->scenario, data->crash, data->alienRank, &battleData.units[ALIEN_UNITS_START], itemDefArr, random.Rand() );

	// Civ team
	GenerateCivTeam( &battleData.units[CIV_UNITS_START], info.nCivs, itemDefArr, random.Rand() );

	battleData.Save( fp, 1 );
	XMLUtil::CloseElement( fp, 1, "BattleScene" );
	XMLUtil::CloseElement( fp, 0, "Game" );		
}


void TacticalIntroScene::FindNodes(	const char* set,
									int size,
									const char* type,
									const gamedb::Item* parent,
									const gamedb::Item** itemMatch,
									int *nMatch )
{
	char buffer[64];
	SNPrintf( buffer, 64, "%4s_%02d_%4s", set, size, type );
	const int LEN=12;
	*nMatch = 0;

	for( int i=0; i<parent->NumChildren(); ++i ) {
		const gamedb::Item* node = parent->Child( i );
		const char* name = node->Name();

		if ( strncmp( name, buffer, LEN ) == 0 ) {
			itemMatch[ (*nMatch)++ ] = node;
		}
	}
}


void TacticalIntroScene::AppendMapSnippet(	int dx, int dy, int tileRotation,
											const char* set,
											int size,
											bool crash,
											const char* type,
											const gamedb::Reader* database,
											const gamedb::Item* parent,
											TiXmlElement* mapElement,
											int _seed )
{
	const gamedb::Item* itemMatch[ MAX_ITEM_MATCH ];
	int nItemMatch = 0;

	FindNodes( set, size, type, parent, itemMatch, &nItemMatch );
	GLASSERT( nItemMatch > 0 );
	
	Random random( _seed );
	random.Rand();
	int seed = random.Rand( nItemMatch );
	const gamedb::Item* item = itemMatch[ seed ];

	const char* xmlText = (const char*) database->AccessData( item, "binary" );

	TiXmlDocument snippet;
	snippet.Parse( xmlText );
	GLASSERT( !snippet.Error() );

	TiXmlElement* itemsElement = mapElement->FirstChildElement( "Items" );
	if ( !itemsElement ) {
		itemsElement = new TiXmlElement( "Items" );
		mapElement->LinkEndChild( itemsElement );
	}

	// Append the <Items>, account for (x,y) changes.
	// Append the <Images>, account for (x,y) changes.
	// Append one sub tree to the other. Adjust x, y as we go.
	Matrix2I m;
	Map::MapImageToWorld( dx, dy, size, size, tileRotation, &m );

	Rectangle2I crashRect, crashRect0;
	crashRect.Set( 0, 0, 0, 0 );
	if ( crash ) {
		int half = size/2;
		crashRect.min.Set( random.Rand( half ), random.Rand( half ) );
		crashRect.max.x = crashRect.min.x + half;
		crashRect.max.y = crashRect.min.y + half;
	}
	Vector2I cr0 = m * crashRect.min;
	Vector2I cr1 = m * crashRect.max;
	crashRect0.FromPair( cr0.x, cr0.y, cr1.x, cr1.y );

	for( TiXmlElement* ele = snippet.FirstChildElement( "Map" )->FirstChildElement( "Items" )->FirstChildElement( "Item" );
		 ele;
		 ele = ele->NextSiblingElement() )
	{
		Vector2I v = { 0, 0 };
		int rot = 0;
		ele->QueryIntAttribute( "x", &v.x );
		ele->QueryIntAttribute( "y", &v.y );
		ele->QueryIntAttribute( "rot", &rot );

		Vector2I v0 = m * v;

		ele->SetAttribute( "x", v0.x );
		ele->SetAttribute( "y", v0.y );
		ele->SetAttribute( "rot", (rot + tileRotation)%4 );

		if ( crashRect.Contains( v.x, v.y ) && random.Bit() ) {
			const MapItemDef* mapItemDef = TacMap::StaticGetItemDef( ele->Attribute( "name" ) );
			if ( mapItemDef && mapItemDef->CanDamage() ) {
				ele->SetAttribute( "hp", 0 );
			}
		}

		itemsElement->InsertEndChild( *ele );
	}

	// And add the image data
	TiXmlElement* imagesElement = mapElement->FirstChildElement( "Images" );
	if ( !imagesElement ) {
		imagesElement = new TiXmlElement( "Images" );
		mapElement->LinkEndChild( imagesElement );
	}

	char buffer[64];
	SNPrintf( buffer, 64, "%4s_%02d_%4s_%02d", set, size, type, seed );

	TiXmlElement* image = new TiXmlElement( "Image" );
	image->SetAttribute( "name", buffer );
	image->SetAttribute( "x", dx );
	image->SetAttribute( "y", dy );
	image->SetAttribute( "size", size );
	image->SetAttribute( "tileRotation", tileRotation );
	imagesElement->LinkEndChild( image );

	if ( crash ) {
		TiXmlElement* pgElement = mapElement->FirstChildElement( "PyroGroup" );
		if ( !pgElement ) {
			pgElement = new TiXmlElement( "PyroGroup" );
			mapElement->LinkEndChild( pgElement );
		}
		for( int j=crashRect0.min.y; j<=crashRect0.max.y; ++j ) {
			for( int i=crashRect0.min.x; i<=crashRect0.max.x; ++i ) {
				if ( random.Bit() ) {
					TiXmlElement* fire = new TiXmlElement( "Pyro" );
					fire->SetAttribute( "x", i );
					fire->SetAttribute( "y", j );
					fire->SetAttribute( "fire", 1 );
					fire->SetAttribute( "duration", 0 );
					pgElement->LinkEndChild( fire );
				}
			}
		}
	}
}


void TacticalIntroScene::CreateMap(	FILE* fp, 
									int seed,
									const SceneInfo& info,
									const gamedb::Reader* database )
{
	// Max world size is 64x64 units, in 16x16 unit blocks. That gives 4x4 blocks max.
	BitArray< 4, 4, 1 > blocks;

	TiXmlElement mapElement( "Map" );
	const gamedb::Item* dataItem = database->Root()->Child( "data" );

	Vector2I size = info.Size();
	mapElement.SetAttribute( "sizeX", size.x*16 );
	mapElement.SetAttribute( "sizeY", size.y*16 );
	
	Random random( seed );

	for( int i=0; i<5; ++i ) 
		random.Rand();

	Vector2I cornerPosBlock[2];
	if ( random.Bit() ) {
		cornerPosBlock[0].Set( 0, 0 );
		cornerPosBlock[1].Set( size.x-1, size.y-1 );
	}
	else {
		cornerPosBlock[0].Set( size.x-1, 0 );
		cornerPosBlock[1].Set( 0, size.y-1 );
	}
	if ( random.Bit() ) {
		Swap( cornerPosBlock+0, cornerPosBlock+1 );
	}


	if ( info.TileAlgorithm() ) {
		// Lander
		{
			Vector2I pos = cornerPosBlock[ 0 ];
			blocks.Set( pos.x, pos.y );
			int tileRotation = random.Rand(4);

			AppendMapSnippet( pos.x*16, pos.y*16, tileRotation, info.Base(), 16, false, "LAND", database, dataItem, &mapElement, random.Rand() );	
		}

		// UFO
		{
			Rectangle2I pos;
			pos.min = pos.max = cornerPosBlock[ 1 ];
			int ufoSize = info.UFOSize();

			if ( ufoSize == 2 ) {
				if ( pos.min.x == 0 ) 
					pos.max.x++;
				else
					pos.min.x--;
				if ( pos.min.y == 0 )
					pos.max.y++;
				else
					pos.min.y--;
			}

			for( int j=pos.min.y; j<=pos.max.y; ++j )
				for( int i=pos.min.x; i<=pos.max.x; ++i )
					blocks.Set( i, j );

			int tileRotation = random.Rand(4);
			AppendMapSnippet( pos.min.x*16, pos.min.y*16, tileRotation, info.Base(), 16*ufoSize, info.crash, info.UFO(), database, dataItem, &mapElement, random.Rand() );
		}

		for( int j=0; j<size.y; ++j ) {
			for( int i=0; i<size.x; ++i ) {
				if ( !blocks.IsSet( i, j ) ) {
					Vector2I pos = { i, j };
					int tileRotation = random.Rand(4);
					AppendMapSnippet( pos.x*16, pos.y*16, tileRotation, info.Base(), 16, false, "TILE", database, dataItem, &mapElement, random.Rand() );	
				}
			}
		}
	}
	else if ( info.scenario == CITY ) {
		/*
			_+_+
			 | |
			_+_+
			 | |
		*/
		static const Vector2I open[4]  = { { 0,1 }, { 2,1 }, { 0,3 }, { 2,3 } };
		static const Vector2I hroad[4] = { { 0,0 }, { 2,0 }, { 0,2 }, { 2,2 } };
		static const Vector2I vroad[4] = { { 1,1 }, { 3,1 }, { 1,3 }, { 3,3 } };
		static const Vector2I inter[4] = { { 1,0 }, { 3,0 }, { 1,2 }, { 3,2 } };

		// Lander
		Vector2I lander = { 0, 0 };
		{
			int tileRotation = random.Rand(4);
			lander = open[random.Rand(4)];
			AppendMapSnippet( lander.x*16, lander.y*16, tileRotation, "CITY", 16, false, "LAND", database, dataItem, &mapElement, random.Rand() );	
		}

		// Open
		for( int i=0; i<4; ++i ) {
			if ( lander != open[i] ) {
				int tileRotation = random.Rand(4);
				AppendMapSnippet( open[i].x*16, open[i].y*16, tileRotation, "CITY", 16, false, "OPEN", database, dataItem, &mapElement, random.Rand() );	
			}
		}
		// Roads
		for( int i=0; i<4; ++i ) {
			AppendMapSnippet( hroad[i].x*16, hroad[i].y*16, 1, "CITY", 16, false, "ROAD", database, dataItem, &mapElement, random.Rand() );	
		}
		for( int i=0; i<4; ++i ) {
			AppendMapSnippet( vroad[i].x*16, vroad[i].y*16, 0, "CITY", 16, false, "ROAD", database, dataItem, &mapElement, random.Rand() );	
		}
		for( int i=0; i<4; ++i ) {
			AppendMapSnippet( inter[i].x*16, inter[i].y*16, 1, "CITY", 16, false, "INTR", database, dataItem, &mapElement, random.Rand() );	
		}
		
	}
	else if ( info.scenario == BATTLESHIP ) {
		AppendMapSnippet( 0, 0, 0, "BATT", 48, false, "TILE", database, dataItem, &mapElement, random.Rand() );	
	}
	else if ( info.scenario == ALIEN_BASE ) {
		AppendMapSnippet( 0, 0, 0, "ALIN", 48, false, "TILE", database, dataItem, &mapElement, random.Rand() );	
	}
	else if ( info.scenario == TERRAN_BASE ) {
		AppendMapSnippet( 0, 0, 0, "BASE", 48, false, "TILE", database, dataItem, &mapElement, random.Rand() );	
	}
	else {
		GLASSERT( 0 );
	}


	mapElement.Print( fp, 2 );
}



void TacticalIntroScene::GenerateAlienTeamUpper(	int scenario,
													bool crash,
													float rank,
													Unit* units,
													const ItemDefArr& itemDefArr,
													int seed )
{
	Random random( seed );
	random.Rand();
	random.Rand();

	int count[Unit::NUM_ALIEN_TYPES] = { 0 };
	switch ( scenario ) {
	case FARM_SCOUT:
	case TNDR_SCOUT:
	case FRST_SCOUT:
	case DSRT_SCOUT:
		count[0] = 3+random.Rand(2);	// green 3-4
		if ( rank >= 2.0f )
			count[1] = random.Rand(2);	// prime 0-1
		break;

	case FARM_DESTROYER:
	case TNDR_DESTROYER:
	case FRST_DESTROYER:
	case DSRT_DESTROYER:
		count[1] = 1 + random.Rand(2);	// prime  1-2
		count[2] = 4 + random.Rand(2);	// hornet 4-5
		count[3] = random.Rand( 2 );	// jackal 0-1
		count[4] = 4 + random.Rand(2);	// viper  4-5	// total: 9-13
		break;

	case CITY:
		count[0] = 2;					// green  2-2
		count[2] = 4 + random.Rand(3);	// hornet 4-6
		count[3] = 4 + random.Rand(3);	// jackal 4-6
		count[4] = random.Rand( 3 );	// viper  0-2	// total: 10-16
		break;

	case BATTLESHIP:
	case ALIEN_BASE:
		count[0] = 2;	// green
		count[1] = 5;	// prime
		count[2] = 5;	// hornet
		count[4] = 4;	// viper
		break;

	case TERRAN_BASE:
		count[0] = 3;	// green
		count[2] = 5;	// hornet
		count[3] = 3;	// jackal
		count[4] = 5;	// viper
		break;

	default:
		GLASSERT( 0 );
	}

	GenerateAlienTeam( units, count, rank, itemDefArr, random.Rand() );
	if ( crash ) {
		for( int i=0; i<MAX_ALIENS; ++i ) {
			if ( units[i].IsAlive() ) {
				DamageDesc dd;
				float max = (float)units[i].GetStats().TotalHP() * 0.8f * random.Uniform();
				dd.energy = max*0.33f;
				dd.kinetic = max*0.33f;
				dd.incend = max*0.33f;
				units[i].DoDamage( dd, 0, false );
				GLASSERT( units[i].IsAlive() );
			}
		}
	}
}


int TacticalIntroScene::RandomRank( grinliz::Random* random, float rank )
{
	int r = Clamp( (int)(rank+random->Uniform()*0.99f ), 0, NUM_RANKS-1 );
	return r;
}


void TacticalIntroScene::GenerateAlienTeam( Unit* unit,				// target units to write
											const int alienCount[],	// aliens per type
											float averageRank,
											const ItemDefArr& itemDefArr,
											int seed )
{
	const char* weapon[Unit::NUM_ALIEN_TYPES][NUM_RANKS] = {
		{	"RAY-1",	"RAY-1",	"RAY-2",	"RAY-2",	"RAY-3" },		// green
		{	"RAY-1",	"RAY-2",	"RAY-2",	"RAY-3",	"PLS-3"	},		// prime
		{	"PLS-1",	"PLS-1",	"PLS-2",	"PLS-2",	"PLS-3" },		// hornet
		{	"STRM-1",	"STRM-1",	"STRM-2",	"STRM-2",	"STRM-3" },		// jackal
		{	"PLS-1",	"PLS-1",	"PLS-2",	"PLS-2",	"PLS-3" }		// viper
	};

	int nAliens = 0;

	for( int i=0; i<Unit::NUM_ALIEN_TYPES; ++i ) {
		nAliens += alienCount[i];
	}
	GLASSERT( nAliens <= MAX_ALIENS );
	// local random - the same inputs always create same outputs.
	grinliz::Random aRand( (nAliens*((int)averageRank)) ^ seed );
	aRand.Rand();

	int index=0;
	for( int i=0; i<Unit::NUM_ALIEN_TYPES; ++i ) {
		for( int k=0; k<alienCount[i]; ++k ) {
			
			// Create the unit.
			int rank = RandomRank( &aRand, averageRank );
 			unit[index].Create( ALIEN_TEAM, i, rank, aRand.Rand() );

			// About 1/3 should be guards that aren't green or prime.
			if ( i >= 2 && (index % 3) == 0 ) {
				unit[index].SetAI( AI::AI_GUARD );
			}

			rank = RandomRank( &aRand, averageRank );

			// Add the weapon.
			Item item( itemDefArr, weapon[i][rank] );
			unit[index].GetInventory()->AddItem( item );

			// Add ammo.
			const WeaponItemDef* weaponDef = item.GetItemDef()->IsWeapon();
			GLASSERT( weaponDef );

			for( int n=0; n<2; ++n ) {
				Item ammo( weaponDef->GetClipItemDef( kSnapFireMode ) );
				unit[index].GetInventory()->AddItem( ammo );
			}
			if ( weaponDef->HasWeapon( kAltFireMode ) ) {
				Item ammo( weaponDef->GetClipItemDef( kAltFireMode ) );
				unit[index].GetInventory()->AddItem( ammo );
			}
			++index;
		}
	}
}


void TacticalIntroScene::GenerateTerranTeam(	Unit* unit,				// target units to write
												int count,	
												float baseRank,
												const ItemDefArr& itemDefArr,
												int seed )
{
	static const int POSITION = 4;
	static const char* weapon[POSITION][NUM_RANKS] = {
		{	"ASLT-1",	"ASLT-1",	"ASLT-2",	"ASLT-2",	"ASLT-3" },		// assault
		{	"ASLT-1",	"ASLT-1",	"ASLT-2",	"PLS-2",	"PLS-3" },		// assault
		{	"LR-1",		"LR-1",		"LR-2",		"LR-2",		"LR-3" },		// sniper
		{	"MCAN-1",	"MCAN-1",	"MCAN-2",	"STRM-2",	"MCAN-3" },		// heavy
	};
	static const char* armorType[NUM_RANKS] = {
		"ARM-1", "ARM-1", "ARM-2", "ARM-2", "ARM-3"
	};
	static const char* extraItems[NUM_RANKS] = {
		"", "", "SG:I", "SG:K", "SG:E"
	};

	// local random - the same inputs always create same outputs.
	grinliz::Random aRand( count ^ seed );
	aRand.Rand();

	for( int k=0; k<count; ++k ) 
	{
		int position = k % POSITION;

		// Create the unit.
		int rank = RandomRank( &aRand, baseRank );
 		unit[k].Create( TERRAN_TEAM, 0, rank, aRand.Rand() );

		rank = RandomRank( &aRand, baseRank );

		// Add the weapon.
		Item item( itemDefArr, weapon[position][rank] );
		unit[k].GetInventory()->AddItem( item );

		// Add ammo.
		const WeaponItemDef* weaponDef = item.GetItemDef()->IsWeapon();
		GLASSERT( weaponDef );

		for( int n=0; n<2; ++n ) {
			Item ammo( weaponDef->GetClipItemDef( kSnapFireMode ) );
			unit[k].GetInventory()->AddItem( ammo );
		}
		if ( weaponDef->HasWeapon( kAltFireMode ) ) {
			Item ammo( weaponDef->GetClipItemDef( kAltFireMode ) );
			unit[k].GetInventory()->AddItem( ammo );
		}

		// Add extras
		{
			rank = RandomRank( &aRand, baseRank );
			Item armor( itemDefArr, armorType[rank] );
			unit[k].GetInventory()->AddItem( armor );
		}

		rank = RandomRank( &aRand, baseRank );
		for( int i=0; i<=rank; ++i ) {
			Item extra( itemDefArr, extraItems[i] );
			unit[k].GetInventory()->AddItem( extra );
		}
	}
}


void TacticalIntroScene::GenerateCivTeam( Unit* unit,				// target units to write
										  int count,
										  const ItemDefArr& itemDefArr,
										  int seed )
{
	grinliz::Random aRand( seed );
	aRand.Rand();
	aRand.Rand();

	for( int i=0; i<count; ++i ) {
		unit[i].Create( CIV_TEAM, 0, 0, aRand.Rand() );
	}
}


Vector2I TacticalIntroScene::SceneInfo::Size() const
{
	Vector2I v = { 0, 0 };
	if ( scenario >= FARM_SCOUT && scenario <= DSRT_SCOUT ) {
		v.Set( 3, 2 );
	}
	else if ( scenario >= FARM_DESTROYER && scenario <= DSRT_DESTROYER ) {
		v.Set( 3, 3 ); // or 3,2 ?
	}
	else if ( scenario == CITY ) {
		v.Set( 4, 4 );
	}
	else if (    scenario == BATTLESHIP
		      || scenario == TERRAN_BASE 
			  || scenario == ALIEN_BASE ) 
	{
		v.Set( 3, 3 );
	}
	else {
		GLASSERT( 0 );
	}
	return v;
}


const char* TacticalIntroScene::SceneInfo::UFO() const
{
	static const char* ufo[2] = { "UFOA", "UFOB" };
	return ( scenario >= FARM_SCOUT && scenario <= DSRT_SCOUT ) ? ufo[0] : ufo[1];
}


const char* TacticalIntroScene::SceneInfo::Base() const
{
	static const char* base[5] = { "FARM", "TNDR", "FRST", "DSRT", "" };
	if ( scenario >= FARM_SCOUT && scenario <= DSRT_DESTROYER ) {
		return base[(scenario-FARM_SCOUT)%4];
	}
	return base[4];
}

