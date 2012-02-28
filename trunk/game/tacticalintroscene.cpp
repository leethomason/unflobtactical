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
#include "../engine/uirendering.h"
#include "../engine/engine.h"
#include "game.h"
#include "cgame.h"
#include "helpscene.h"
#include "gamesettings.h"
#include "ai.h"
#include "tacmap.h"
#include "saveloadscene.h"
#include "geoscene.h"
#include "tacticalintroscene.h"

#include "../version.h"

using namespace grinliz;
using namespace gamui;
using namespace tinyxml2;


TacticalIntroScene::TacticalIntroScene( Game* _game ) : Scene( _game )
{
	//Engine* engine = GetEngine();
	//const Screenport& port = engine->GetScreenport();
	random.SetSeedFromTime();

	// -- Background -- //
	gamui::RenderAtom nullAtom;

	const gamui::ButtonLook& green = game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );

	continueButton.Init( &gamui2D, green );
	continueButton.SetSize( GAME_BUTTON_SIZE_B*2.5F, GAME_BUTTON_SIZE_B );
	continueButton.SetText( "Continue" );

	loadButton.Init( &gamui2D, green );
	loadButton.SetSize( GAME_BUTTON_SIZE_B, GAME_BUTTON_SIZE_B );
	loadButton.SetDeco( Game::CalcDecoAtom( DECO_SAVE_LOAD, true ),
						Game::CalcDecoAtom( DECO_SAVE_LOAD, false ) );	 

	newTactical.Init( &gamui2D, green );
	newTactical.SetSize( GAME_BUTTON_SIZE_B*2.0F, GAME_BUTTON_SIZE_B );
	newTactical.SetText( "New Tactical" );
	
	newGeo.Init( &gamui2D, green );
	newGeo.SetSize( GAME_BUTTON_SIZE_B*2.0F, GAME_BUTTON_SIZE_B );
	newGeo.SetText( "New Geo" );
	
	// Same place as new geo
	newGame.Init( &gamui2D, blue );
	newGame.SetSize( GAME_BUTTON_SIZE_B*2.2F, GAME_BUTTON_SIZE_B );
	newGame.SetText( "New Game" );

	newGameWarning.Init( &gamui2D );
	
	if ( game->HasSaveFile( SAVEPATH_GEO, 0 ) ) {
		newGeo.SetVisible( false );
		newTactical.SetVisible( false );
		newGame.SetVisible( true );
		newGameWarning.SetText( "'New' deletes current Geo game." );
	}
	else if ( game->HasSaveFile( SAVEPATH_TACTICAL, 0 ) ) {
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
	helpButton.SetSize( GAME_BUTTON_SIZE_B, GAME_BUTTON_SIZE_B );
	helpButton.SetDeco( Game::CalcDecoAtom( DECO_HELP, true ),
						Game::CalcDecoAtom( DECO_HELP, false ) );	

	audioButton.Init( &gamui2D, green );
	audioButton.SetSize( GAME_BUTTON_SIZE_B, GAME_BUTTON_SIZE_B );

	settingButton.Init( &gamui2D, green );
	settingButton.SetSize( GAME_BUTTON_SIZE_B, GAME_BUTTON_SIZE_B );
	settingButton.SetDeco(	Game::CalcDecoAtom( DECO_SETTINGS, true ),
							Game::CalcDecoAtom( DECO_SETTINGS, false ) );	

	backgroundUI.Init( game, &gamui2D, true );


	// Is there a current game?
	continueButton.SetEnabled( game->HasSaveFile( SAVEPATH_GEO, 0 ) || game->HasSaveFile( SAVEPATH_TACTICAL, 0 ) );
}


TacticalIntroScene::~TacticalIntroScene()
{
}


