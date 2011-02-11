#include "geoscene.h"
#include "game.h"
#include "cgame.h"
#include "areaWidget.h"
#include "geomap.h"
#include "geoai.h"
#include "geoendscene.h"

#include "../engine/loosequadtree.h"
#include "../engine/particle.h"
#include "../engine/ufoutil.h"

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
	"w. 0c 0d 0a 0c w. w. 2c w. 2c 2a 2d 3d 2a 3a 3a 3a 3a w. w. "
	"w. 0c 0d 0a w. w. w. 4c 4d 4d 4d 3d 3d 3a 3a 3a 3a 3c w. w. "
	"w. w. 0c w. 0f w. w. 4f 4f 4d 4c 3d 3c w. 3f w. 3c w. w. w. "
	"w. w. w. 0f 1c w. w. 4f 4f 4f 4d 4f w. w. 3c w. w. w. w. w. "
	"w. w. w. w. 1f 1f 1f w. w. 4f 4f w. w. w. w. w. 3f 3f w. w. "
	"w. w. w. w. 1f 1a 1c w. w. 4f 4f w. w. w. w. w. w. w. 5c w. "
	"w. w. w. w. w. 1f 1f w. w. 4f 4f 4f w. w. w. w. w. 5f 5d 5a "
	"w. w. w. w. w. 1f w. w. w. 4c w. w. w. w. w. w. w. 5f 5d 5c "
;


