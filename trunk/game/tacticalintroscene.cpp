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

#include "../grinliz/glbitarray.h"
#include "tacticalintroscene.h"
#include "../engine/uirendering.h"
#include "../engine/engine.h"
#include "game.h"

#include <string>
using namespace std;
using namespace grinliz;

TacticalIntroScene::TacticalIntroScene( Game* _game ) : Scene( _game )
{
	showNewChoices = false;

	Engine* engine = GetEngine();

	// -- Background -- //
	background = new UIImage( engine->GetScreenport() );
	const Texture* bg = TextureManager::Instance()->GetTexture( "intro" );
	GLASSERT( bg );
	background->Init( bg, 480, 320 );

	backgroundNew = new UIImage( engine->GetScreenport() );
	backgroundNew->Init( TextureManager::Instance()->GetTexture( "newscreen" ), 480, 320 );

	// -- Buttons -- //
	{
		buttons = new UIButtonBox( engine->GetScreenport() );

		int icons[] = { ICON_GREEN_BUTTON, ICON_GREEN_BUTTON, ICON_GREEN_BUTTON };
		const char* iconText[] = { "test", "new", "continue" };
		buttons->InitButtons( icons, 3 );
		buttons->SetOrigin( 42, 40 );
		buttons->SetButtonSize( 120, 50 );
		buttons->SetText( iconText );
	}

	// -- New Game choices -- //
	{
		choices = new UIButtonGroup( engine->GetScreenport() );
	
		const int XSIZE = 50;
		const int YSIZE = 50;
		int h = engine->GetScreenport().UIHeight()-1;

		int icons[OPTION_COUNT] = { ICON_GREEN_BUTTON };

		choices->SetOrigin( 0, 0 );
		choices->InitButtons( icons, OPTION_COUNT );

		choices->SetBG( SQUAD_4, 20+XSIZE*0, h-124, ICON_GREEN_BUTTON, DECO_NONE, "4", false );
		choices->SetBG( SQUAD_8, 20+XSIZE*1, h-124, ICON_GREEN_BUTTON, DECO_NONE, "8", true );
		choices->SetBG( TERRAN_LOW,  20+XSIZE*0, h-124+YSIZE*1, ICON_GREEN_BUTTON, DECO_NONE, "Low", false );
		choices->SetBG( TERRAN_MED,  20+XSIZE*1, h-124+YSIZE*1, ICON_GREEN_BUTTON, DECO_NONE, "Med", true );
		choices->SetBG( TERRAN_HIGH, 20+XSIZE*2, h-124+YSIZE*1, ICON_GREEN_BUTTON, DECO_NONE, "Hi", false );

		choices->SetBG( ALIEN_8,  20+XSIZE*0, h-245, ICON_GREEN_BUTTON, DECO_NONE, "8", true );
		choices->SetBG( ALIEN_16, 20+XSIZE*1, h-245, ICON_GREEN_BUTTON, DECO_NONE, "16", false );
		choices->SetBG( ALIEN_LOW,  20+XSIZE*0, h-245+YSIZE*1, ICON_GREEN_BUTTON, DECO_NONE, "Low", true );
		choices->SetBG( ALIEN_MED,  20+XSIZE*1, h-245+YSIZE*1, ICON_GREEN_BUTTON, DECO_NONE, "Med", false );
		choices->SetBG( ALIEN_HIGH, 20+XSIZE*2, h-245+YSIZE*1, ICON_GREEN_BUTTON, DECO_NONE, "Hi", false );

		choices->SetBG( TIME_DAY,   20+XSIZE*0, h-316, ICON_GREEN_BUTTON, DECO_NONE, "Day", true );
		choices->SetBG( TIME_NIGHT, 20+XSIZE*1, h-316, ICON_GREEN_BUTTON, DECO_NONE, "Night", false );

		choices->SetBG( LOC_FARM,	210, h-124,	ICON_GREEN_BUTTON, DECO_NONE, "Farm", true );
		choices->SetBG( GO_NEW_GAME, 360, h-317, ICON_GREEN_BUTTON, DECO_NONE, "GO!", false );
	}

	// Is there a current game?
	const std::string& savePath = game->GameSavePath();
	buttons->SetEnabled( CONTINUE_GAME, false );
	FILE* fp = fopen( savePath.c_str(), "r" );
	if ( fp ) {
		fseek( fp, 0, SEEK_END );
		unsigned long len = ftell( fp );
		if ( len > 100 ) {
			// 20 ignores empty XML noise (hopefully)
			buttons->SetEnabled( CONTINUE_GAME, true );
		}
		fclose( fp );
	}
}


TacticalIntroScene::~TacticalIntroScene()
{
	delete background;
	delete backgroundNew;
	delete buttons;
	delete choices;
}


void TacticalIntroScene::DrawHUD()
{
	if ( showNewChoices ) {
		backgroundNew->Draw();
		choices->Draw();
	}
	else {
		background->Draw();
		buttons->Draw();
	}
}


