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
#include "scene.h"

#include "battlescene.h"
#include "characterscene.h"
#include "tacticalintroscene.h"
#include "tacticalendscene.h"
#include "tacticalunitscorescene.h"
#include "helpscene.h"
#include "dialogscene.h"
#include "geoscene.h"
#include "geoendscene.h"
#include "basetradescene.h"
#include "buildbasescene.h"
#include "researchscene.h"
#include "settingscene.h"
#include "saveloadscene.h"
#include "newtacticaloptions.h"
#include "newgeooptions.h"

#include "../engine/text.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"
#include "../engine/particle.h"
#include "../engine/gpustatemanager.h"
#include "../engine/renderqueue.h"

#include "../grinliz/glmatrix.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glperformance.h"
#include "../grinliz/glstringutil.h"
#include "../tinyxml/tinyxml.h"
#include "../version.h"

#include "ufosound.h"
#include "gamesettings.h"

#include <time.h>

using namespace grinliz;
using namespace gamui;

extern long memNewCount;

Game::Game( int width, int height, int rotation, const char* path ) :
	battleData( itemDefArr ),
	screenport( width, height, rotation ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 ),
	debugLevel( 0 ),
	suppressText( false ),
	previousTime( 0 ),
	isDragging( false )
{
	//GLRELASSERT( 0 );

	savePath = path;
	char c = savePath[savePath.size()-1];
	if ( c != '\\' && c != '/' ) {	
#ifdef WIN32
		savePath += "\\";
#else
		savePath += "/";
#endif
	}	
	
	Init();

	PushScene( INTRO_SCENE, 0 );
	PushPopScene();
}


// WARNING: strange map maker code
Game::Game( int width, int height, int rotation, const char* path, const TileSetDesc& base ) :
	battleData( itemDefArr ),
	screenport( width, height, rotation ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 ),
	debugLevel( 0 ),
	suppressText( false ),
	previousTime( 0 ),
	isDragging( false )
{
	GLASSERT( Engine::mapMakerMode == true );
	savePath = path;
	char c = savePath[savePath.size()-1];
	if ( c != '\\' && c != '/' ) {	
#ifdef WIN32
		savePath += '\\';
#else
		savePath += '/';
#endif
	}	
	
	Init();
	
	char buffer[128];
	SNPrintf( buffer, 128, "%4s_%2d_%4s_%02d", base.set, base.size, base.type, base.variation );

	mapmaker_xmlFile  = path;
	mapmaker_xmlFile += buffer;
	mapmaker_xmlFile += ".xml";

	GLString	texture  = buffer; 
				texture += "_TEX";
	GLString	dayMap   = buffer;
				dayMap  += "_DAY";
	GLString	nightMap  = buffer;
				nightMap += "_NGT";

	//engine->camera.SetPosWC( -25.f, 45.f, 30.f );	// standard test
	//engine->camera.SetYRotation( -60.f );

	PushScene( BATTLE_SCENE, 0 );
	PushPopScene();
	engine->GetMap()->SetSize( base.size, base.size );

	TiXmlDocument doc( mapmaker_xmlFile.c_str() );
	doc.LoadFile();
	if ( !doc.Error() )
		engine->GetMap()->Load( doc.FirstChildElement( "Map" ) );
	
	engine->CameraLookAt( (float)engine->GetMap()->Width()*0.5f, (float)engine->GetMap()->Height()*0.5f, engine->camera.PosWC().y );
}