void TacticalIntroScene::Resize()
{
	const Screenport& port = GetEngine()->GetScreenport();
	GLOUTPUT(( "TacticalIntroScene Resize screen=%.1f,%.1f\n", port.UIWidth(), port.UIHeight() ));

	LayoutCalculator layout( port.UIWidth(), port.UIHeight() );
	layout.SetGutter( GAME_GUTTER );
	layout.SetSpacing( GAME_GUTTER/2 );

	// Double wide buttons
	layout.SetSize( GAME_BUTTON_SIZE_B*2.0f, GAME_BUTTON_SIZE_B );
	layout.PosAbs( &continueButton, 0, -1 );
	loadButton.SetPos( continueButton.X() + continueButton.Width(), continueButton.Y() );
	
	layout.PosAbs( &newGeo,			-2, -1 );
	layout.PosAbs( &newTactical,	-1, -1 );
	layout.PosAbs( &newGame,		-2, -1 );
	newGameWarning.SetPos( newGeo.X()-15, newGeo.Y() + newGeo.Height()+2 );

	// Square buttons
	layout.SetSize( GAME_BUTTON_SIZE_B, GAME_BUTTON_SIZE_B );
	layout.PosAbs( &helpButton,		-1, 0 );
	layout.PosAbs( &audioButton,	-1, 1 );
	layout.PosAbs( &settingButton,  -1, 2 );

	backgroundUI.backgroundText.SetPos( GAME_GUTTER, GAME_GUTTER );
	float maxX = settingButton.X() - GAME_GUTTER*2.0f;
	float maxY = continueButton.Y() - GAME_GUTTER*2.0f;
	if ( maxX < maxY*2.0f ) {
		backgroundUI.backgroundText.SetSize( maxX, maxX*0.5f );
	}
	else {
		backgroundUI.backgroundText.SetSize( maxY*2.0f, maxY );
	}
	backgroundUI.background.SetSize( port.UIWidth(), port.UIHeight() );
}


void TacticalIntroScene::Activate()
{
	SettingsManager* settings = SettingsManager::Instance();
	if ( settings->GetAudioOn() ) {
		audioButton.SetDown();
		audioButton.SetDeco( Game::CalcDecoAtom( DECO_AUDIO, true ),
							 Game::CalcDecoAtom( DECO_AUDIO, false ) );	
	}
	else {
		audioButton.SetUp();
		audioButton.SetDeco( Game::CalcDecoAtom( DECO_MUTE, true ),
							 Game::CalcDecoAtom( DECO_MUTE, false ) );	
	}
	Resize();
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
	else if ( item == &loadButton ) {
		game->PushScene( Game::SAVE_LOAD_SCENE, new SaveLoadSceneData( false ) );
	}
	else if ( item == &newTactical ) {
		game->PushScene( Game::NEW_TAC_OPTIONS, 0 ); 
		game->DeleteSaveFile( SAVEPATH_TACTICAL, 0 );
		game->DeleteSaveFile( SAVEPATH_GEO, 0 );
	}
	else if ( item == &continueButton ) {
		if ( game->HasSaveFile( SAVEPATH_GEO, 0 ) )
			onToNext = Game::GEO_SCENE;
		else if ( game->HasSaveFile( SAVEPATH_TACTICAL, 0 ) )
			onToNext = Game::BATTLE_SCENE;
	}
	else if ( item == &helpButton ) {
		game->PushScene( Game::HELP_SCENE, new HelpSceneData("introHelp", false ));
	}
	else if ( item == &audioButton ) {
		SettingsManager* settings = SettingsManager::Instance();
		if ( audioButton.Down() ) {
			settings->SetAudioOn( true );
			audioButton.SetDeco( Game::CalcDecoAtom( DECO_AUDIO, true ),
								 Game::CalcDecoAtom( DECO_AUDIO, false ) );	
		}
		else {
			settings->SetAudioOn( false );
			audioButton.SetDeco( Game::CalcDecoAtom( DECO_MUTE, true ),
								 Game::CalcDecoAtom( DECO_MUTE, false ) );	
		}
	}
	else if ( item == &settingButton ) {
		game->PushScene( Game::SETTING_SCENE, 0 );
	}
	else if ( item == &newGeo ) {
		game->PushScene( Game::NEW_GEO_OPTIONS, 0 ); 
		game->DeleteSaveFile( SAVEPATH_GEO, 0 );
		game->DeleteSaveFile( SAVEPATH_TACTICAL, 0 );
		//onToNext = Game::GEO_SCENE;
	}

	if ( onToNext >= 0 ) {
		game->PopScene();
		game->PushScene( onToNext, 0 );
	}
}


