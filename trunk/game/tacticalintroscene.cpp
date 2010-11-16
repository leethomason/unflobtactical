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


using namespace grinliz;
using namespace gamui;


TacticalIntroScene::TacticalIntroScene( Game* _game ) : Scene( _game )
{
	Engine* engine = GetEngine();
	const Screenport& port = engine->GetScreenport();

	GLOUTPUT(( "TacticalIntroScene screen=%.1f,%.1f\n", engine->GetScreenport().UIWidth(), engine->GetScreenport().UIHeight() ));

	// -- Background -- //
	gamui::RenderAtom nullAtom;

	backgroundUI.Init( game, &gamui2D );

	const gamui::ButtonLook& green = game->GetButtonLook( Game::GREEN_BUTTON );
	const gamui::ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );

	continueButton.Init( &gamui2D, green );
	continueButton.SetPos( 50, 150 );
	continueButton.SetSizeByScale( 2.0f, 1.0f );
	continueButton.SetText( "Continue" );

	newButton.Init( &gamui2D, green );
	newButton.SetPos( 50, 220 );
	newButton.SetSizeByScale( 2.0f, 1.0f );
	newButton.SetText( "New" );

	helpButton.Init( &gamui2D, green );
	helpButton.SetPos( port.UIWidth() - 100, 220 );
	helpButton.SetDeco( UIRenderer::CalcDecoAtom( DECO_HELP, true ),
						UIRenderer::CalcDecoAtom( DECO_HELP, false ) );	

	audioButton.Init( &gamui2D, green );
	audioButton.SetPos( port.UIWidth() - 100, 150 );
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

	static const char* toggleLabel[TOGGLE_COUNT] = { "4", "8", "Low", "Med", "Hi", "8", "16", "Low", "Med", "Hi", "Day", "Night", "Farm", "Land", "Crash" };
	for( int i=0; i<TOGGLE_COUNT; ++i ) {
		GLASSERT( toggleLabel[i] );
		toggles[i].Init( &gamui2D, green );
		toggles[i].SetText( toggleLabel[i] );
		toggles[i].SetVisible( false );
		toggles[i].SetSize( 50, 50 );
	}

	toggles[SQUAD_4].AddToToggleGroup( &toggles[SQUAD_8] );

	toggles[TERRAN_LOW].AddToToggleGroup( &toggles[TERRAN_MED] );
	toggles[TERRAN_LOW].AddToToggleGroup( &toggles[TERRAN_HIGH] );

	toggles[ALIEN_8].AddToToggleGroup( &toggles[ALIEN_16] );

	toggles[ALIEN_LOW].AddToToggleGroup( &toggles[ALIEN_MED] );
	toggles[ALIEN_LOW].AddToToggleGroup( &toggles[ALIEN_HIGH] );

	toggles[TIME_DAY].AddToToggleGroup( &toggles[TIME_NIGHT] );

	toggles[SCEN_LANDING].AddToToggleGroup( &toggles[SCEN_CRASH] );

	toggles[SQUAD_8].SetDown();
	toggles[TERRAN_MED].SetDown();
	toggles[ALIEN_8].SetDown();
	toggles[ALIEN_MED].SetDown();
	toggles[TIME_DAY].SetDown();
	toggles[LOC_FARM].SetDown();
	toggles[SCEN_LANDING].SetDown();

	terranLabel.Init( &gamui2D );
	terranLabel.SetVisible( false );
	terranLabel.SetText( "Terran Squad" );
	terranLabel.SetPos( 20, 25-20 );

	UIItem* squadItems[] = { &toggles[0], &toggles[1] };
	Gamui::Layout(	squadItems, 2,			// squad #
					4, 1, 
					20, 25,
					150, 50 );

	UIItem* squadStrItems[] = { &toggles[2], &toggles[3], &toggles[4] };
	Gamui::Layout(	squadStrItems, 3,			// squad strength
					4, 1, 
					20, 75,
					150, 50 );

	alienLabel.Init( &gamui2D );
	alienLabel.SetVisible( false );
	alienLabel.SetText( "Aliens" );
	alienLabel.SetPos( 20, 150-20 );

	UIItem* alienItems[] = { &toggles[5], &toggles[6] };
	Gamui::Layout(	alienItems, 2,			// alien #
					4, 1, 
					20, 150,
					150, 50 );
	UIItem* alienStrItems[] = { &toggles[7], &toggles[8], &toggles[9] };
	Gamui::Layout(	alienStrItems, 3,			// alien strength
					4, 1, 
					20, 200,
					150, 50 );

	timeLabel.Init( &gamui2D );
	timeLabel.SetVisible( false );
	timeLabel.SetText( "Time" );
	timeLabel.SetPos( 20, 270-20 );
	UIItem* weatherItems[] = { &toggles[10], &toggles[11] };
	Gamui::Layout(	weatherItems, 2,		// weather
					2, 1, 
					20, 270,
					100, 50 );

	scenarioLabel.Init( &gamui2D );
	scenarioLabel.SetVisible( false );
	scenarioLabel.SetText( "Scenario" );
	scenarioLabel.SetPos( 215, 25-20 );

	UIItem* locationItems[] = { &toggles[12] };
	Gamui::Layout(  locationItems, 1,		// location
					5, 2,
					215, 25,
					250, 100 );

	UIItem* scenItems[] = { &toggles[SCEN_LANDING], &toggles[SCEN_CRASH] };
	Gamui::Layout( scenItems, 2,
				   5, 2,
				   215, 75,
				   250, 100 );
							
	goButton.Init( &gamui2D, blue );
	goButton.SetPos( 360, 270 );
	goButton.SetSize( 100, 50 );
	goButton.SetText( "Go!" );
	goButton.SetVisible( false );

	seedLabel.Init( &gamui2D );
	seedLabel.SetVisible( false );
	seedLabel.SetText( "Seed" );
	seedLabel.SetPos( 155, 270-20 );
	seedButton.Init( &gamui2D, green );
	seedButton.SetPos( 155, 270 );
	seedButton.SetSize( 50, 50 );

	{
		int t = (int)time( 0 ) + (int)clock();
		Random random( t );
		char buf[5];
		SNPrintf( buf, 5, "%d", random.Rand( 1000 ) );
		seedButton.SetText( buf );
	}
	seedButton.SetVisible( false );

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


	if ( item == &newButton ) {
		newButton.SetVisible( false );
		continueButton.SetVisible( false );
		for( int i=0; i<TOGGLE_COUNT; ++i ) {
			toggles[i].SetVisible( true );
		}
		goButton.SetVisible( true );
		seedButton.SetVisible( true );
		backgroundUI.backgroundText.SetVisible( false );

		terranLabel.SetVisible( true );
		alienLabel.SetVisible( true );
		timeLabel.SetVisible( true );
		seedLabel.SetVisible( true );
		scenarioLabel.SetVisible( true );
		
		helpButton.SetVisible( false );
		audioButton.SetVisible( false );
	}
	else if ( item == &continueButton ) {
		game->loadRequested = 0;
	}
	else if ( item == &goButton ) {
		game->loadRequested = 1;
		game->newGameXML.Clear();
		WriteXML( &game->newGameXML );
	}
	else if ( item == &seedButton ) {
		const char* seedStr = seedButton.GetText();
		int seed = atol( seedStr );
		seed += 1;
		if ( seed >= 10 )
			seed = 0;
		if ( seed < 0 )
			seed = 0;

		char buffer[16];
		SNPrintf( buffer, 16, "%d", seed );
		seedButton.SetText( buffer );
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
	if ( game->loadRequested >= 0 ) {
		game->PopScene();
		game->PushScene( Game::BATTLE_SCENE, 0 );
	}

}