void Game::Init()
{
	mainPalette = 0;
	mapmaker_showPathing = 0;
	scenePopQueued = false;
	loadSlot = 0;
	currentFrame = 0;
	surface.Set( Surface::RGBA16, 256, 256 );		// All the memory we will ever need (? or that is the intention)

	// Load the database.
	char buffer[260];
	int offset;
	int length;
	PlatformPathToResource( buffer, 260, &offset, &length );
	database0 = new gamedb::Reader();
	database0->Init( 0, buffer, offset );
	database1 = 0;

	GLOUTPUT(( "Game::Init Database initialized.\n" ));

	GLOUTPUT(( "Game::Init stage 0\n" ));
	GameSettingsManager::Create( savePath.c_str() );
	{
		GameSettingsManager* sm = GameSettingsManager::Instance();
		if ( sm->GetCurrentModName().size() ) {
			LoadModDatabase( sm->GetCurrentModName().c_str(), true );
		}
	}

	GLOUTPUT(( "Game::Init stage 10\n" ));
	SoundManager::Create( database0 );
	TextureManager::Create( database0 );
	ImageManager::Create( database0 );
	ModelResourceManager::Create();
	ParticleSystem::Create();

	GLOUTPUT(( "Game::Init stage 20\n" ));
	engine = new Engine( &screenport, database0 );
	GLOUTPUT(( "Game::Init stage 30\n" ));

	LoadTextures();
	GLOUTPUT(( "Game::Init stage 31\n" ));
	modelLoader = new ModelLoader();
	LoadModels();
	GLOUTPUT(( "Game::Init stage 32\n" ));
	LoadItemResources();
	GLOUTPUT(( "Game::Init stage 33\n" ));
	LoadAtoms();
	GLOUTPUT(( "Game::Init stage 34\n" ));
	LoadPalettes();
	GLOUTPUT(( "Game::Init stage 35\n" ));

	delete modelLoader;
	modelLoader = 0;

	GLOUTPUT(( "Game::Init stage 40\n" ));
	Texture* textTexture = TextureManager::Instance()->GetTexture( "font" );
	GLASSERT( textTexture );
	UFOText::Create( database0, textTexture, engine->GetScreenportMutable() );

	faceSurface.Set( Surface::RGBA16, FaceGenerator::SIZE*MAX_TERRANS, FaceGenerator::SIZE );	// harwire sizes for face system
	oneFaceSurface.Set( Surface::RGBA16, FaceGenerator::SIZE, FaceGenerator::SIZE );
	memset( faceCache, 0, sizeof(FaceCache)*MAX_TERRANS );
	faceCacheSlot = 0;

	GLOUTPUT(( "Game::Init stage 50\n" ));
	ImageManager* im = ImageManager::Instance();
	im->LoadImage( "faceChins", &faceGen.chins );
	faceGen.nChins = 17;
	im->LoadImage( "faceMouths", &faceGen.mouths );
	faceGen.nMouths = 14;
	im->LoadImage( "faceNoses", &faceGen.noses );
	faceGen.nNoses = 5;
	im->LoadImage( "faceHairs", &faceGen.hairs );
	faceGen.nHairs = 17;
	im->LoadImage( "faceEyes", &faceGen.eyes );
	faceGen.nEyes = 15;
	im->LoadImage( "faceGlasses", &faceGen.glasses );
	faceGen.nGlasses = 5;

	GLOUTPUT(( "Game::Init complete.\n" ));
}


Game::~Game()
{
	if ( Engine::mapMakerMode ) {
		FILE* fp = fopen( mapmaker_xmlFile.c_str(), "w" );
		GLASSERT( fp );
		if ( fp ) {
			engine->GetMap()->Save( fp, 0 );
			fclose( fp );
		}
	}

	// Roll up to the main scene before saving.
	while( sceneStack.Size() > 1 ) {
		PopScene();
		PushPopScene();
	}

	sceneStack.Top()->scene->DeActivate();
	sceneStack.Top()->Free();
	sceneStack.Pop();

	float predicted, actual;
	WeaponItemDef::CurrentAccData( &predicted, &actual );
	GLOUTPUT(( "Game accuracy: predicted=%.2f actual=%.2f\n", predicted, actual ));

	delete engine;
	UFOText::Destroy();
	GameSettingsManager::Destroy();
	SoundManager::Destroy();
	ParticleSystem::Destroy();
	ModelResourceManager::Destroy();
	ImageManager::Destroy();
	TextureManager::Destroy();
	delete database0;
	delete database1;
}


bool Game::HasSaveFile( SavePathType type, int slot ) const
{
	bool result = false;

	FILE* fp = GameSavePath( type, SAVEPATH_READ, slot );
	if ( fp ) {
		fseek( fp, 0, SEEK_END );
		long d = ftell( fp );
		if ( d > 100 ) {	// has to be something there: sanity check
			result = true;
		}
		fclose( fp );
	}
	return result;
}


void Game::DeleteSaveFile( SavePathType type, int slot )
{
	FILE* fp = GameSavePath( type, SAVEPATH_WRITE, slot );
	if ( fp ) {
		fclose( fp );
	}
}


void Game::SceneNode::Free()
{
	sceneID = Game::NUM_SCENES;
	delete scene;	scene = 0;
	delete data;	data = 0;
	result = INT_MIN;
}


