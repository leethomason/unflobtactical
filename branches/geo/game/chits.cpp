#include "chits.h"
#include "geoscene.h"
#include "item.h"

#include "../engine/loosequadtree.h"
#include "../engine/particle.h"
#include "../engine/uirendering.h"

#include "tacticalintroscene.h"		// fixme: only for unit generation

using namespace grinliz;


void Chit::SetMapPos( int x, int y ) 
{ 
	GLASSERT( x > -GEO_MAP_X && x < GEO_MAP_X*3 );
	pos.Set( (float)x+0.5f, (float)y+0.5f );
	pos = Cylinder<float>::Normalize( pos );
}


void Chit::SetPos( float x, float y ) { 
	GLASSERT( x > -GEO_MAP_X && x < GEO_MAP_X*3 );
	pos.Set( x, y ); 
}


float BaseChit::MissileRange( const int type ) const
{ 
	return ( type == 0 ) ? (float)GUN_LIFETIME * GUN_SPEED / 1000.0f : (float)MISSILE_LIFETIME * MISSILE_SPEED / 1000.0f; 
}


Chit::Chit( SpaceTree* _tree ) 
{
	tree = _tree;
	model[0] = 0;
	model[1] = 0;
	destroyed = false;
	pos.Set( 0, 0 );
	next = prev = 0;
	chitBag = 0;
	id = 0;
}

Chit::~Chit()
{
	for( int i=0; i<2; ++i ) {
		if ( model[i] )
			tree->FreeModel( model[i] );
	}
	// unlink:
	if ( chitBag )
		chitBag->Remove( this );
	prev->next = next;
	next->prev = prev;
}


UFOChit::UFOChit( SpaceTree* tree, int type, const grinliz::Vector2F& start, const grinliz::Vector2F& dest ) : Chit( tree )
{
	this->type = type;
	this->ai = AI_TRAVELLING;
	SetPos( start.x, start.y );
	this->dest = dest;
	this->speed = UFO_SPEED[type]*2.0f;
	this->effectTimer = 0;
	this->jobTimer = 0;
	this->hp = UFO_HP[type];
	decal[0] = 0;
	decal[1] = 0;

	static const char* name[3] = { "geo_scout", "geo_frigate", "geo_battleship" };
	GLASSERT( type >= 0 && type < 3 );

	model[0] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( name[type] ) );
	model[1] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( name[type] ) );
}


UFOChit::~UFOChit()
{
	for( int i=0; i<2; ++i ) {
		if ( decal[i] )
			tree->FreeModel( decal[i] );
	}
}



void UFOChit::SetAI( int _ai )
{
	GLASSERT( ai == AI_NONE || ai == AI_TRAVELLING || ai == AI_ORBIT );
	ai = _ai;
	jobTimer = 0;
	effectTimer = 0;
}


Vector2F UFOChit::Velocity() const
{
	Vector2F d = dest - pos;
	if ( d.LengthSquared() < 0.001 ) {
		d.Set( 1, 0 );	// dummy value to keep going.
	}
	d.Normalize();
	d = d * speed;
	return d;
}


float UFOChit::Radius() const
{
	return model[0]->AABB().SizeX() * 0.40f;	// fudge factor - bounds are quite circular
}

void UFOChit::DoDamage( float d )
{
	hp -= d;

	if ( hp <= 0 ) {
		static const float INV = 1.f/255.f;
		static const Color4F particleColor = { 171.f*INV, 42.f*INV, 42.f*INV, 1.0f };
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
	}
}



void UFOChit::EmitEntryExitBurn( U32 deltaTime, const Vector3F& p0, const Vector3F& p1, bool entry )
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



void UFOChit::Decal( U32 timer, float speed, int id )
{
	if ( !decal[0] ) {
		for( int i=0; i<2; ++i ) {
			decal[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( "unitplate" ) );

			gamui::RenderAtom atom = UIRenderer::CalcIcon2Atom( id, true );
			decal[i]->SetTexture( (Texture*)atom.textureHandle );
			decal[i]->SetTexXForm( 0, atom.tx1-atom.tx0, atom.ty1-atom.ty0, atom.tx0, atom.ty0 );
			
			Vector3F pos = model[i]->Pos();
			decal[i]->SetPos( pos );
		}
	}
	decal[0]->SetRotation( (float)timer * speed );
	decal[1]->SetRotation( (float)timer * speed );
}