void TacticalIntroScene::WriteXML( TiXmlNode* xml )
{
	//	Game
	//		Scene
	//			BattleScene
	//		Map
	//			Images
	//			Items
	//			GroundDebris
	//		Units

	TiXmlElement* gameElement = new TiXmlElement( "Game" );
	TiXmlElement* sceneElement = new TiXmlElement( "Scene" );
	TiXmlElement* battleElement = new TiXmlElement( "BattleScene" );

	xml->LinkEndChild( gameElement );
	gameElement->LinkEndChild( sceneElement );
	gameElement->LinkEndChild( battleElement );
	
	battleElement->SetAttribute( "dayTime", toggles[TIME_DAY].Down() ? 1 : 0 );

	const char* seedStr = seedButton.GetText();
	int seed = atol( seedStr );

	int scenario = SCEN_LANDING;
	if ( toggles[SCEN_CRASH].Down() ) {
		scenario = SCEN_CRASH;
	}

	CreateMap( gameElement, seed, LOC_FARM, scenario, 1 );

	{
		GLString buf = "<Units>";
		const gamedb::Reader* database = game->GetDatabase();
		const gamedb::Item* parent = database->Root()->Child( "data" );
		const gamedb::Item* item = 0;

		// Terran units
		{
			if ( toggles[TERRAN_LOW].Down() )
				item = parent->Child( "new_squad_LA" );
			else if ( toggles[TERRAN_MED].Down() )
				item = parent->Child( "new_squad_MA" );
			else if ( toggles[TERRAN_HIGH].Down() )
				item = parent->Child( "new_squad_HA" );

			const char* squad = (const char*)database->AccessData( item, "binary" );

			buf += squad;
			if ( toggles[SQUAD_8].Down() ) {
				buf += squad;
			}
		}
		// Alien units
		{
			if ( toggles[ALIEN_LOW].Down() )
				item = parent->Child( "new_alien_LA" );
			else if ( toggles[ALIEN_MED].Down() )
				item = parent->Child( "new_alien_MA" );
			else if ( toggles[ALIEN_HIGH].Down() )
				item = parent->Child( "new_alien_HA" );

			const char* alien = (const char*)database->AccessData( item, "binary" );

			buf += alien;
			if ( toggles[ALIEN_16].Down() ) {
				buf += alien;
			}
		}
		buf += "</Units>";

		TiXmlElement* unitElement = new TiXmlElement( "Units" );
		unitElement->Parse( buf.c_str(), 0, TIXML_ENCODING_UTF8 );
		gameElement->LinkEndChild( unitElement );
	}

#if 0
	{
		TiXmlDocument* doc = xml->ToDocument();
		if ( doc ) 
			doc->SaveFile( "testnewgame.xml" );
	}
#endif
}