void Game::PushScene( int sceneID, SceneData* data )
{
	GLOUTPUT(( "PushScene %d\n", sceneID ));
	GLASSERT( sceneQueued.sceneID == NUM_SCENES );
	GLASSERT( sceneQueued.scene == 0 );

	sceneQueued.sceneID = sceneID;
	sceneQueued.data = data;
}


void Game::PopScene( int result )
{
	GLOUTPUT(( "PopScene result=%d\n", result ));
	GLASSERT( scenePopQueued == false );
	scenePopQueued = true;
	if ( result != INT_MAX )
		sceneStack.Top()->result = result;
}


void Game::PopAllAndLoad( int slot )
{
	GLASSERT( scenePopQueued == false );
	GLASSERT( slot > 0 );
	scenePopQueued = true;
	loadSlot = slot;
}


void Game::PushPopScene() 
{
	if ( scenePopQueued || sceneQueued.sceneID != NUM_SCENES ) {
		TextureManager::Instance()->ContextShift();
	}

	while ( ( scenePopQueued || loadSlot ) && !sceneStack.Empty() )
	{
		sceneStack.Top()->scene->DeActivate();
		scenePopQueued = false;
		int result = sceneStack.Top()->result;
		int id     = sceneStack.Top()->sceneID;

		sceneStack.Top()->Free();
		sceneStack.Pop();

		if ( !sceneStack.Empty() ) {
			sceneStack.Top()->scene->Activate();
			sceneStack.Top()->scene->Resize();
			if ( result != INT_MIN ) {
				sceneStack.Top()->scene->SceneResult( id, result );
			}
		}
	}

	if ( loadSlot ) {
		if( HasSaveFile( SAVEPATH_GEO, loadSlot ) )
			sceneQueued.sceneID = Game::GEO_SCENE;
		else if ( HasSaveFile( SAVEPATH_TACTICAL, loadSlot ) )
			sceneQueued.sceneID = Game::BATTLE_SCENE;		
	}

	if (    sceneQueued.sceneID == NUM_SCENES 
		 && sceneStack.Empty() ) 
	{
		// Unwind and full reset.
		delete engine;
		engine = new Engine( &screenport, database0 );
		DeleteSaveFile( SAVEPATH_GEO, 0 );
		DeleteSaveFile( SAVEPATH_TACTICAL, 0 );

		PushScene( INTRO_SCENE, 0 );
		PushPopScene();
	}
	else if ( sceneQueued.sceneID != NUM_SCENES ) 
	{
		GLASSERT( sceneQueued.sceneID < NUM_SCENES );

		if ( sceneStack.Size() ) {
			sceneStack.Top()->scene->DeActivate();
		}

		SceneNode* oldTop = 0;
		if ( !sceneStack.Empty() ) {
			oldTop = sceneStack.Top();
		}

		SceneNode* node = sceneStack.Push();
		CreateScene( sceneQueued, node );
		sceneQueued.data = 0;
		sceneQueued.Free();

		node->scene->Activate();
		node->scene->Resize();

		if ( oldTop ) 
			oldTop->scene->ChildActivated( node->sceneID, node->scene, node->data );

		if (    node->scene->CanSave() 
			 && !Engine::mapMakerMode 
			 && sceneStack.Size() == 1 ) 
		{
			SavePathType savePath = node->scene->CanSave();
			FILE* fp = GameSavePath( savePath, SAVEPATH_READ, loadSlot );
			if ( fp ) {
				TiXmlDocument doc;
				doc.LoadFile( fp );
				//GLASSERT( !doc.Error() );
				if ( !doc.Error() ) {
					Load( doc );
				}
				fclose( fp );
			}
			loadSlot = 0;	// queried during the load; don't clear until after load.
		}
	}
}


const Research* Game::GetResearch()
{
	for( SceneNode* node = sceneStack.BeginTop(); node; node = sceneStack.Next() ) {
		if ( node->sceneID == GEO_SCENE ) {
			return &((GeoScene*)node->scene)->GetResearch();
		}
	}
	return 0;
}