void UFOChit::RemoveDecal()
{
	for( int i=0; i<2; ++i ) {
		tree->FreeModel( decal[i] );
		decal[i] = 0;
	}
}


int UFOChit::DoTick( U32 deltaTime )
{
	float timeFraction = (float)deltaTime / 1000.0f;
	float travel = speed*timeFraction;
	bool done = false;
	int msg = MSG_NONE;


	if ( ai != AI_CRASHED && hp <= 0 ) {
		msg = MSG_UFO_CRASHED;
	} 
	else if ( ai == AI_TRAVELLING )
	{
		Vector2F normal = dest - pos;
		if ( normal.Length() >= travel ) {
			normal.Normalize();
			float theta = ToDegree( atan2f( normal.x, normal.y ) ) + 90.0f;
			model[0]->SetRotation( theta );
			model[1]->SetRotation( theta );

			normal = normal * travel;
			pos += normal;

			// reduce speed:
			if ( speed > UFO_SPEED[type] ) {
				speed -= UFO_ACCEL*timeFraction;
				EmitEntryExitBurn( deltaTime, model[0]->Pos(), model[1]->Pos(), true );
			}
			if ( speed < UFO_SPEED[type] ) {
				speed = UFO_SPEED[type];
			}
		}
		else {
			SetPos( dest.x, dest.y );
			ai = AI_NONE;
			msg = MSG_UFO_AT_DESTINATION;
		}
	}
	else if ( ai == AI_ORBIT ) {
		Vector2F normal = { travel, 0 };
		pos += normal;
		model[0]->SetRotation( 0 );
		model[1]->SetRotation( 0 );

		speed += UFO_ACCEL*timeFraction;
		EmitEntryExitBurn( deltaTime, model[0]->Pos(), model[1]->Pos(), false );
		if ( speed >= UFO_SPEED[0] * 2.0f ) {	// everything accelerates to the scout speed

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
			msg = MSG_DONE;
		}
	}
	else if ( ai == AI_CRASHED ) {
		jobTimer += deltaTime;
		effectTimer += deltaTime;

		if ( effectTimer > 100 ) {
			effectTimer = 0;
			for( int i=0; i<2; ++i ) {
				//ParticleSystem::Instance()->EmitSmoke( deltaTime, model[i]->Pos() );

				static const float INV = 1.f/255.f;
				static const Color4F particleColor = { 129.f*INV, 21.f*INV, 21.f*INV, 1.0f };
				static const Color4F colorVec	= { -0.2f, -0.2f, -0.2f, -0.4f };
				static const Vector3F particleVel = { -0.2f, 0.2f, -0.2f };

				for( int i=0; i<2; ++i ) {

					ParticleSystem::Instance()->EmitPoint( 
									1, ParticleSystem::PARTICLE_RAY, 
									particleColor, colorVec,
									model[i]->Pos(),
									0.2f,
									particleVel, 0.1f );
				}
			}
		}
		if ( jobTimer >= UFO_CRASH_TIME ) {
			msg = MSG_DONE;
		}
	}
	else if ( ai == AI_CITY_ATTACK ) {
		jobTimer += deltaTime;		

		if ( jobTimer >= UFO_LAND_TIME ) {
			msg = MSG_CITY_ATTACK_COMPLETE;
			ai = AI_ORBIT;
			RemoveDecal();
		}
		else {
			Decal( jobTimer, 0.1f, ICON2_UFO_CITY_ATTACKING );
		}
	}
	else if ( ai == AI_BASE_ATTACK ) {
		jobTimer += deltaTime;		

		if ( jobTimer >= UFO_LAND_TIME ) {
			msg = MSG_BASE_ATTACK_COMPLETE;
			ai = AI_ORBIT;
			RemoveDecal();
		}
		else {
			Decal( jobTimer, 0.1f, ICON2_UFO_CITY_ATTACKING );
		}
	}
	else if ( ai == AI_CROP_CIRCLE ) {
		jobTimer += deltaTime;
		if ( jobTimer >= UFO_LAND_TIME ) {
			msg = MSG_CROP_CIRCLE_COMPLETE;
			ai = AI_ORBIT;
			RemoveDecal();
		}
		else {
			Decal( jobTimer, 0.2f, ICON2_UFO_CROP_CIRCLING );
		}
	}
	else if ( ai == AI_OCCUPATION ) {
		// do nothing. Can only be disloged by an soldier attack
		jobTimer += deltaTime;
		Decal( jobTimer, 0.05f, ICON2_UFO_OCCUPYING );
	}
	else {
		GLASSERT( 0 );
	}

	Vector3F pos0 = { pos.x, UFO_HEIGHT, pos.y };
	Vector3F pos1 = { pos.x, UFO_HEIGHT, pos.y };

	if ( ai == AI_CRASHED ) {
		pos0.y = 0;
		pos1.y = 0;
	}

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

	if ( ai == AI_CRASHED ) {
		model[0]->SetRotation( 30.0, 2 );
		model[1]->SetRotation( 30.0, 2 );
	}

	return msg;
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


int CropCircle::DoTick( U32 deltaTime )
{
	jobTimer += deltaTime;
	int msg = MSG_NONE;

	if ( jobTimer > CROP_CIRCLE_TIME_SECONDS*1000 ) {
		msg = MSG_DONE;
	}
	return msg;
}


CityChit::CityChit( SpaceTree* tree, const grinliz::Vector2I& posi, bool _capital ) : Chit( tree ), capital( _capital )
{
	pos.Set( (float)posi.x+0.5f, (float)posi.y+0.5f );
	for( int i=0; i<2; ++i ) {
		model[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( capital ? "capital" : "city" ) );
	}
	model[0]->SetPos( pos.x, 0, pos.y );
	model[1]->SetPos( pos.x+(float)GEO_MAP_X, 0, pos.y  );
}


BaseChit::BaseChit( SpaceTree* tree, const grinliz::Vector2I& posi, int index, const ItemDefArr& itemDefArr, bool firstBase ) : Chit( tree )
{
	pos.Set( (float)posi.x+0.5f, (float)posi.y+0.5f );
	this->index = index;

	static const char* name[MAX_BASES] = { "baseA", "baseB", "baseC", "baseD" };
	for( int i=0; i<2; ++i ) {
		model[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( name[index] ) );
	}
	model[0]->SetPos( pos.x, 0, pos.y );
	model[1]->SetPos( pos.x+(float)GEO_MAP_X, 0, pos.y  );
	lander[0] = 0;
	lander[1] = 0;

	storage = new Storage( posi.x, posi.y, itemDefArr );
	if ( firstBase ) 
		TacticalIntroScene::GenerateTerranTeam( units, MAX_TERRANS, 0, itemDefArr, 0 );
	else
		memset( units, 0, sizeof(Unit)*MAX_TERRANS );

	for( int i=0; i<NUM_FACILITIES; ++i ) {
		facilityStatus[i] = firstBase ? 0 : -1;
	}
	nScientists = firstBase ? MAX_SCIENTISTS : 0;
}


BaseChit::~BaseChit()
{
	delete storage;
	for( int i=0; i<2; ++i ) {
		if ( lander[i] ) {
			tree->FreeModel( lander[i] );
		}
	}
}


int BaseChit::DoTick(  U32 deltaTime )
{
	int msg = MSG_NONE;
	for( int i=0; i<NUM_FACILITIES; ++i ) {
		if ( facilityStatus[i] > 0 ) {
			if ( deltaTime >= (U32)facilityStatus[i] ) {
				facilityStatus[i] = 0;
			}
			else {
				facilityStatus[i] -= (int)deltaTime;
			}
		}
	}

	if ( lander[0] ) {
		// Way out or way in?
		bool wayOut = (landerTarget.x >= 0);
		Vector2F target;
		if ( wayOut ) {
			target.Set( (float)landerTarget.x + 0.5f, (float)landerTarget.y + 0.5f );
		}
		else {
			target = pos;	// position of the base
		}
		Vector2F landerPos = { lander[0]->X(), lander[0]->Z() };
		landerPos = Cylinder<float>::Normalize( landerPos );

		Vector2F delta;
		Cylinder<float>::ShortestPath( landerPos, target, &delta );

		float travel = LANDER_SPEED * (float)deltaTime / 1000.0f;

		if ( delta.Length() <= travel ) {
			if ( wayOut ) {
				landerPos = target;
				landerTarget.Set( -1, -1 );
				msg = MSG_LANDER_ARRIVED;
			}
			else {
				for( int i=0; i<2; ++i ) {
					tree->FreeModel( lander[i] );
					lander[i] = 0;
				}
			}
		}
		else {
			delta.Normalize();
			delta = delta * travel;
			landerPos = landerPos + delta;
		}

		landerPos = Cylinder<float>::Normalize( landerPos );
		if ( lander[0] ) {
			lander[0]->SetPos( landerPos.x, UFO_HEIGHT, landerPos.y );
			lander[1]->SetPos( landerPos.x+GEO_MAP_XF, UFO_HEIGHT, landerPos.y );

			float theta = ToDegree( atan2f( delta.x, delta.y ) );
			lander[0]->SetRotation( theta );
			lander[1]->SetRotation( theta );
		}
	}

	return msg;
}


void BaseChit::DeployLander( const grinliz::Vector2I& pos )
{
	GLASSERT( lander[0] == 0 );
	landerTarget = pos;

	for( int i=0; i<2; ++i ) {
		lander[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( "geolander" ) );
	}
	lander[0]->SetPos( Pos().x, UFO_HEIGHT, Pos().y );
	lander[1]->SetPos( Pos().x+GEO_MAP_XF, UFO_HEIGHT, Pos().y );
}


int BaseChit::NumUnits()
{
	return Unit::Count( units, MAX_TERRANS, Unit::STATUS_ALIVE );
}


Vector2I BaseChit::LanderMapPos() const
{
	grinliz::Vector2I v = { 0, 0 };
	if ( lander[0] ) {
		v.Set( (int)lander[0]->X(), (int)lander[0]->Z() );
	}
	return v;
}


const char* BaseChit::Name() const
{
	static const char* names[MAX_BASES] = { "Alpha", "Bravo", "Charlie", "Delta" };
	GLASSERT( index >=0 && index < MAX_BASES );
	return names[index];
}


CargoChit::CargoChit( SpaceTree* tree, const grinliz::Vector2I& start, const grinliz::Vector2I& end ) : Chit( tree )
{
	this->city.Set( (float)start.x+0.5f, (float)start.y+0.5f );
	this->base.Set( (float)end.x+0.5f, (float)end.y+0.5f );
	this->goingToBase = true;
	SetPos( city.x, city.y );
	baseInMap = end;
	cityInMap = start;

	model[0] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( "cargo" ) );
	model[1] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( "cargo" ) );
}

