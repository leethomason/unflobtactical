#include "geoscene.h"
#include "game.h"
#include "cgame.h"
#include "areaWidget.h"
#include "geomap.h"
#include "geoai.h"
#include "geoendscene.h"
#include "chits.h"

#include "../engine/loosequadtree.h"
#include "../engine/particle.h"
#include "../engine/particleeffect.h"
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
	"w. 0c 0d 0a 0c w. w. 2c 2a 2c 2a 2d 3d 2a 3a 3a 3a 3a w. w. "
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


GeoScene::GeoScene( Game* _game ) : Scene( _game )
{
	missileTimer[0] = 0;
	missileTimer[1] = 0;

	const Screenport& port = GetEngine()->GetScreenport();
	random.SetSeedFromTime();

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

	/*
	RenderAtom upE = UIRenderer::CalcIconAtom( ICON_GREEN_STAND_MARK_OUTLINE, true );
	RenderAtom upD = UIRenderer::CalcIconAtom( ICON_GREEN_STAND_MARK_OUTLINE, false );
	RenderAtom downE = UIRenderer::CalcIconAtom( ICON_YELLOW_STAND_MARK_OUTLINE, true );
	RenderAtom downD = UIRenderer::CalcIconAtom( ICON_YELLOW_STAND_MARK_OUTLINE, false );
	RenderAtom nullAtom;
	*/	
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
		if ( baseButton.Down() ) {
			PlaceBase( mapi );
		}
	}
}