void Game::CreateScene( const SceneNode& in, SceneNode* node )
{
	Scene* scene = 0;
	switch ( in.sceneID ) {
		case BATTLE_SCENE:		battleData.Init(); scene = new BattleScene( this );									break;
		case CHARACTER_SCENE:	scene = new CharacterScene( this, (CharacterSceneData*)in.data );					break;
		case INTRO_SCENE:		scene = new TacticalIntroScene( this );												break;
		case END_SCENE:			scene = new TacticalEndScene( this );												break;
		case UNIT_SCORE_SCENE:	scene = new TacticalUnitScoreScene( this );											break;
		case HELP_SCENE:		scene = new HelpScene( this, (const HelpSceneData*)in.data );						break;
		case DIALOG_SCENE:		scene = new DialogScene( this, (const DialogSceneData*)in.data );					break;
		case GEO_SCENE:			scene = new GeoScene( this, (const GeoSceneData*)in.data );														break;
		case GEO_END_SCENE:		scene = new GeoEndScene( this, (const GeoEndSceneData*)in.data );					break;
		case BASETRADE_SCENE:	scene = new BaseTradeScene( this, (BaseTradeSceneData*)in.data );					break;
		case BUILDBASE_SCENE:	scene = new BuildBaseScene( this, (BuildBaseSceneData*)in.data );					break;
		case RESEARCH_SCENE:	scene = new ResearchScene( this, (ResearchSceneData*)in.data );						break;
		case SETTING_SCENE:		scene = new SettingScene( this );													break;
		case SAVE_LOAD_SCENE:	scene = new SaveLoadScene( this, (const SaveLoadSceneData*)in.data );				break;
		case NEW_TAC_OPTIONS:	scene = new NewTacticalOptions( this );												break;
		case NEW_GEO_OPTIONS:	scene = new NewGeoOptions( this );												break;
		default:
			GLASSERT( 0 );
			break;
	}
	node->scene = scene;
	node->sceneID = in.sceneID;
	node->data = in.data;
	node->result = INT_MIN;
}


const gamui::RenderAtom& Game::GetRenderAtom( int id )
{
	GLASSERT( id >= 0 && id < ATOM_COUNT );
	GLASSERT( renderAtoms[id].textureHandle );
	return renderAtoms[id];
}


const gamui::ButtonLook& Game::GetButtonLook( int id )
{
	GLASSERT( id >= 0 && id < LOOK_COUNT );
	return buttonLooks[id];
}


void Game::Load( const TiXmlDocument& doc )
{
	ParticleSystem::Instance()->Clear();

	// Already pushed the BattleScene. Note that the
	// BOTTOM of the stack loads. (BattleScene or GeoScene).
	// A GeoScene will in turn load a BattleScene.
	const TiXmlElement* game = doc.RootElement();
	GLASSERT( StrEqual( game->Value(), "Game" ) );
	const TiXmlElement* scene = game->FirstChildElement();
	sceneStack.Top()->scene->Load( scene );
}


FILE* Game::GameSavePath( SavePathType type, SavePathMode mode, int slot ) const
{	
	grinliz::GLString str( savePath );
	if ( type == SAVEPATH_GEO )
		str += "geogame";
	else if ( type == SAVEPATH_TACTICAL )
		str += "tacgame";
	else
		GLASSERT( 0 );

	if ( slot > 0 ) {
		str += "-";
		str += '0' + slot;
	}
	str += ".xml";

	FILE* fp = fopen( str.c_str(), (mode == SAVEPATH_WRITE) ? "wb" : "rb" );
	return fp;
}


void Game::SavePathTimeStamp( SavePathType type, int slot, GLString* stamp )
{
	*stamp = "";
	FILE* fp = GameSavePath( type, SAVEPATH_READ, slot );
	if ( fp ) {
		char buf[400];
		for( int i=0; i<10; ++i ) {
			fgets( buf, 400, fp );
			buf[399] = 0;
			const char* p = strstr( buf, "timestamp" );
			if ( p ) {
				const char* q0 = strstr( p+1, "\"" );
				if ( q0 ) {
					const char* q1 = strstr( q0+1, "\"" );
					if ( q1 && (q1-q0)>2 ) {
						stamp->append( q0+1, q1-q0-1 );
					}
				}
				break;
			}
		}
		fclose( fp );
	}
}