void GeoMapData::Init( const char* map, Random* random )
{
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
			int r = Region( data[j*GEO_MAP_X+i] );

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


GeoScene::GeoScene( Game* _game ) : Scene( _game )
{
	const Screenport& port = GetEngine()->GetScreenport();

	geoMapData.Init( MAP, &random );
	for( int i=0; i<GEO_REGIONS; ++i ) {
		regionData[i].Init();
	}

	geoMap = new GeoMap( GetEngine()->GetSpaceTree() );
	tree = GetEngine()->GetSpaceTree();
	geoAI = new GeoAI( geoMapData );

	lastAlienTime = 0;
	timeBetweenAliens = 5*1000;

	static const char* names[GEO_REGIONS] = { "North", "South", "Europe", "Asia", "Africa", "Under" };
	for( int i=0; i<GEO_REGIONS; ++i ) {
		areaWidget[i] = new AreaWidget( _game, &gamui2D, names[i] );
	}
}


GeoScene::~GeoScene()
{
	for( int i=0; i<GEO_REGIONS; ++i ) {
		delete areaWidget[i];
	}
	delete geoMap;
	delete geoAI;
}


void GeoScene::Activate()
{
	GetEngine()->CameraIso( false, false, (float)GeoMap::MAP_X, (float)GeoMap::MAP_Y );
	SetMapLocation();
	GetEngine()->SetIMap( geoMap );
	SetMapLocation();

	for( int j=0; j<GEO_REGIONS; ++j ) {
		for( int i=0, n=geoMapData.NumCities( j ); i<n; ++i ) {
			Vector2I pos = geoMapData.City( j, i );
			Chit** chit= chitArr.Push();
			*chit = new CityChit( GetEngine()->GetSpaceTree(), pos, (i==geoMapData.CapitalID(j) ) );
		}
	}
}


void GeoScene::DeActivate()
{
	for( int i=0; i<chitArr.Size(); ++i ) {
		delete chitArr[i];
	}
	chitArr.Clear();

	GetEngine()->CameraIso( true, false, 0, 0 );
	GetEngine()->SetIMap( 0 );
}


void GeoScene::Tap(	int action, 
					const grinliz::Vector2F& view,
					const grinliz::Ray& world )
{
	Vector2F ui;
	const Screenport& port = GetEngine()->GetScreenport();
	port.ViewToUI( view, &ui );

	const Vector3F& cameraPos = GetEngine()->camera.PosWC();
	float d = cameraPos.y * tanf( ToRadian( EL_FOV*0.5f ) );
	float widthInMap = d*2.0f;

	if ( action == GAME_TAP_DOWN || action == GAME_TAP_DOWN_PANNING ) {
		if ( action == GAME_TAP_DOWN ) {
			// First check buttons.
			gamui2D.TapDown( ui.x, ui.y );
		}

		dragStart.Set( -1, -1 );

		if ( !gamui2D.TapCaptured() ) {
			dragStart = ui;
		}
	}
	else if ( action == GAME_TAP_MOVE || action == GAME_TAP_MOVE_PANNING ) {
		if ( dragStart.x >= 0 ) {
			//mapOffset += ui.x - dragStart.x;
			GetEngine()->camera.DeltaPosWC( -(ui.x - dragStart.x)*widthInMap/port.UIWidth(), 0, 0 );
			dragStart = ui;
			SetMapLocation();
		}
	}
	else if ( action == GAME_TAP_UP || action == GAME_TAP_UP_PANNING ) {
		if ( dragStart.x >= 0 ) {
			//mapOffset += ui.x - dragStart.x;
			GetEngine()->camera.DeltaPosWC( (ui.x - dragStart.x)*widthInMap/port.UIWidth(), 0, 0 );
			dragStart = ui;
			SetMapLocation();
		}
		
		if ( gamui2D.TapCaptured() ) {
			//HandleIconTap( item );
		}
	}
	else if ( action == GAME_TAP_CANCEL || action == GAME_TAP_CANCEL_PANNING ) {
		gamui2D.TapUp( ui.x, ui.y );
		dragStart.Set( -1, -1 );
	}

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
}


void GeoScene::DoTick( U32 currentTime, U32 deltaTime )
{
	if ( lastAlienTime==0 || (currentTime > lastAlienTime+timeBetweenAliens) ) {
		Chit** test = chitArr.Push();

		Vector2F start, dest;
		int type = TravellingUFO::SCOUT+random.Rand( 3 );
		geoAI->GenerateAlienShip( type, &start, &dest, regionData );
		*test = new TravellingUFO( tree, type, start, dest );

		lastAlienTime = currentTime;
	}

	geoMap->DoTick( currentTime, deltaTime );

	for( int i=0; i<chitArr.Size(); ) {
		message.Clear();
		chitArr[i]->DoTick( deltaTime, &message );
		Vector2I pos = chitArr[i]->MapPos();

		bool done = false;
		for( int j=0; j<message.Size(); ++j ) {
			switch ( message[j] ) {
			case Chit::MSG_DONE:
				done = true;
				break;

			case Chit::MSG_UFO_AT_DESTINATION:
				{

					// Battleship, at a capital, it can occupy.
					// Battleship or Destroyer, at city, can city attack
					// Any ship, not at city, can crop circle

					TravellingUFO* ufoChit = (TravellingUFO*)chitArr[i];	// back casting always scary, but we know this is a UFO
					int region = geoMapData.GetRegion( pos.x, pos.y );
					int type = geoMapData.GetType( pos.x, pos.y );

					bool returnToOrbit = false;

					// If the destination space is taken, return to orbit.
					for( int k=0; k<chitArr.Size(); ++k ) {
						if ( i != k ) {
							if ( chitArr[k]->Parked() && (chitArr[k]->MapPos()==pos ) ) {
								returnToOrbit = true;
								break;
							}
						}
					}

					if ( returnToOrbit ) {
						ufoChit->SetAI( TravellingUFO::ORBIT );
					}
					else if (    ufoChit->Type() == TravellingUFO::BATTLESHIP 
						      && !regionData[region].occupied
						      && geoMapData.Capital( region ) == pos 
							  && regionData[region].influence >= MIN_OCCUPATION_INFLUENCE )
					{
						regionData[region].influence = 10;
						regionData[region].occupied = true;
						areaWidget[region]->SetInfluence( (float)regionData[region].influence );
						ufoChit->SetAI( TravellingUFO::OCCUPATION );
					}
					else if (    ( ufoChit->Type() == TravellingUFO::BATTLESHIP || ufoChit->Type() == TravellingUFO::FRIGATE )
						      && !regionData[region].occupied
						      && geoMapData.IsCity( pos.x, pos.y )
							  && regionData[region].influence >= MIN_CITY_ATTACK_INFLUENCE ) 
					{
						ufoChit->SetAI( TravellingUFO::CITY_ATTACK );
					}
					else if ( !geoMapData.IsCity( pos.x, pos.y ) ) {
						ufoChit->SetAI( TravellingUFO::CROP_CIRCLE );
					}
					else {
						// Er, total failure. Collision of AI goals.
						ufoChit->SetAI( TravellingUFO::ORBIT );
					}
				}
				break;

				case Chit::MSG_CROP_CIRCLE_COMPLETE:
				{
					Chit** chit = chitArr.Push();
					*chit = new CropCircle( tree, pos, random.Rand() );

					int region = geoMapData.GetRegion( pos.x, pos.y );
					if ( regionData[region].influence < MAX_CROP_CIRCLE_INFLUENCE ) {
						regionData[region].influence += CROP_CIRCLE_INFLUENCE;
						areaWidget[region]->SetInfluence( (float)regionData[region].influence );
					}
				}
				break;

				case Chit::MSG_CITY_ATTACK_COMPLETE:
				{
					int region = geoMapData.GetRegion( pos.x, pos.y );
					regionData[region].influence += CITY_ATTACK_INFLUENCE;
					if ( regionData[region].influence > 10 ) regionData[region].influence = 10;
					areaWidget[region]->SetInfluence( (float)regionData[region].influence );
				}
				break;

			default:
				GLASSERT( 0 );
			}
		}
		
		if ( !done ) {
			++i;
		}
		else {
			delete chitArr[i];
			chitArr.SwapRemove( i );
		}
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



static const float UFO_HEIGHT = 0.5f;
static const float UFO_SPEED[TravellingUFO::NUM_TYPES] = { 1.0f, 0.8f, 0.5f };
static const float UFO_ACCEL = 0.2f;	// units/second2

static const float UFO_LAND_TIME = 10*1000;
static const float UFO_CRASH_TIME = 20*1000;

Chit::Chit( SpaceTree* _tree ) 
{
	tree = _tree;
	model[0] = 0;
	model[1] = 0;
}

Chit::~Chit()
{
	for( int i=0; i<2; ++i ) {
		tree->FreeModel( model[i] );
	}
}


TravellingUFO::TravellingUFO( SpaceTree* tree, int type, const grinliz::Vector2F& start, const grinliz::Vector2F& dest ) : Chit( tree )
{
	this->type = type;
	this->ai = TRAVELLING;
	this->pos = start;
	this->dest = dest;
	this->velocity = UFO_SPEED[type]*2.0f;
	this->effectTimer = 0;
	this->jobTimer = 0;
	decal[0] = 0;
	decal[1] = 0;

	static const char* name[3] = { "geo_scout", "geo_frigate", "geo_battleship" };
	GLASSERT( type >= 0 && type < 3 );

	model[0] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( name[type] ) );
	model[1] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( name[type] ) );
}


TravellingUFO::~TravellingUFO()
{
	for( int i=0; i<2; ++i ) {
		if ( decal[i] )
			tree->FreeModel( decal[i] );
	}
}


void TravellingUFO::EmitEntryExitBurn( U32 deltaTime, const Vector3F& p0, const Vector3F& p1, bool entry )
{
	static const U32 INTERVAL = 300;
	effectTimer += deltaTime;
	const Vector3F pos[2] = { p0, p1 };

	if ( effectTimer >= INTERVAL ) {
		effectTimer -= INTERVAL;

		ParticleSystem* ps = ParticleSystem::Instance();

		static const Color4F smokeColor		= { 0.5f, 0.5f, 0.5f, 1.0f };
		static const Color4F smokeColorVec	= { -0.1f, -0.1f, -0.1f, -0.40f };
		static const Vector3F vel = { 0, 0, 0 };

		//static const Color4F particleColor = { 1.0f, 0.3f, 0.1f, 1.0f };	// red
		static const float INV = 1.f/255.f;
		static const Color4F particleColor = { 66.f*INV, 204.f*INV, 3.f*INV, 1.0f };
		static const Color4F colorVec	= { 0.0f, -0.1f, -0.1f, -0.5f };
		static const Vector3F particleVel = { 0.4f, 0, 0 };

		float halfSize = model[0]->AABB().SizeX()* 0.5f;
		static const float GROW = 0.2f;

		for( int i=0; i<2; ++i ) {
			if ( entry ) {
				ps->EmitQuad( ParticleSystem::SMOKE,
							  smokeColor,
							  smokeColorVec,
							  pos[i],
							  0.0,
							  vel, 0.1f,
							  halfSize, GROW );
			}

			ps->EmitPoint( 10, ParticleSystem::PARTICLE_SPHERE, 
						   particleColor, colorVec,
						   pos[i],
						   0.4f,
						   particleVel, 0.2f );
		}
	}
}



void TravellingUFO::Decal( U32 timer, float c, float d )
{
	if ( !decal[0] ) {
		for( int i=0; i<2; ++i ) {
			decal[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( "unitplate" ) );
			decal[i]->SetTexture( TextureManager::Instance()->GetTexture( "atlas2" ) );
			decal[i]->SetTexXForm( 0, 1, 0.25f, c, d );
			
			Vector3F pos = model[i]->Pos();
			//pos.y = 0.1f;
			decal[i]->SetPos( pos );
		}
	}
	decal[0]->SetRotation( (float)timer * 0.2f );
	decal[1]->SetRotation( (float)timer * 0.2f );
}


void TravellingUFO::RemoveDecal()
{
	for( int i=0; i<2; ++i ) {
		tree->FreeModel( decal[i] );
		decal[i] = 0;
	}
}


void TravellingUFO::DoTick( U32 deltaTime, ::CDynArray<int>* message )
{
	float timeFraction = (float)deltaTime / 1000.0f;
	float travel = velocity*timeFraction;
	bool done = false;

	if ( ai == TRAVELLING )
	{
		Vector2F normal = dest - pos;
		if ( normal.Length() >= travel ) {
			normal.Normalize();
			normal = normal * travel;
			pos += normal;

			// reduce speed:
			if ( velocity > UFO_SPEED[type] ) {
				velocity -= UFO_ACCEL*timeFraction;
				EmitEntryExitBurn( deltaTime, model[0]->Pos(), model[1]->Pos(), true );
			}
			if ( velocity < UFO_SPEED[type] ) {
				velocity = UFO_SPEED[type];
			}
		}
		else {
			pos = dest;
			ai = AI_NONE;
			int* msg = message->Push();
			*msg = MSG_UFO_AT_DESTINATION;
		}
	}
	else if ( ai == ORBIT ) {
		Vector2F normal = { travel, 0 };
		pos += normal;

		velocity += UFO_ACCEL*timeFraction;
		EmitEntryExitBurn( deltaTime, model[0]->Pos(), model[1]->Pos(), false );
		if ( velocity >= UFO_SPEED[type] * 2.0f ) {

			static const float INV = 1.f/255.f;
			static const Color4F particleColor = { 66.f*INV, 204.f*INV, 3.f*INV, 1.0f };
			static const Color4F colorVec	= { 0.0f, -0.1f, -0.1f, -0.5f };
			static const Vector3F particleVel = { 0.4f, 0, 0 };

			for( int i=0; i<2; ++i ) {
				ParticleSystem::Instance()->EmitPoint( 
								40, ParticleSystem::PARTICLE_SPHERE, 
								particleColor, colorVec,
								model[i]->Pos(),
								0.2f,
								particleVel, 0.1f );
			}
			int* msg = message->Push();
			*msg = MSG_DONE;
		}
	}
	else if ( ai == CRASHED ) {
		jobTimer += deltaTime;
		for( int i=0; i<2; ++i ) {
			ParticleSystem::Instance()->EmitSmoke( deltaTime, model[i]->Pos() );
		}
		if ( jobTimer >= UFO_CRASH_TIME ) {
			int* msg = message->Push();
			*msg = MSG_DONE;
		}
	}
	else if ( ai == CITY_ATTACK ) {
		jobTimer += deltaTime;		

		if ( jobTimer >= UFO_LAND_TIME ) {
			int* msg = message->Push();
			*msg = MSG_CITY_ATTACK_COMPLETE;
			ai = ORBIT;
			RemoveDecal();
		}
		else {
			Decal( jobTimer, 0, 0.25f );
		}
	}
	else if ( ai == CROP_CIRCLE ) {
		jobTimer += deltaTime;
		if ( jobTimer >= UFO_LAND_TIME ) {
			int* msg = message->Push();
			*msg = MSG_CROP_CIRCLE_COMPLETE;
			ai = ORBIT;
			RemoveDecal();
		}
		else {
			Decal( jobTimer, 0, 0.0f );
		}
	}
	else if ( ai == OCCUPATION ) {
		// do nothing. Can only be disloged by an soldier attack
		Decal( jobTimer, 0, 0.50f );
	}
	else {
		GLASSERT( 0 );
	}

	Vector3F pos0 = { pos.x, UFO_HEIGHT, pos.y };
	Vector3F pos1 = { pos.x, UFO_HEIGHT, pos.y };

	while ( pos0.x < 0 )
		pos0.x += (float)GEO_MAP_X;
	while ( pos0.x > (float)GEO_MAP_X )
		pos0.x -= (float)GEO_MAP_X;


	while ( pos1.x < (float)GEO_MAP_X )
		pos1.x += (float)GEO_MAP_X;
	while ( pos1.x > (float)(2*GEO_MAP_X) )
		pos1.x -= (float)GEO_MAP_X;

	model[0]->SetPos( pos0 );
	model[1]->SetPos( pos1 );
}


CropCircle::CropCircle( SpaceTree* tree, const Vector2I& mapPos, U32 seed ) : Chit( tree )
{
	Texture* texture = TextureManager::Instance()->GetTexture( "cropcircle" );
	Random r( seed );
	float dx = (float)(r.Rand(CROP_CIRCLES_X)) / (float)CROP_CIRCLES_X;
	float dy = (float)(r.Rand(CROP_CIRCLES_Y)) / (float)CROP_CIRCLES_Y;

	Vector3F pos = { (float)mapPos.x+0.5f, UFO_HEIGHT*0.1f, (float)mapPos.y+0.5f };

	for( int i=0; i<2; ++i ) {
		model[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( "unitplate" ) );
		model[i]->SetTexture( texture );
		model[i]->SetTexXForm( 0, 1.f/(float)CROP_CIRCLES_X, 1.f/(float)CROP_CIRCLES_Y, dx, dy );
	}
	model[0]->SetPos( pos );
	pos.x += (float)GEO_MAP_X;
	model[1]->SetPos( pos );
	jobTimer = 0;
}


CropCircle::~CropCircle()
{
	// nothing. model released by parent.
}


void CropCircle::DoTick( U32 deltaTime, ::CDynArray<int>* message )
{
	jobTimer += deltaTime;
	if ( jobTimer > CROP_CIRCLE_TIME_SECONDS*1000 ) {
		int* msg = message->Push();
		*msg = MSG_DONE;
	}
}


CityChit::CityChit( SpaceTree* tree, const grinliz::Vector2I& posi, bool _capital ) : Chit( tree ), capital( _capital )
{
	Vector3F pos = { (float)posi.x+0.5f, 0.0f, (float)posi.y+0.5f };
	for( int i=0; i<2; ++i ) {
		model[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( capital ? "capital" : "city" ) );
	}
	model[0]->SetPos( pos );
	pos.x += (float)GEO_MAP_X;
	model[1]->SetPos( pos );
}

