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


using namespace grinliz;
using namespace gamui;


TacticalIntroScene::TacticalIntroScene( Game* _game ) : Scene( _game )
{
	Engine* engine = GetEngine();
	const Screenport& port = engine->GetScreenport();

	GLOUTPUT(( "TacticalIntroScene screen=%.1f,%.1f\n", engine->GetScreenport().UIWidth(), engine->GetScreenport().UIHeight() ));

	// -- Background -- //
	gamui::RenderAtom nullAtom;

	backgroundUI.Init( game, &gamui2D, true );
	//backgroundUI.backgroundText.SetPos( 200, backgroundUI.background.Y() );

	const gamui::ButtonLook& green = game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );

	static const float BORDER = 25;

	continueButton.Init( &gamui2D, green );
	continueButton.SetSizeByScale( 2.2f, 1 );
	continueButton.SetPos( BORDER, 320-BORDER-continueButton.Height() );
	continueButton.SetText( "Continue" );

	/*
	newCampaign.Init( &gamui2D, green );
	newCampaign.SetPos( LEFT, 250 );
	newCampaign.SetSizeByScale( 2.2f, 1 );
	newCampaign.SetText( "Campaign" );
	newCampaign.SetEnabled( false );
	*/

	newTactical.Init( &gamui2D, green );
	newTactical.SetSizeByScale( 2.2f, 1 );
	newTactical.SetPos( port.UIWidth()-BORDER-newTactical.Width(), 320-BORDER-continueButton.Height() );
	newTactical.SetText( "New Game" );

	
	newGeo.Init( &gamui2D, green );
	newGeo.SetPos( port.UIWidth()-BORDER*2-newTactical.Width()*2, 320-BORDER-continueButton.Height() );
	newGeo.SetSizeByScale( 2.2f, 1 );
	newGeo.SetText( "New Geo" );
	

	helpButton.Init( &gamui2D, green );
	//helpButton.SetPos( port.UIWidth() - helpButton.Width() - BORDER, BORDER );
	helpButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_HELP, true ),
						UIRenderer::CalcDecoAtom( DECO_HELP, false ) );	

	audioButton.Init( &gamui2D, green );
	//audioButton.SetPos( port.UIWidth() - helpButton.Width() - BORDER, helpButton.Y() + helpButton.Height() + BORDER );
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

	infoButton.Init( &gamui2D, green );
	infoButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_INFO, true ),
						UIRenderer::CalcDecoAtom( DECO_INFO, false ) );	

	UIItem* items[3] = { &helpButton, &audioButton, &infoButton };
	Gamui::Layout( items, 3, 1, 3, port.UIWidth() - helpButton.Width() - BORDER, BORDER, helpButton.Width(), helpButton.Height()*3.0f+BORDER );


	static const char* toggleLabel[TOGGLE_COUNT] = { "4", "6", "8", "Low", "Med", "Hi", "8", "12", "16", "Low", "Med", "Hi", "Day", "Night",
													 "Fa-S", "T-S", "Fo-S", "D-S", "Fa-D", "T-D", "Fo-D", "D-D",
													 "City", "BattleShip",	// "ABase", "TBase",
													 "Civs", "Crash" };
	for( int i=0; i<TOGGLE_COUNT; ++i ) {
		GLASSERT( toggleLabel[i] );
		toggles[i].Init( &gamui2D, ( i < CIVS_PRESENT ) ? green : blue );
		toggles[i].SetText( toggleLabel[i] );
		toggles[i].SetVisible( false );
		toggles[i].SetSize( 50, 50 );
	}
	toggles[BATTLESHIP].SetSize( 100, 50 );

	toggles[SQUAD_4].AddToToggleGroup( &toggles[SQUAD_6] );
	toggles[SQUAD_4].AddToToggleGroup( &toggles[SQUAD_8] );

	toggles[TERRAN_LOW].AddToToggleGroup( &toggles[TERRAN_MED] );
	toggles[TERRAN_LOW].AddToToggleGroup( &toggles[TERRAN_HIGH] );

	toggles[ALIEN_8].AddToToggleGroup( &toggles[ALIEN_12] );
	toggles[ALIEN_8].AddToToggleGroup( &toggles[ALIEN_16] );

	toggles[ALIEN_LOW].AddToToggleGroup( &toggles[ALIEN_MED] );
	toggles[ALIEN_LOW].AddToToggleGroup( &toggles[ALIEN_HIGH] );

	toggles[TIME_DAY].AddToToggleGroup( &toggles[TIME_NIGHT] );

	//toggles[SCEN_LANDING].AddToToggleGroup( &toggles[SCEN_CRASH] );

	toggles[SQUAD_8].SetDown();
	toggles[TERRAN_MED].SetDown();
	toggles[ALIEN_8].SetDown();
	toggles[ALIEN_MED].SetDown();
	toggles[TIME_DAY].SetDown();
	toggles[FARM_SCOUT].SetDown();

	toggles[CIVS_PRESENT].SetDown();

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

	UIItem* alienItems[] = { &toggles[ALIEN_8], &toggles[ALIEN_12], &toggles[ALIEN_16] };
	Gamui::Layout(	alienItems, 3,			// alien #
					4, 1, 
					20, 150,
					150, 50 );
	UIItem* alienStrItems[] = { &toggles[ALIEN_LOW], &toggles[ALIEN_MED], &toggles[ALIEN_HIGH] };
	Gamui::Layout(	alienStrItems, 3,			// alien strength
					4, 1, 
					20, 200,
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
//		scenarioLabel[i].SetPos( 215+SIZE*i, 5 );
	}

	for( int i=0; i<4; ++i ) {
		toggles[FARM_SCOUT+i].SetPos( 215+SIZE*i, 25 );
		toggles[FARM_DESTROYER+i].SetPos( 215+SIZE*i, 75 );
	}
	toggles[CITY].SetPos( 215+SIZE*0, 125 );
	toggles[BATTLESHIP].SetPos( 215+SIZE*1, 125 );

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
	//toggles[ALIEN_BASE].SetEnabled(		false );
	//toggles[TERRAN_BASE].SetEnabled(	false );


	UIItem* scenItems[] = { &toggles[CIVS_PRESENT], &toggles[UFO_CRASH] };
	Gamui::Layout( scenItems, 2,
				   4, 2,
				   215, 175,
				   100, 100 );
							
	goButton.Init( &gamui2D, blue );
	goButton.SetPos( 360, 270 );
	goButton.SetSize( 100, 50 );
	goButton.SetText( "Go!" );
	goButton.SetVisible( false );

	// Is there a current game?
	GLString savePath = game->GameSavePath();
	continueButton.SetEnabled( false );
	FILE* fp = fopen( savePath.c_str(), "r" );
	if ( fp ) {
		fseek( fp, 0, SEEK_END );
		unsigned long len = ftell( fp );
		if ( len > 100 ) {
			// 20 ignores empty XML noise (hopefully)
			continueButton.SetEnabled( true );
		}
		fclose( fp );
	}
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


	if ( item == &newTactical ) {
		newTactical.SetVisible( false );
		//newCampaign.SetVisible( false );
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
		infoButton.SetVisible( false );
	}
	else if ( item == &continueButton ) {
		onToNext = Game::BATTLE_SCENE;
	}
	else if ( item == &goButton ) {
		GLString path = game->GameSavePath();
		FILE* fp = fopen( path.c_str(), "w" );
		GLASSERT( fp );
		if ( fp ) {
			onToNext = Game::BATTLE_SCENE;
			WriteXML( fp );
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
	else if ( item == &infoButton ) {
		game->SetDebugLevel( (game->GetDebugLevel() + 1)%4 );
	}
	else if ( item == &newGeo ) {
		onToNext = Game::GEO_SCENE;
	}


	if ( onToNext >= 0 ) {
		game->PopScene();
		game->PushScene( onToNext, 0 );
	}

}


void TacticalIntroScene::WriteXML( FILE* fp )
 {
	//	Game
	//		Scene
	//		BattleScene
	//		Map
	//			Seen
	//			Items
	//			Images
	//			GroundDebris
	//		Units
	//			Unit
	
	XMLUtil::OpenElement( fp, 0, "Game" );
	XMLUtil::SealElement( fp );

	XMLUtil::OpenElement( fp, 1, "Scene" );
	XMLUtil::Attribute( fp, "id", Game::BATTLE_SCENE );
	XMLUtil::SealCloseElement( fp );

	XMLUtil::OpenElement( fp, 1, "BattleScene" );
	XMLUtil::Attribute( fp, "dayTime", toggles[TIME_DAY].Down() ? 1 : 0 );
	XMLUtil::SealCloseElement( fp );

	int seed = (int)time( 0 ) + (int)clock();

	SceneInfo info;
	info.scenario = FARM_SCOUT;
	for( int i=FIRST_SCENARIO; i<=LAST_SCENARIO; ++i ) {
		if ( toggles[i].Down() ) {
			info.scenario = i;
			break;
		}
	}
	info.crash = false;
	info.nCivs = 0;

	if ( info.SupportsCivs() )
		info.nCivs = toggles[CIVS_PRESENT].Down() ? 8 : 0;
	if ( info.SupportsCrash() )
		info.crash = toggles[UFO_CRASH].Down() ? true : false;

	CreateMap( fp, seed, info );
	fprintf( fp, "\n" );

	XMLUtil::OpenElement( fp, 1, "Units" );
	XMLUtil::SealElement( fp );

	Unit units[MAX_UNITS];
	int rank = 0;
	int count = 8;
	int types[Unit::NUM_ALIEN_TYPES] = { 0 };

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

	memset( units, 0, sizeof(Unit)*MAX_UNITS );
	GenerateTerranTeam( units, count, rank, game->GetItemDefArr(), seed );
	for( int i=0; i<count; ++i ) {
		if ( units[i].IsAlive() ) {
			units[i].Save( fp, 2 );
		}
	}

	// Alien units
	if ( toggles[ALIEN_8].Down() )
		count = 8;
	else if ( toggles[ALIEN_12].Down() )
		count = 12;
	else if ( toggles[ALIEN_16].Down() )
		count = 16;

	if ( toggles[ALIEN_LOW].Down() ) {
		types[Unit::ALIEN_GREEN] = count*3/4;
		types[Unit::ALIEN_HORNET] = count*1/4;
		rank = 0;
	}
	else if ( toggles[ALIEN_MED].Down() ) {
		types[Unit::ALIEN_GREEN] = count*1/4;
		types[Unit::ALIEN_HORNET] = count*1/4;
		types[Unit::ALIEN_JACKAL] = count*1/4;
		types[Unit::ALIEN_VIPER] = count*1/4;
		rank = 2;
	}
	else if ( toggles[ALIEN_HIGH].Down() ) {
		types[Unit::ALIEN_PRIME] = count*1/4;
		types[Unit::ALIEN_HORNET] = count*1/4;
		types[Unit::ALIEN_JACKAL] = count*1/4;
		types[Unit::ALIEN_VIPER] = count*1/4;
		rank = 4;
	}

	memset( units, 0, sizeof(Unit)*MAX_UNITS );
	GenerateAlienTeam( units, types, (float)rank, game->GetItemDefArr(), seed );

	int created=0;
	for( int i=0; i<MAX_ALIENS; ++i ) {
		if ( units[i].IsAlive() ) {
			units[i].Save( fp, 2 );
			++created;
		}
	}
	GLASSERT( created == count );

	// Civ team
	memset( units, 0, sizeof(Unit)*MAX_UNITS );
	GenerateCivTeam( units, info.nCivs, game->GetItemDefArr(), seed );
	for( int i=0; i<MAX_CIVS; ++i ) {
		if ( units[i].IsAlive() ) {
			units[i].Save( fp, 2 );
		}
	}

	XMLUtil::CloseElement( fp, 1, "Units" );
	XMLUtil::CloseElement( fp, 0, "Game" );		
}


void TacticalIntroScene::FindNodes(	const char* set,
									int size,
									const char* type,
									const gamedb::Item* parent )
{
	char buffer[64];
	SNPrintf( buffer, 64, "%4s_%02d_%4s", set, size, type );
	const int LEN=12;
	nItemMatch = 0;

	for( int i=0; i<parent->NumChildren(); ++i ) {
		const gamedb::Item* node = parent->Child( i );
		const char* name = node->Name();

		if ( strncmp( name, buffer, LEN ) == 0 ) {
			itemMatch[ nItemMatch++ ] = node;
		}
	}
}


void TacticalIntroScene::AppendMapSnippet(	int dx, int dy, int tileRotation,
											const char* set,
											int size,
											bool crash,
											const char* type,
											const gamedb::Item* parent,
											TiXmlElement* mapElement )
{
	FindNodes( set, size, type, parent );
	GLASSERT( nItemMatch > 0 );
	random.Rand();
	int seed = random.Rand( nItemMatch );
	const gamedb::Item* item = itemMatch[ seed ];
	const char* xmlText = (const char*) game->GetDatabase()->AccessData( item, "binary" );

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
			Map* map = game->engine->GetMap();
			const MapItemDef* mapItemDef = map->GetItemDef( ele->Attribute( "name" ) );

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
									const SceneInfo& info )
{
	// Max world size is 64x64 units, in 16x16 unit blocks. That gives 4x4 blocks max.
	BitArray< 4, 4, 1 > blocks;

	TiXmlElement mapElement( "Map" );
	const gamedb::Item* dataItem = game->GetDatabase()->Root()->Child( "data" );

	Vector2I size = info.Size();
	mapElement.SetAttribute( "sizeX", size.x*16 );
	mapElement.SetAttribute( "sizeY", size.y*16 );
	
	random.SetSeed( seed );
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

			AppendMapSnippet( pos.x*16, pos.y*16, tileRotation, info.Base(), 16, false, "LAND", dataItem, &mapElement );	
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
			AppendMapSnippet( pos.min.x*16, pos.min.y*16, tileRotation, info.Base(), 16*ufoSize, info.crash, info.UFO(), dataItem, &mapElement );
		}

		for( int j=0; j<size.y; ++j ) {
			for( int i=0; i<size.x; ++i ) {
				if ( !blocks.IsSet( i, j ) ) {
					Vector2I pos = { i, j };
					int tileRotation = random.Rand(4);
					AppendMapSnippet( pos.x*16, pos.y*16, tileRotation, info.Base(), 16, false, "TILE", dataItem, &mapElement );	
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
			AppendMapSnippet( lander.x*16, lander.y*16, tileRotation, "CITY", 16, false, "LAND", dataItem, &mapElement );	
		}

		// Open
		for( int i=0; i<4; ++i ) {
			if ( lander != open[i] ) {
				int tileRotation = random.Rand(4);
				AppendMapSnippet( open[i].x*16, open[i].y*16, tileRotation, "CITY", 16, false, "OPEN", dataItem, &mapElement );	
			}
		}
		// Roads
		for( int i=0; i<4; ++i ) {
			AppendMapSnippet( hroad[i].x*16, hroad[i].y*16, 1, "CITY", 16, false, "ROAD", dataItem, &mapElement );	
		}
		for( int i=0; i<4; ++i ) {
			AppendMapSnippet( vroad[i].x*16, vroad[i].y*16, 0, "CITY", 16, false, "ROAD", dataItem, &mapElement );	
		}
		for( int i=0; i<4; ++i ) {
			AppendMapSnippet( inter[i].x*16, inter[i].y*16, 1, "CITY", 16, false, "INTR", dataItem, &mapElement );	
		}
		
	}
	else if ( info.scenario == BATTLESHIP ) {
		AppendMapSnippet( 0, 0, 0, "BATT", 48, false, "TILE", dataItem, &mapElement );	
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
				units[i].DoDamage( dd, 0 );
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
		{	"RAY-1",	"RAY-1",	"RAY-1",	"RAY-2",	"RAY-3" },		// green
		{	"RAY-2",	"RAY-2",	"RAY-3",	"RAY-3",	"PLS-3"	},		// prime
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
			if ( i >= 2 && index % 3 == 0 ) {
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
												int averageRank,
												const ItemDefArr& itemDefArr,
												int seed )
{
	static const int POSITION = 4;
	static const char* weapon[POSITION][NUM_RANKS] = {
		{	"ASLT-1",	"ASLT-1",	"ASLT-2",	"ASLT-2",	"ASLT-3" },		// assault
		{	"ASLT-1",	"PLS-1",	"ASLT-2",	"PLS-2",	"PLS-3" },		// assault
		{	"LR-1",		"LR-1",		"LR-2",		"LR-2",		"LR-3" },		// sniper
		{	"MCAN-1",	"MCAN-1",	"MCAN-2",	"STRM-2",	"MCAN-3" },	// heavy
	};
	static const char* armorType[NUM_RANKS] = {
		"ARM-1", "ARM-2", "ARM-2", "ARM-3", "ARM-3"
	};
	static const char* extraItems[NUM_RANKS] = {
		"", "", "SG:I", "SG:K", "SG:E"
	};

	// local random - the same inputs always create same outputs.
	grinliz::Random aRand( (count*averageRank) ^ seed );
	aRand.Rand();

	for( int k=0; k<count; ++k ) 
	{
		int position = k % POSITION;

		// Create the unit.
		int rank = Clamp( averageRank + aRand.Sign()*aRand.Bit(), 0, NUM_RANKS-1 );
 		unit[k].Create( TERRAN_TEAM, 0, rank, aRand.Rand() );

		rank = Clamp( averageRank + aRand.Sign()*aRand.Bit(), 0, NUM_RANKS-1 );

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
			rank = Clamp( averageRank + aRand.Sign()*aRand.Bit(), 0, NUM_RANKS-1 );
			Item armor( itemDefArr, armorType[rank] );
			unit[k].GetInventory()->AddItem( armor );
		}

		rank = Clamp( averageRank + aRand.Sign()*aRand.Bit(), 0, NUM_RANKS-1 );
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
	else if ( scenario == BATTLESHIP ) {
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