void Game::Save( int slot, bool saveGeo, bool saveTac )
{
	// For loading, the BOTTOM loads and then loads higher scenes.
	// For saving, the GeoScene saves itself before pushing the tactical
	// scene, so save from the top back. (But still need to save if
	// we are in a character scene for example.)
	for( SceneNode* node=sceneStack.BeginTop(); node; node=sceneStack.Next() ) {
		if (    ( saveGeo && node->scene->CanSave() == SAVEPATH_GEO )
			 || ( saveTac && node->scene->CanSave() == SAVEPATH_TACTICAL ) )
		{
			FILE* fp = GameSavePath( node->scene->CanSave(), SAVEPATH_WRITE, slot );
			GLASSERT( fp );
			if ( fp ) {
				XMLUtil::OpenElement( fp, 0, "Game" );
				XMLUtil::Attribute( fp, "version", VERSION );
				XMLUtil::Attribute( fp, "sceneID", node->sceneID );

				// Somewhat scary c code to get the current time.
				char buf[40];
			    time_t rawtime;
				struct tm * timeinfo;  
				time ( &rawtime );
				timeinfo = localtime ( &rawtime );
				const char* atime = asctime( timeinfo );

				StrNCpy( buf, atime, 40 );
				buf[ strlen(buf)-1 ] = 0;	// remove trailing newline.

				XMLUtil::Attribute( fp, "timestamp", buf );
				XMLUtil::SealElement( fp );

				node->scene->Save( fp, 1 );
	
				XMLUtil::CloseElement( fp, 0, "Game" );

				fclose( fp );
			}
		}
	}
}


void Game::DoTick( U32 _currentTime )
{
	{
		GRINLIZ_PERFTRACK

		currentTime = _currentTime;
		if ( previousTime == 0 ) {
			previousTime = currentTime-1;
		}
		U32 deltaTime = currentTime - previousTime;

		if ( markFrameTime == 0 ) {
			markFrameTime			= currentTime;
			frameCountsSinceMark	= 0;
			framesPerSecond			= 0.0f;
		}
		else {
			++frameCountsSinceMark;
			if ( currentTime - markFrameTime > 500 ) {
				framesPerSecond		= 1000.0f*(float)(frameCountsSinceMark) / ((float)(currentTime - markFrameTime));
				// actually K-tris/second
				markFrameTime		= currentTime;
				frameCountsSinceMark = 0;
			}
		}

		// Limit so we don't ever get big jumps:
		if ( deltaTime > 100 )
			deltaTime = 100;

		GPUShader::ResetState();
		GPUShader::Clear();

		Scene* scene = sceneStack.Top()->scene;
		scene->DoTick( currentTime, deltaTime );

		Rectangle2I clip2D, clip3D;
		int renderPass = scene->RenderPass( &clip3D, &clip2D );
		GLASSERT( renderPass );
	
		if ( renderPass & Scene::RENDER_3D ) {
			GRINLIZ_PERFTRACK_NAME( "Game::DoTick 3D" );
			screenport.SetPerspective( clip3D.Width() > 0 ? &clip3D : 0 );

			engine->Draw();
			if ( mapmaker_showPathing ) {
				engine->GetMap()->DrawPath( mapmaker_showPathing );
			}
			scene->Draw3D();
		
			const grinliz::Vector3F* eyeDir = engine->camera.EyeDir3();
			ParticleSystem* particleSystem = ParticleSystem::Instance();
			particleSystem->Update( deltaTime, currentTime );
			particleSystem->Draw( eyeDir, engine->GetMap() ? &engine->GetMap()->GetFogOfWar() : 0 );
		}

		{
			GRINLIZ_PERFTRACK_NAME( "Game::DoTick UI" );

			// UI Pass
			screenport.SetUI( clip2D.IsValid() ? &clip2D : 0 ); 
			if ( renderPass & Scene::RENDER_3D ) {
				scene->RenderGamui3D();
			}
			if ( renderPass & Scene::RENDER_2D ) {
				screenport.SetUI( clip2D.IsValid() ? &clip2D : 0 );
				scene->DrawHUD();
				scene->RenderGamui2D();
			}
		}
//		SoundManager::Instance()->PlayQueuedSounds();
	}

	const int Y = 305;
	#ifndef GRINLIZ_DEBUG_MEM
	const int memNewCount = 0;
	#endif
#if 1
	if ( !suppressText ) {
		UFOText* ufoText = UFOText::Instance();
		if ( debugLevel >= 1 ) {
			ufoText->Draw(	0,  Y, "#%d %5.1ffps vbo=%d ps=%d", 
							VERSION, 
							framesPerSecond, 
							GPUShader::SupportsVBOs() ? 1 : 0,
							PointParticleShader::IsSupported() ? 1 : 0 );
		}
		if ( debugLevel >= 2 ) {
			ufoText->Draw(	0,  Y-15, "%4.1fK/f %3ddc/f", 
							(float)GPUShader::TrianglesDrawn()/1000.0f,
							GPUShader::DrawCalls() );
		}
		if ( debugLevel >= 3 ) {
			if ( !Engine::mapMakerMode )  {
				ufoText->Draw(  0, Y-30, "new=%d Tex(%d/%d) %dK/%dK mis=%d reuse=%d hit=%d",
								memNewCount,
								TextureManager::Instance()->NumTextures(),
								TextureManager::Instance()->NumGPUResources(),
								TextureManager::Instance()->CalcTextureMem()/1024,
								TextureManager::Instance()->CalcGPUMem()/1024,
								TextureManager::Instance()->CacheMiss(),
								TextureManager::Instance()->CacheReuse(),
								TextureManager::Instance()->CacheHit() );		
			}
		}
	}
#endif
	GPUShader::ResetTriCount();

#ifdef EL_SHOW_MODELS
	int k=0;
	while ( k < nModelResource ) {
		int total = 0;
		for( unsigned i=0; i<modelResource[k].nGroups; ++i ) {
			total += modelResource[k].atom[i].trisRendered;
		}
		UFODrawText( 0, 12+12*k, "%16s %5d K", modelResource[k].name, total );
		++k;
	}
#endif

#ifdef GRINLIZ_PROFILE
	const int SAMPLE = 8;
	if ( (currentFrame & (SAMPLE-1)) == 0 ) {
		Performance::SampleData();
	}
	for( int i=0; i<Performance::NumData(); ++i ) {
		const PerformanceData& data = Performance::GetData( i );

		UFOText::Draw( 60,  20+i*12, "%s", data.name );
		UFOText::Draw( 300, 20+i*12, "%.3f", data.normalTime );
		UFOText::Draw( 380, 20+i*12, "%d", data.functionCalls/SAMPLE );
	}
#endif

	previousTime = currentTime;
	++currentFrame;

	PushPopScene();
}