void TacticalIntroScene::SceneResult( int sceneID, int r )
{
	if ( sceneID == Game::NEW_TAC_OPTIONS ) {
		NewSceneOptionsReturn result;
		result = *((NewSceneOptionsReturn*)&r);

		FILE* fp = game->GameSavePath( SAVEPATH_TACTICAL, SAVEPATH_WRITE, 0 );
		GLASSERT( fp );
		if ( fp ) {
			BattleSceneData data;
			data.seed = random.Rand();
			data.scenario = result.scenario;
			data.crash = result.crash != 0;

			Unit units[MAX_TERRANS];

			GenerateTerranTeam( units, result.nTerrans, (float)result.terranRank, 
							    game->GetItemDefArr(), random.Rand() );
			data.soldierUnits = units;
			data.nScientists = 8;

			data.dayTime = result.dayTime != 0;
			data.alienRank = (float)result.alienRank;
			data.storage = 0;

			WriteXML( fp, &data, game->GetItemDefArr(), game->GetDatabase() );
			fclose( fp );
			game->PopScene();
			game->PushScene( Game::BATTLE_SCENE, 0 );
		}
	}
	else if ( sceneID == Game::NEW_GEO_OPTIONS ) {
		GLOUTPUT(( "Difficulty=%d\n", r ));
		game->PopScene();
		game->PushScene( Game::GEO_SCENE, new GeoSceneData( r ) );
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

	XMLPrinter printer( fp );

	printer.OpenElement( "Game" );
	printer.PushAttribute( "version", VERSION );
	printer.PushAttribute( "sceneID", Game::BATTLE_SCENE );

	printer.OpenElement( "BattleScene" );
	printer.PushAttribute( "dayTime", data->dayTime ? 1 : 0 );
	printer.PushAttribute( "scenario", data->scenario );

	Random random;
	random.SetSeedFromTime();

	int nCivs = ( data->scenario == TERRAN_BASE ) ? data->nScientists : CivsInScenario( data->scenario );
	SceneInfo info( data->scenario, data->crash, nCivs );

	CreateMap( fp, random.Rand(), info, database );
	fprintf( fp, "\n" );

	BattleData battleData( itemDefArr );
	battleData.SetDayTime( data->dayTime );
	battleData.SetScenario( data->scenario );

	// Terran soldier units.
	//memcpy( &battleData.units[TERRAN_UNITS_START], data->soldierUnits, sizeof(Unit)*MAX_TERRANS );
	for( int i=0; i<MAX_TERRANS; ++i ) {
		battleData.CopyUnit( TERRAN_UNITS_START+i, data->soldierUnits[i] );
	}

	// Alien units
	GenerateAlienTeamUpper( data->scenario, 
							data->crash, 
							data->alienRank, 
							battleData.AlienPtr(), 
							itemDefArr, 
							random.Rand() );

	// Civ team
	GenerateCivTeam( battleData.CivPtr(), 
					 info.nCivs, 
					 itemDefArr, 
					 random.Rand() );

	battleData.Save( &printer );
	printer.CloseElement();
	printer.CloseElement();
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
											XMLElement* mapElement,
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

	// Snippet is the XML to append. We need to be
	// a little careful sticking them together due
	// to the memory model.
	XMLDocument snippet;
	snippet.Parse( xmlText );
	GLASSERT( !snippet.Error() );

	XMLDocument* mapDocument = mapElement->GetDocument();

	XMLElement* itemsElement = mapElement->FirstChildElement( "Items" );
	if ( !itemsElement ) {
		itemsElement = mapDocument->NewElement( "Items" );
		mapElement->InsertEndChild( itemsElement );
	}

	// Append the <Items>, account for (x,y) changes.
	// Append the <Images>, account for (x,y) changes.
	// Append one sub tree to the other. Adjust x, y as we go.
	Matrix2I m;
	Map::MapImageToWorld( dx, dy, size, size, tileRotation, &m );

	Rectangle2I crashRect, crashRect0;
	if ( crash ) {
		int half = size/2;
		crashRect.min.Set( random.Rand( half ), random.Rand( half ) );
		crashRect.max.x = crashRect.min.x + half;
		crashRect.max.y = crashRect.min.y + half;
	}
	Vector2I cr0 = m * crashRect.min;
	Vector2I cr1 = m * crashRect.max;
	crashRect0.FromPair( cr0.x, cr0.y, cr1.x, cr1.y );

	for( XMLElement* eleIt = snippet.FirstChildElement( "Map" )->FirstChildElement( "Items" )->FirstChildElement( "Item" );
		 eleIt;
		 eleIt = eleIt->NextSiblingElement() )
	{
		GLASSERT( eleIt->FirstChild() == 0 );	// this is a shallow clone.
		XMLElement* ele = snippet.ShallowClone( mapDocument )->ToElement();

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

		itemsElement->InsertEndChild( ele );
	}

	// And add the image data
	XMLElement* imagesElement = mapElement->FirstChildElement( "Images" );
	if ( !imagesElement ) {
		imagesElement = mapDocument->NewElement( "Images" );
		mapElement->InsertEndChild( imagesElement );
	}

	char buffer[64];
	SNPrintf( buffer, 64, "%4s_%02d_%4s_%02d", set, size, type, seed );

	XMLElement* image = mapDocument->NewElement( "Image" );
	image->SetAttribute( "name", buffer );
	image->SetAttribute( "x", dx );
	image->SetAttribute( "y", dy );
	image->SetAttribute( "size", size );
	image->SetAttribute( "tileRotation", tileRotation );
	imagesElement->LinkEndChild( image );

	if ( crash ) {
		XMLElement* pgElement = mapElement->FirstChildElement( "PyroGroup" );
		if ( !pgElement ) {
			pgElement = mapDocument->NewElement( "PyroGroup" );
			mapElement->LinkEndChild( pgElement );
		}
		for( int j=crashRect0.min.y; j<=crashRect0.max.y; ++j ) {
			for( int i=crashRect0.min.x; i<=crashRect0.max.x; ++i ) {
				if ( random.Bit() ) {					
					XMLElement* fire = mapDocument->NewElement( "Pyro" );
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

	XMLDocument doc;
	XMLElement* mapElement = doc.NewElement( "Map" );
	doc.InsertEndChild( mapElement );

	const gamedb::Item* dataItem = database->Root()->Child( "data" );

	Vector2I size = info.Size();
	mapElement->SetAttribute( "sizeX", size.x*16 );
	mapElement->SetAttribute( "sizeY", size.y*16 );
	
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

			AppendMapSnippet( pos.x*16, pos.y*16, tileRotation, info.Base(), 16, false, "LAND", database, dataItem, mapElement, random.Rand() );	
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
			AppendMapSnippet( pos.min.x*16, pos.min.y*16, tileRotation, info.Base(), 16*ufoSize, info.crash, info.UFO(), database, dataItem, mapElement, random.Rand() );
		}

		for( int j=0; j<size.y; ++j ) {
			for( int i=0; i<size.x; ++i ) {
				if ( !blocks.IsSet( i, j ) ) {
					Vector2I pos = { i, j };
					int tileRotation = random.Rand(4);
					AppendMapSnippet( pos.x*16, pos.y*16, tileRotation, info.Base(), 16, false, "TILE", database, dataItem, mapElement, random.Rand() );	
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
			AppendMapSnippet( lander.x*16, lander.y*16, tileRotation, "CITY", 16, false, "LAND", database, dataItem, mapElement, random.Rand() );	
		}

		// Open
		for( int i=0; i<4; ++i ) {
			if ( lander != open[i] ) {
				int tileRotation = random.Rand(4);
				AppendMapSnippet( open[i].x*16, open[i].y*16, tileRotation, "CITY", 16, false, "OPEN", database, dataItem, mapElement, random.Rand() );	
			}
		}
		// Roads
		for( int i=0; i<4; ++i ) {
			AppendMapSnippet( hroad[i].x*16, hroad[i].y*16, 1, "CITY", 16, false, "ROAD", database, dataItem, mapElement, random.Rand() );	
		}
		for( int i=0; i<4; ++i ) {
			AppendMapSnippet( vroad[i].x*16, vroad[i].y*16, 0, "CITY", 16, false, "ROAD", database, dataItem, mapElement, random.Rand() );	
		}
		for( int i=0; i<4; ++i ) {
			AppendMapSnippet( inter[i].x*16, inter[i].y*16, 1, "CITY", 16, false, "INTR", database, dataItem, mapElement, random.Rand() );	
		}
		
	}
	else if ( info.scenario == BATTLESHIP ) {
		AppendMapSnippet( 0, 0, 0, "BATT", 48, false, "TILE", database, dataItem, mapElement, random.Rand() );	
	}
	else if ( info.scenario == ALIEN_BASE ) {
		AppendMapSnippet( 0, 0, 0, "ALIN", 48, false, "TILE", database, dataItem, mapElement, random.Rand() );	
	}
	else if ( info.scenario == TERRAN_BASE ) {
		AppendMapSnippet( 0, 0, 0, "BASE", 48, false, "TILE", database, dataItem, mapElement, random.Rand() );	
	}
	else {
		GLASSERT( 0 );
	}

	XMLPrinter printer( fp );
	mapElement->Accept( &printer );
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
	int baseAlien = GameSettingsManager::Instance()->GetTestAlien();

	switch ( scenario ) {
	case FARM_SCOUT:
	case TNDR_SCOUT:
	case FRST_SCOUT:
	case DSRT_SCOUT:
		if ( baseAlien ) {
			count[baseAlien] = 3+random.Rand(2);				// green 3-4 (typically green - use others if in settings)
		}
		else if ( rank < 2 || random.Rand(3) || !CivsInScenario(scenario) ) {
			count[Unit::ALIEN_GREEN] = 3 + random.Rand(2);		// OR green 3-4
		}
		else {
			count[Unit::ALIEN_SPITTER] = 3 + random.Rand(2);	// OR spitter 3-4
		}

		if ( rank >= 2.0f ) {
			if ( random.Bit() )
				count[Unit::ALIEN_SQUID] = random.Rand(2);	// squid 0-1 OR
			else
				count[Unit::ALIEN_PRIME] = random.Rand(2);	// prime 0-1
		}
		break;

	case FARM_DESTROYER:
	case TNDR_DESTROYER:
	case FRST_DESTROYER:
	case DSRT_DESTROYER:
		count[Unit::ALIEN_PRIME]  = 1;					// prime  1-1
		count[Unit::ALIEN_HORNET] = 4 + random.Rand(2);	// hornet 4-5
		count[Unit::ALIEN_SQUID]  = 1 + random.Rand(2);	// squid  1-2
		count[Unit::ALIEN_VIPER]  = 4 + random.Rand(2);	// viper  4-5	// total: 10-13
		break;

	case CITY:
		count[Unit::ALIEN_GREEN]  = 2;					// green  2-2
		count[Unit::ALIEN_SQUID]  = random.Rand( 3 );	// squid  0-2
		count[Unit::ALIEN_HORNET] = 4 + random.Rand(3);	// hornet 4-4
		if ( random.Bit() )
			count[Unit::ALIEN_JACKAL] = 4 + random.Rand(3);	    // jackal 4-6 OR
		else
			count[Unit::ALIEN_SPITTER] = 4 + random.Rand(3);	// spitter 4-6
		count[Unit::ALIEN_VIPER]  = random.Rand( 3 );   // viper  0-2	// total: 10-16
		break;

	case BATTLESHIP:
		count[Unit::ALIEN_GREEN]  = 2;
		count[Unit::ALIEN_SQUID]  = 2;
		count[Unit::ALIEN_PRIME]  = 3;	// prime
		count[Unit::ALIEN_HORNET] = 5;	// hornet
		count[Unit::ALIEN_VIPER]  = 4;	// viper
		break;

	case ALIEN_BASE:
		count[Unit::ALIEN_SQUID]  = 2;
		count[Unit::ALIEN_PRIME]  = 5;	// prime
		count[Unit::ALIEN_HORNET] = 5;	// hornet
		count[Unit::ALIEN_VIPER]  = 4;	// viper
		break;

	case TERRAN_BASE:
		count[Unit::ALIEN_GREEN]  = 3;	// green
		count[Unit::ALIEN_HORNET] = 5;	// hornet
		count[Unit::ALIEN_JACKAL] = 3;	// jackal
		count[Unit::ALIEN_VIPER]  = 5;	// viper
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


int TacticalIntroScene::RandomRank( grinliz::Random* random, float rank, int nRanks )
{
	int r = Clamp( (int)(rank+random->Uniform()*0.99f ), 0, nRanks-1 );
	return r;
}


void TacticalIntroScene::GenerateAlienTeam( Unit* unit,				// target units to write
											const int alienCount[],	// aliens per type
											float averageRank,
											const ItemDefArr& itemDefArr,
											int seed )
{
	static const int WEAPON_RANKS = 5;
	const char* weapon[Unit::NUM_ALIEN_TYPES][WEAPON_RANKS] = {
		{	"RAY-1",	"RAY-1",	"RAY-2",	"RAY-2",	"RAY-3" },		// green
		{	"RAY-1",	"RAY-2",	"RAY-2",	"RAY-3",	"PLS-3"	},		// prime
		{	"PLS-1",	"PLS-1",	"PLS-2",	"PLS-2",	"PLS-3" },		// hornet
		{	"STRM-1",	"STRM-1",	"STRM-2",	"STRM-2",	"STRM-3" },		// jackal
		{	"PLS-1",	"PLS-1",	"PLS-2",	"PLS-2",	"PLS-3" },		// viper
		{	"RAY-1",	"RAY-2",	"RAY-2",	"RAY-3",	"RAY-3"	},		// squid
		{   "Spit-1",	"Spit-2",	"Spit-2",	"Spit-2",	"Spit-3" },		// spitter - intrinsic weapon
		{	"", "", "", "", "" },											// crawler - can't use weapons
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
			int rank = RandomRank( &aRand, averageRank, NUM_ALIEN_RANKS );
 			unit[index].Create( ALIEN_TEAM, i, rank, aRand.Rand() );

			// About 1/3 should be guards that aren't green or prime.
			if (    i == Unit::ALIEN_HORNET
				 || i == Unit::ALIEN_VIPER ) 
			{
				if ( (index % 3) == 0 ) {
					unit[index].SetAI( AI::AI_GUARD );
				}
			}

			rank = RandomRank( &aRand, averageRank, WEAPON_RANKS );

			// Add the weapon.
			if ( *weapon[i][rank] ) {
				Item item( itemDefArr, weapon[i][rank] );
				unit[index].GetInventory()->AddItem( item, 0 );

				// Add ammo.
				const WeaponItemDef* weaponDef = item.GetItemDef()->IsWeapon();
				GLASSERT( weaponDef );

				for( int n=0; weaponDef->HasWeapon(n); ++n ) {
					Item ammo( weaponDef->GetClipItemDef( n ) );
					unit[index].GetInventory()->AddItem( ammo, 0 );
				}
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
	static const char* weapon[POSITION][NUM_TERRAN_RANKS] = {
		{	"ASLT-1",	"ASLT-1",	"ASLT-2",	"ASLT-2",	"ASLT-3" },		// assault
		{	"ASLT-1",	"ASLT-1",	"ASLT-2",	"PLS-2",	"PLS-3" },		// assault
		{	"LR-1",		"LR-1",		"LR-2",		"LR-2",		"LR-3" },		// sniper
		{	"MCAN-1",	"MCAN-1",	"MCAN-2",	"STRM-2",	"MCAN-3" },		// heavy
	};
	static const char* armorType[NUM_TERRAN_RANKS] = {
		"ARM-1", "ARM-1", "ARM-2", "ARM-2", "ARM-3"
	};
	static const char* extraItems[NUM_TERRAN_RANKS] = {
		"", "", "SG:I", "SG:K", "SG:E"
	};

	// local random - the same inputs always create same outputs.
	grinliz::Random aRand( count ^ seed );
	aRand.Rand();

	for( int k=0; k<count; ++k ) 
	{
		int position = k % POSITION;

		// Create the unit.
		int rank = RandomRank( &aRand, baseRank, NUM_TERRAN_RANKS );
		memset( &unit[k], 0, sizeof(Unit) );
 		unit[k].Create( TERRAN_TEAM, 0, rank, aRand.Rand() );

		rank = RandomRank( &aRand, baseRank, NUM_TERRAN_RANKS );

		// Add the weapon.
		Item item( itemDefArr, weapon[position][rank] );
		unit[k].GetInventory()->AddItem( item, 0 );

		// Add ammo.
		const WeaponItemDef* weaponDef = item.GetItemDef()->IsWeapon();
		GLASSERT( weaponDef );

		for( int n=0; weaponDef->HasWeapon(n); ++n ) {
			Item ammo( weaponDef->GetClipItemDef( n ) );
			unit[k].GetInventory()->AddItem( ammo, 0 );
		}

		// Add extras
		{
			rank = RandomRank( &aRand, baseRank, NUM_TERRAN_RANKS );
			Item armor( itemDefArr, armorType[rank] );
			unit[k].GetInventory()->AddItem( armor, 0 );
		}
		rank = RandomRank( &aRand, baseRank, NUM_TERRAN_RANKS );
		for( int i=0; i<=rank; ++i ) {
			Item extra( itemDefArr, extraItems[i] );
			unit[k].GetInventory()->AddItem( extra, 0 );
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