void TacticalIntroScene::CalcInfo( int location, int scenario, int ufoSize, SceneInfo* info )
{
	info->base = "FARM";
	info->blockSizeX = 3;	// ufosize doesn't matter
	info->blockSizeY = 2;
	info->needsLander = true;
	info->ufoSize = 1;
	info->crash = false;

	switch ( location ) {
		case LOC_FARM:
			break;

		default:
			GLASSERT( 0 );
	}

	switch ( scenario ) {
	case SCEN_CRASH:
		info->crash = true;
		info->ufoSize = 1;
	}
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

	// Patch the off coordinates systems:
	int patchx = 0;	//(tileRotation == 2 || tileRotation == 3 ) ? -1 : 0;
	int patchy = 0;	//(tileRotation == 1 || tileRotation == 2 ) ? -1 : 0;

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

		ele->SetAttribute( "x", v0.x+patchx );
		ele->SetAttribute( "y", v0.y+patchy );
		ele->SetAttribute( "rot", (rot + tileRotation)%4 );

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
}


void TacticalIntroScene::CreateMap(	TiXmlNode* parent, 
									int seed,
									int location,
									int scenario,	
									int ufoSize )
{
	// Max world size is 64x64 units, in 16x16 unit blocks. That gives 4x4 blocks max.
	BitArray< 4, 4, 1 > blocks;

	TiXmlElement* mapElement = new TiXmlElement( "Map" );
	parent->LinkEndChild( mapElement );

	const gamedb::Item* dataItem = game->GetDatabase()->Root()->Child( "data" );

	SceneInfo info;
	CalcInfo( location, scenario, ufoSize, &info );
	mapElement->SetAttribute( "sizeX", info.blockSizeX*16 );
	mapElement->SetAttribute( "sizeY", info.blockSizeY*16 );
	
	random.SetSeed( seed );
	for( int i=0; i<5; ++i ) 
		random.Rand();

	Vector2I cornerPosBlock[2];
	if ( random.Bit() ) {
		cornerPosBlock[0].Set( 0, 0 );
		cornerPosBlock[1].Set( info.blockSizeX-1, info.blockSizeY-1 );
	}
	else {
		cornerPosBlock[0].Set( info.blockSizeX-1, 0 );
		cornerPosBlock[1].Set( 0, info.blockSizeY-1 );
	}
	if ( random.Bit() ) {
		Swap( cornerPosBlock+0, cornerPosBlock+1 );
	}


	if ( info.needsLander ) {
		Vector2I pos = cornerPosBlock[ 0 ];
		blocks.Set( pos.x, pos.y );
		int tileRotation = random.Rand(4);

		AppendMapSnippet( pos.x*16, pos.y*16, tileRotation, info.base, 16, "LAND", dataItem, mapElement );	
	}

	if ( info.ufoSize ) {
		Vector2I pos = cornerPosBlock[ 1 ];
		blocks.Set( pos.x, pos.y );
		int tileRotation = random.Rand(4);

		const char* ufoStr = info.crash ? "UFOB" : "UFOA";
		AppendMapSnippet( pos.x*16, pos.y*16, tileRotation, info.base, 16, ufoStr, dataItem, mapElement );
	}

	for( int j=0; j<info.blockSizeY; ++j ) {
		for( int i=0; i<info.blockSizeX; ++i ) {
			if ( !blocks.IsSet( i, j ) ) {
				Vector2I pos = { i, j };
				int tileRotation = random.Rand(4);
				AppendMapSnippet( pos.x*16, pos.y*16, tileRotation, info.base, 16, "TILE", dataItem, mapElement );	
			}
		}
	}

#ifdef DEBUG
	parent->Print( stdout, 0 );
	fflush( stdout );
#endif
}