void Game::LoadModDatabase( const char* name, bool preload )
{
	database0->AttachChain( 0 );
	delete database1;
	database1 = 0;

	if ( name && *name ) {
		database1 = new gamedb::Reader();

		if  ( !database1->Init( 1, name, 0 ) ) {
			delete database1;
			database1 = 0;
		}
	}
	database0->AttachChain( database1 );
	if ( !preload ) {
		TextureManager::Instance()->Reload();
		this->LoadPalettes();
		
		// Full wipe of text and text cache.
		UFOText::Destroy();
		UFOText::Create( database0, TextureManager::Instance()->GetTexture( "font" ), engine->GetScreenportMutable() );
	}
}


void Game::AddDatabase( const char* path )
{
	if ( path ) {
		for( int i=0; i<GAME_MAX_MOD_DATABASES; ++i ) {
			if ( modDatabase[i].size() == 0 ) {
				modDatabase[i] = path;
				GLOUTPUT(( "ModDatabase: id=%d name=%s\n", i, modDatabase[i].c_str() ));
				break;
			}
		}
	}
}


bool Game::PopSound( int* database, int* offset, int* size )
{
	return SoundManager::Instance()->PopSound( database, offset, size );
}


void Game::Tap( int action, int wx, int wy )
{
	// The tap is in window coordinate - need to convert to view.
	Vector2F window = { (float)wx, (float)wy };
	Vector2F view;
	screenport.WindowToView( window, &view );

	grinliz::Ray world;
	screenport.ViewToWorld( view, 0, &world );

#if 0
	{
		Vector2F ui;
		screenport.ViewToUI( view, &ui );
		if ( action != GAME_TAP_MOVE )
			GLOUTPUT(( "Tap: action=%d window(%.1f,%.1f) view(%.1f,%.1f) ui(%.1f,%.1f)\n", action, window.x, window.y, view.x, view.y, ui.x, ui.y ));
	}
#endif
	sceneStack.Top()->scene->Tap( action, view, world );
}


void Game::MouseMove( int x, int y )
{
	//GLASSERT( Engine::mapMakerMode );
	if ( sceneStack.Top()->sceneID == BATTLE_SCENE ) {
		((BattleScene*)sceneStack.Top()->scene)->MouseMove( x, y );
	}
}