CargoChit::~CargoChit()
{
}


int CargoChit::DoTick( U32 deltaTime )
{
	float distance = CARGO_SPEED * (float)deltaTime / 1000.0f;
	
	Vector2F dest = goingToBase ? base : city;

	int msg = MSG_NONE;
	if ( Cylinder<float>::LengthSquared( pos, dest ) <= (distance+0.1f)*(distance+0.1f) ) {
		msg = goingToBase ? MSG_CARGO_ARRIVED : MSG_DONE;
		goingToBase = false;
	}
	else {
		Vector2F normal = dest - pos;
		normal.Normalize();
		pos += normal * distance;

		for( int i=0; i<2; ++i ) {
			float theta = ToDegree( atan2f( normal.x, normal.y ) );
			model[0]->SetRotation( theta );
			model[1]->SetRotation( theta );
		}
		Vector3F p = { pos.x, UFO_HEIGHT, pos.y };
		model[0]->SetPos( p );
		p.x += GEO_MAP_XF;
		model[1]->SetPos( p );
	}
	return msg;
}


ChitBag::ChitBag() : sentinel( 0 )
{
	idPool = 1;
	sentinel.next = &sentinel;
	sentinel.prev = &sentinel;
	memset( baseChitArr, 0, sizeof(BaseChit*)*MAX_BASES );
}