void GeoScene::PlaceBase( const grinliz::Vector2I& map )
{
	if (    geoMapData.GetType( map.x, map.y ) 
		 && !geoMapData.IsCity( map.x, map.y )
		 && CalcBaseChits(0) < MAX_BASES ) 
	{
		bool parked = false;
		for( int i=0; i<chitArr.Size(); ++i ) {
			if (    chitArr[i]->IsBaseChit() 
					&& chitArr[i]->MapPos() == map ) 
			{
				parked = true;
				break;
			}
		}
		if ( !parked ) {
			BaseChit* bases[MAX_BASES] = {0};
			int count = CalcBaseChits( bases );

			int id=0, b=0;
			for( id=0; id<MAX_BASES; id++ ) {
				for( b=0; b<count; ++b ) {
					if ( bases[b]->ID() == id )
						break;
				}
				if ( b == count ) {
					break;	// free slot!
				}
			}
			GLASSERT( id < MAX_BASES );

			Chit** chit = chitArr.Push();
			*chit = new BaseChit( tree, map, id );

			baseButton.SetUp();
			int region = geoMapData.GetRegion( map.x, map.y );
			regionData[region].AddBase( map );
		}
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


void GeoScene::FireBaseWeapons()
{
	float best2 = FLT_MAX;
	const UFOChit* bestUFO = 0;
	
	BaseChit* bases[MAX_BASES];
	CalcBaseChits( bases );

	for( int type=0; type<2; ++type ) {
		while ( missileTimer[type] > MissileFreq( type ) ) {
			missileTimer[type] -= MissileFreq( type );

			for( int i=0; i<MAX_BASES && bases[i]; ++i ) {
				BaseChit* base = bases[i];
				float range2 = MissileRange( type ) * MissileRange( type );

				for( int k=0; k<chitArr.Size(); ++k ) {
					UFOChit* ufo = chitArr[k]->IsUFOChit();
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
					// (The scrolling world is a pain.) FIXME: account for the seam.

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
			for( int k=0; k<chitArr.Size(); ++k ) {
				UFOChit* ufo = chitArr[k]->IsUFOChit();
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


BaseChit* GeoScene::IsBaseLocation( int x, int y )
{
	Vector2I pos = { x, y };
	BaseChit* bases[MAX_BASES];
	CalcBaseChits( bases );

	for( int i=0; i<MAX_BASES; ++i ) {
		if ( bases[i] && bases[i]->MapPos() == pos )
			return bases[i];
	}
	return 0;
}


int GeoScene::CalcBaseChits( BaseChit** array )
{
	if ( array ) {
		for( int i=0; i<MAX_BASES; ++i )
			array[i] = 0;
	}

	int count=0;
	for( int i=0; i<chitArr.Size(); ++i ) {
		if ( chitArr[i]->IsBaseChit() )
		{
			GLASSERT( count < MAX_BASES );
			if ( array ) {
				array[count] = chitArr[i]->IsBaseChit();
			}
			count++;
		}
	}
	return count;
}



void GeoScene::DoTick( U32 currentTime, U32 deltaTime )
{
	if ( lastAlienTime==0 || (currentTime > lastAlienTime+timeBetweenAliens) ) {
		Chit** test = chitArr.Push();

		Vector2F start, dest;
		int type = UFOChit::SCOUT+random.Rand( 3 );
		geoAI->GenerateAlienShip( type, &start, &dest, regionData );
		*test = new UFOChit( tree, type, start, dest );

		lastAlienTime = currentTime;
	}
	missileTimer[0] += deltaTime;
	missileTimer[1] += deltaTime;
	FireBaseWeapons();

	geoMap->DoTick( currentTime, deltaTime );

	UpdateMissiles( deltaTime );

	for( int i=0; i<chitArr.Size(); ++i ) {
		
		Chit* chit = chitArr[i];
		int message = chit->DoTick( deltaTime );
		
		Vector2I pos = chit->MapPos();

		switch ( message ) {

		case Chit::MSG_NONE:
			break;

		case Chit::MSG_DONE:
			chit->SetDestroyed();
			break;

		case Chit::MSG_UFO_AT_DESTINATION:
			{

				// Battleship, at a capital, it can occupy.
				// Battleship, at base, can attack it
				// Battleship or Destroyer, at city, can city attack
				// Any ship, not at city, can crop circle

				UFOChit* ufoChit = chit->IsUFOChit();
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
					ufoChit->SetAI( UFOChit::AI_ORBIT );
				}
				else if (    ufoChit->Type() == UFOChit::BATTLESHIP 
							&& IsBaseLocation( pos.x, pos.y )
							&& regionData[region].influence >= MIN_BASE_ATTACK_INFLUENCE )
				{
					ufoChit->SetAI( UFOChit::AI_BASE_ATTACK );
				}
				else if (    ufoChit->Type() == UFOChit::BATTLESHIP 
						    && !regionData[region].occupied
						    && geoMapData.Capital( region ) == pos 
							&& regionData[region].influence >= MIN_OCCUPATION_INFLUENCE )
				{
					regionData[region].influence = MAX_INFLUENCE;
					regionData[region].occupied = true;
					areaWidget[region]->SetInfluence( (float)regionData[region].influence );
					ufoChit->SetAI( UFOChit::AI_OCCUPATION );
				}
				else if (    ( ufoChit->Type() == UFOChit::BATTLESHIP || ufoChit->Type() == UFOChit::FRIGATE )
						    && !regionData[region].occupied
						    && geoMapData.IsCity( pos.x, pos.y )
							&& regionData[region].influence >= MIN_CITY_ATTACK_INFLUENCE ) 
				{
					ufoChit->SetAI( UFOChit::AI_CITY_ATTACK );
				}
				else if ( !geoMapData.IsCity( pos.x, pos.y ) ) {
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
				Chit** chit = chitArr.Push();
				*chit = new CropCircle( tree, pos, random.Rand() );

				int region = geoMapData.GetRegion( pos.x, pos.y );
				if ( regionData[region].influence < MAX_CROP_CIRCLE_INFLUENCE ) {
					regionData[region].influence += CROP_CIRCLE_INFLUENCE;
					regionData[region].influence = Min( regionData[region].influence, MAX_INFLUENCE );
					areaWidget[region]->SetInfluence( (float)regionData[region].influence );
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
				for( int k=0; k<chitArr.Size(); ++k ) {
					if ( k!=i 
							&& chitArr[k]->MapPos() == pos 
							&& ( chitArr[k]->Parked() || chitArr[k]->IsBaseChit() ) )
					{
						parked = true;
						break;
					}
				}
				// check for water, cities
				int mapType = geoMapData.GetType( pos.x, pos.y );

				if ( mapType == 0 || mapType == GeoMapData::CITY ) {
					parked = true;
				}

				if ( !parked ) {
					chit->IsUFOChit()->SetAI( UFOChit::AI_CRASHED );
					chit->SetMapPos( pos.x, pos.y );
				}
				else {
					chit->SetDestroyed();
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
				regionData[region].influence += CITY_ATTACK_INFLUENCE;
				regionData[region].influence = Min( regionData[region].influence, MAX_INFLUENCE );
				areaWidget[region]->SetInfluence( (float)regionData[region].influence );

				if ( !geoMapData.IsWater( pos.x, pos.y ) ) {
					int region = geoMapData.GetRegion( pos.x, pos.y );
					regionData[region].Success();
				}
			}
			break;

			case Chit::MSG_BASE_ATTACK_COMPLETE:
			{
				BaseChit* base = IsBaseLocation( pos.x, pos.y );
				base->SetDestroyed();	// can't delete in this loop
			}
			break;

		default:
			GLASSERT( 0 );
		}
	}

	// Check for deferred chit destruction.
	for( int i=0; i<chitArr.Size(); ) {
		if ( chitArr[i]->IsDestroyed() ) {
			if ( chitArr[i]->IsBaseChit() ) {
				static const float INV = 1.f/255.f;
				static const Color4F particleColor = { 59.f*INV, 98.f*INV, 209.f*INV, 1.0f };
				static const Color4F colorVec	= { 0.0f, -0.1f, -0.1f, -0.3f };
				static const Vector3F particleVel = { 2.0f, 0, 0 };

				for( int k=0; k<2; ++k ) {
					Vector3F pos = { chitArr[i]->Pos().x + (float)(GEO_MAP_X*k), 1.0f, chitArr[i]->Pos().y };

					ParticleSystem::Instance()->EmitPoint( 
						80, ParticleSystem::PARTICLE_HEMISPHERE, 
						particleColor, colorVec,
						pos, 
						0.2f,
						particleVel, 0.1f );
				}
				Vector2I mapi = chitArr[i]->MapPos();
				int region = geoMapData.GetRegion( mapi.x, mapi.y );
				regionData[region].RemoveBase( mapi );
			}
			delete chitArr[i];
			chitArr.SwapRemove( i );
		}
		else {
			++i;
		}
	}
	baseButton.SetEnabled( CalcBaseChits(0) < MAX_BASES );

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