void TacticalIntroScene::Tap(	int count, 
								const Vector2I& screen,
								const Ray& world )
{
	int ux, uy;
	GetEngine()->GetScreenport().ViewToUI( screen.x, screen.y, &ux, &uy );

	if ( !showNewChoices ) {
		int tap = buttons->QueryTap( ux, uy );
		switch ( tap ) {
			case TEST_GAME:			game->loadRequested = 2;			break;
			case NEW_GAME:			showNewChoices = true;				break;
			case CONTINUE_GAME:		game->loadRequested = 0;			break;

			default:
				break;
		}
	}
	else {
		int tap = choices->QueryTap( ux, uy );
		switch ( tap ) {
			case SQUAD_4:	
			case SQUAD_8:
				choices->SetHighLight( SQUAD_4, (tap==SQUAD_4) );
				choices->SetHighLight( SQUAD_8, (tap==SQUAD_8) );
				break;

			case TERRAN_LOW:	
			case TERRAN_MED:
			case TERRAN_HIGH:
				choices->SetHighLight( TERRAN_LOW, (tap==TERRAN_LOW) );
				choices->SetHighLight( TERRAN_MED, (tap==TERRAN_MED) );
				choices->SetHighLight( TERRAN_HIGH, (tap==TERRAN_HIGH) );
				break;

			case ALIEN_8:	
			case ALIEN_16:
				choices->SetHighLight( ALIEN_8,  (tap==ALIEN_8) );
				choices->SetHighLight( ALIEN_16, (tap==ALIEN_16) );
				break;

			case ALIEN_LOW:	
			case ALIEN_MED:
			case ALIEN_HIGH:
				choices->SetHighLight( ALIEN_LOW, (tap==ALIEN_LOW) );
				choices->SetHighLight( ALIEN_MED, (tap==ALIEN_MED) );
				choices->SetHighLight( ALIEN_HIGH, (tap==ALIEN_HIGH) );
				break;

			case TIME_DAY:	
			case TIME_NIGHT:
				choices->SetHighLight( TIME_DAY,  (tap==TIME_DAY) );
				choices->SetHighLight( TIME_NIGHT, (tap==TIME_NIGHT) );
				break;

			case GO_NEW_GAME:
				game->loadRequested = 1;
				WriteXML( &game->newGameXML );
				break;
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
	sceneElement->LinkEndChild( battleElement );
	
	battleElement->SetAttribute( "dayTime", choices->GetHighLight( TIME_NIGHT ) ? 0 : 1 );

	CreateMap( gameElement, 0, LOC_FARM, 1 );

	{
		std::string buf = "<Units>";
		const gamedb::Reader* database = game->GetDatabase();
		const gamedb::Item* parent = database->Root()->Child( "data" );
		const gamedb::Item* item = 0;

		// Terran units
		{
			if ( choices->GetHighLight( TERRAN_LOW ) )
				item = parent->Child( "new_squad_LA" );
			else if ( choices->GetHighLight( TERRAN_MED ) )
				item = parent->Child( "new_squad_MA" );
			else if ( choices->GetHighLight( TERRAN_HIGH ) )
				item = parent->Child( "new_squad_HA" );

			const char* squad = (const char*)database->AccessData( item, "binary" );

			buf += squad;
			if ( choices->GetHighLight( SQUAD_8 ) ) {
				buf += squad;
			}
		}
		// Alien units
		{
			if ( choices->GetHighLight( ALIEN_LOW ) )
				item = parent->Child( "new_alien_LA" );
			else if ( choices->GetHighLight( ALIEN_MED ) )
				item = parent->Child( "new_alien_MA" );
			else if ( choices->GetHighLight( ALIEN_HIGH ) )
				item = parent->Child( "new_alien_HA" );

			const char* alien = (const char*)database->AccessData( item, "binary" );

			buf += alien;
			if ( choices->GetHighLight( ALIEN_16 ) ) {
				buf += alien;
			}
		}
		buf += "</Units>";

		TiXmlElement* unitElement = new TiXmlElement( "Units" );
		unitElement->Parse( buf.c_str(), 0, TIXML_ENCODING_UTF8 );
		gameElement->LinkEndChild( unitElement );
	}

#if 1
	{
		TiXmlDocument* doc = xml->ToDocument();
		if ( doc ) 
			doc->SaveFile( "testnewgame.xml" );
	}
#endif
}


void TacticalIntroScene::CalcInfo( int location, int ufoSize, SceneInfo* info )
{
	switch ( location ) {
		case LOC_FARM:
			info->base = "FARM";
			info->blockSize = 3;	// ufosize doesn't matter
			info->needsLander = true;
			info->ufo = 1;
			break;

		default:
			GLASSERT( 0 );
	}
}


void TacticalIntroScene::CreateMap( TiXmlNode* parent, 
									int seed,
									int location,
									int ufoSize )
{
	// Max world size is 64x64 units, in 16x16 unit blocks. That gives 4x4 blocks max.
	BitArray< 4, 4, 1 > blocks;

	TiXmlElement* mapElement = new TiXmlElement( "Map" );
	TiXmlElement* imageElement = new TiXmlElement( "Images" );
	mapElement->LinkEndChild( imageElement );

	SceneInfo info;
	CalcInfo( location, ufoSize, &info );
	
	Random random( seed );
	for( int i=0; i<5; ++i ) 
		random.Rand();

	Vector2I landerPos = { 0, 0 };
	Vector2I ufoPos = { 0, 0 };

	if ( info.needsLander ) {
		Vector2I pos = { random.Bit() ? info.blockSize-1 : 0, random.Bit() ? info.blockSize-1 : 0 };
		blocks.Set( pos.x, pos.y );

	}
}