void Game::Zoom( int style, float distance )
{
	sceneStack.Top()->scene->Zoom( style, distance );
}


void Game::Rotate( float degrees )
{
	sceneStack.Top()->scene->Rotate( degrees );
}


void Game::CancelInput()
{
	isDragging = false;
}


void Game::HandleHotKeyMask( int mask )
{
	sceneStack.Top()->scene->HandleHotKeyMask( mask );
	if ( mask & GAME_HK_TOGGLE_DEBUG_TEXT ) {
		SetDebugLevel( GetDebugLevel() + 1 );
	}
}


void Game::DeviceLoss()
{
	TextureManager::Instance()->DeviceLoss();
	ModelResourceManager::Instance()->DeviceLoss();
	GPUShader::ResetState();
}


void Game::Resize( int width, int height, int rotation ) 
{
	screenport.Resize( width, height, rotation );
	sceneStack.Top()->scene->Resize();
}


void Game::RotateSelection( int delta )
{
	((BattleScene*)sceneStack.Top()->scene)->RotateSelection( delta );
}

void Game::DeleteAtSelection()
{
	((BattleScene*)sceneStack.Top()->scene)->DeleteAtSelection();
}


void Game::DeltaCurrentMapItem( int d )
{
	((BattleScene*)sceneStack.Top()->scene)->DeltaCurrentMapItem(d);
}


void Game::SetLightMap( float r, float g, float b )
{
	((BattleScene*)sceneStack.Top()->scene)->SetLightMap( r, g, b );
}


RenderAtom Game::CalcDecoAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id <= 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "iconDeco" );
	float tx0 = 0;
	float ty0 = 0;
	float tx1 = 0;
	float ty1 = 0;

	if ( id != DECO_NONE ) {
		int y = id / 8;
		int x = id - y*8;
		tx0 = (float)x / 8.f;
		ty0 = (float)y / 4.f;
		tx1 = tx0 + 1.f/8.f;
		ty1 = ty0 + 1.f/4.f;
	}
	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_DECO : UIRenderer::RENDERSTATE_UI_DECO_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}


RenderAtom Game::CalcParticleAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 16 );
	Texture* texture = TextureManager::Instance()->GetTexture( "particleQuad" );
	int y = id / 4;
	int x = id - y*4;
	float tx0 = (float)x / 4.f;
	float ty0 = (float)y / 4.f;
	float tx1 = tx0 + 1.f/4.f;
	float ty1 = ty0 + 1.f/4.f;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}


RenderAtom Game::CalcIconAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "icons" );
	int y = id / 8;
	int x = id - y*8;
	float tx0 = (float)x / 8.f;
	float ty0 = (float)y / 4.f;
	float tx1 = tx0 + 1.f/8.f;
	float ty1 = ty0 + 1.f/4.f;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}


RenderAtom Game::CalcIcon2Atom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "icons2" );

	static const int CX = 4;
	static const int CY = 4;

	int y = id / CX;
	int x = id - y*CX;
	float tx0 = (float)x / (float)CX;
	float ty0 = (float)y / (float)CY;;
	float tx1 = tx0 + 1.f/(float)CX;
	float ty1 = ty0 + 1.f/(float)CY;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}


RenderAtom Game::CalcPaletteAtom( int c0, int c1, int blend, bool enabled )
{
	Vector2I c = { 0, 0 };
	Texture* texture = TextureManager::Instance()->GetTexture( "palette" );

	static const int offset[5] = { 75, 125, 175, 225, 275 };
	static const int subOffset[3] = { 0, -12, 12 };

	if ( blend == PALETTE_NORM ) {
		if ( c1 > c0 )
			Swap( &c1, &c0 );
		c.x = offset[c0];
		c.y = offset[c1];
	}
	else {
		if ( c0 > c1 )
			Swap( &c0, &c1 );

		if ( c0 == c1 ) {
			// first column special:
			c.x = 25 + subOffset[blend];
			c.y = offset[c1];
		}
		else {
			c.x = offset[c0] + subOffset[blend];;
			c.y = offset[c1];
		}
	}
	RenderAtom atom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, 0, 0, 0, 0 );
	UIRenderer::SetAtomCoordFromPixel( c.x, c.y, c.x, c.y, 300, 300, &atom );
	return atom;
}