ChitBag::~ChitBag()
{
	Clear();
}

void ChitBag::Clear()
{	
	while( sentinel.next != &sentinel )
		delete sentinel.next;
}


int ChitBag::AllocBaseChitIndex()
{
	for( int i=0; i<MAX_BASES; ++i ) {
		if ( baseChitArr[i] == 0 ) {
			return i;
		}
	}
	return MAX_BASES;
}



void ChitBag::Add( Chit* chit )
{
	// Special arrays!
	if ( chit->IsBaseChit() ) {
		GLASSERT( baseChitArr[chit->IsBaseChit()->Index()] == 0 );
		baseChitArr[chit->IsBaseChit()->Index()] = chit->IsBaseChit();
	}

	chit->chitBag = this;
	chit->id = idPool++;

	chit->next = sentinel.next;
	chit->prev = &sentinel;

	sentinel.next->prev = chit;
	sentinel.next = chit;

	map.Add( chit->ID(), chit );
}


void ChitBag::Remove( Chit* chit )
{
	map.Remove( chit->ID() );

	if ( chit->IsBaseChit() ) {
		GLASSERT( baseChitArr[chit->IsBaseChit()->Index()] == chit );
		baseChitArr[chit->IsBaseChit()->Index()] = 0;
	}
}


void ChitBag::Clean()
{
	Chit* it=sentinel.next;
	while( it != &sentinel ) {
		if ( it->IsDestroyed() ) {
			Chit* temp=it;
			it=it->next;
			delete temp;
		}
		else {
			it=it->Next();
		}
	}
}


Chit* ChitBag::GetChit( const grinliz::Vector2I& pos )
{
	for( Chit* chit = sentinel.next; chit != &sentinel; chit=chit->next ) {
		if ( chit->MapPos() == pos ) {
			return chit;
		}
	}
	return 0;
}


BaseChit* ChitBag::GetBaseChitAt( const grinliz::Vector2I& pos )
{
	for( int i=0; i<MAX_BASES; ++i ) {
		if ( baseChitArr[i] && baseChitArr[i]->MapPos() == pos )
			return baseChitArr[i];
	}
	return 0;
}


CargoChit* ChitBag::GetCargoGoingTo( const grinliz::Vector2I& pos )
{
	for( Chit* chit = sentinel.next; chit != &sentinel; chit=chit->next ) {
		if ( chit->IsCargoChit() && chit->IsCargoChit()->Base() == pos ) {
			return chit->IsCargoChit();
		}
	}
	return 0;
}


Chit* ChitBag::GetParkedChitAt( const grinliz::Vector2I& pos )
{
	for( Chit* chit = sentinel.next; chit != &sentinel; chit=chit->next ) {
		if ( chit->Parked() && chit->MapPos() == pos ) {
			return chit;
		}
	}
	return 0;
}


BaseChit* ChitBag::GetBaseChit( const char* name )
{
	for( int i=0; i<MAX_BASES; ++i ) {
		if ( StrEqual( baseChitArr[i]->Name(), name ) )
			return baseChitArr[i];
	}
	return 0;
}


UFOChit* ChitBag::GetLandedUFOChitAt( const grinliz::Vector2I& pos )
{
	for( Chit* chit = sentinel.next; chit != &sentinel; chit=chit->next ) {
		if ( chit->MapPos() == pos && chit->IsUFOChit() && !chit->IsUFOChit()->Flying() ) {
			return chit->IsUFOChit();
		}
	}
	return 0;
}


int	ChitBag::NumBaseChits()
{
	int count=0;
	for( int i=0; i<MAX_BASES; ++i ) {
		if ( baseChitArr[i] )
			++count;
	}
	return count;
}

