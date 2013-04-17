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

#include "chits.h"
#include "geoscene.h"
#include "item.h"

#include "../engine/loosequadtree.h"
#include "../engine/particle.h"
#include "../engine/uirendering.h"

#include "tacticalintroscene.h"		// fixme: only for unit generation
#include "game.h"
#include "ufosound.h"

using namespace grinliz;
using namespace tinyxml2;

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


void Chit::Save( XMLPrinter* printer )
{
	printer->PushAttribute( "pos.x", pos.x );
	printer->PushAttribute( "pos.y", pos.y );
	printer->PushAttribute( "id", id );
}


void Chit::Load( const XMLElement* doc )
{
	doc->QueryFloatAttribute( "pos.x", &pos.x );
	doc->QueryFloatAttribute( "pos.y", &pos.y );
	//pos = Cylinder<float>::Normalize( pos ); // subtle bug: ufos will turn around if normalized
	doc->QueryIntAttribute( "id", &id );
}


float BaseChit::MissileRange( const int type ) const
{ 
	return ( type == 0 ) ? (float)GUN_LIFETIME * GUN_SPEED / 1000.0f : (float)MISSILE_LIFETIME * MISSILE_SPEED / 1000.0f; 
}


Chit::Chit( SpaceTree* _tree ) 
{
	pos.Set( 0, 0 );
	model[0] = 0;
	model[1] = 0;
	tree = _tree;
	destroyed = false;
	id = 0;

	next = prev = 0;
	chitBag = 0;
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
	this->dest = dest;
	this->speed = UFO_SPEED[type]*2.0f;
	this->effectTimer = 0;
	this->jobTimer = 0;
	this->hp = UFO_HP[type];
	decal[0] = 0;
	decal[1] = 0;
	SetPos( start.x, start.y );
	Init();
}


UFOChit::~UFOChit()
{
	for( int i=0; i<2; ++i ) {
		if ( decal[i] ) {
			tree->FreeModel( decal[i] );
			decal[i] = 0;
		}
	}
}


void UFOChit::Init()
{
	static const char* name[NUM_TYPES] = { "geo_scout", "geo_frigate", "geo_battleship", "geo_alienBase" };
	GLASSERT( type >= 0 && type < NUM_TYPES );

	for( int i=0; i<2; ++i ) {
		if ( model[i] ) tree->FreeModel( model[i] );
		model[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( name[type] ) );
		// Decals get re-created as needed.
		if ( decal[i] ) { 
			tree->FreeModel( decal[i] ); 
			decal[i] = 0; 
		}
	}
}


void UFOChit::Save( XMLPrinter* printer )
{
	printer->OpenElement( "UFOChit" );
	Chit::Save( printer );

	printer->PushAttribute( "dest.x", dest.x );
	printer->PushAttribute( "dest.y", dest.y );
	printer->PushAttribute( "type", type );
	printer->PushAttribute( "ai", ai );
	printer->PushAttribute( "speed", speed );
	printer->PushAttribute( "hp", hp );
	printer->PushAttribute( "effectTimer", effectTimer );
	printer->PushAttribute( "jobTimer", jobTimer );

	printer->CloseElement();	// UFOChit
}


void UFOChit::Load( const XMLElement* doc )
{
	Chit::Load( doc );

	doc->QueryFloatAttribute( "dest.x", &dest.x );
	doc->QueryFloatAttribute( "dest.y", &dest.y );
	doc->QueryIntAttribute( "type", &type );
	doc->QueryIntAttribute( "ai", &ai );
	doc->QueryFloatAttribute( "speed", &speed );
	doc->QueryFloatAttribute( "hp", &hp );
	doc->QueryUnsignedAttribute( "effectTimer", &effectTimer );
	doc->QueryUnsignedAttribute( "jobTimer", &jobTimer );
	Init();
}


void UFOChit::SetAI( int _ai )
{
	//GLASSERT( ai == AI_NONE || ai == AI_TRAVELLING || ai == AI_ORBIT );
	RemoveDecal();
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
		hp = 0;
		static const float INV = 1.f/255.f;
		static const Color4F particleColor = { 171.f*INV, 42.f*INV, 42.f*INV, 1.0f };
		static const Color4F colorVec	= { 0.0f, -0.1f, -0.1f, -0.5f };
		static const Vector3F particleVel = { 0.4f, 0, 0 };

		//int count = TVMode() ? 1 : 2;
		int count = 2;
		for( int i=0; i<count; ++i ) {
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
		
		//int count = TVMode() ? 1 : 2;
		int count = 2;
		for( int i=0; i<count; ++i ) {
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
			decal[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( "unitplateplus" ) );

			gamui::RenderAtom atom = Game::CalcIcon2Atom( id, true );
			decal[i]->SetTexture( (Texture*)atom.textureHandle );
			decal[i]->SetTexXForm( 0, atom.tx1-atom.tx0, atom.ty1-atom.ty0, atom.tx0, atom.ty0 );
		}
	}
	for( int i=0; i<2; ++i ) {
		decal[i]->SetRotation( (float)timer * speed );
		Vector3F pos = model[i]->Pos();
		decal[i]->SetPos( pos );
	}
	//if ( TVMode() ) {
	//	decal[1]->SetFlag( Model::MODEL_INVISIBLE );
	//}
}


void UFOChit::RemoveDecal()
{
	for( int i=0; i<2; ++i ) {
		if ( decal[i] ) {
			tree->FreeModel( decal[i] );
			decal[i] = 0;
		}
	}
}


int UFOChit::DoTick( U32 deltaTime )
{
	float timeFraction = (float)deltaTime / 1000.0f;
	float travel = speed*timeFraction;
	//bool done = false;
	int msg = MSG_NONE;
	//U32 timeBoost = 10;		// in 10ths
	//if ( hp < UFO_HP[type]) {
	//	float tb = Interpolate( 0.0f, 20.0f, UFO_HP[type], 10.0f, hp );
	//	GLASSERT( tb > 0 );
	//	if ( tb > 0 )
	//		timeBoost = (U32)tb;
	//}

	if ( ai != AI_CRASHED && hp <= 0 ) {
		// Minor issue: there are no crashed battleship models.
		// Should fix this, but until then, just blow up battleships
		if ( type != BATTLESHIP ) {
			msg = MSG_UFO_CRASHED;
		}
		else {
			msg = MSG_BATTLESHIP_DESTROYED;
			SetDestroyed();
		}
	} 
	else if ( ai == AI_PARKED ) 
	{
		// sit forever!
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

			//int count = TVMode() ? 1 : 2;
			int count = 2;
			for( int i=0; i<count; ++i ) {
				ParticleSystem::Instance()->EmitPoint( 
								40, ParticleSystem::PARTICLE_SPHERE, 
								particleColor, colorVec,
								model[i]->Pos(),
								0.2f,
								particleVel, 0.1f );
			}
			SoundManager::Instance()->QueueSound( "geo_ufo_warp" );
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

				//int count = TVMode() ? 1 : 2;
				int count = 2;
				for( int i=0; i<count; ++i ) {

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
//	else if ( ai == AI_BASE_ATTACK ) {
//		jobTimer += deltaTime;		
//
//		if ( jobTimer >= UFO_LAND_TIME ) {
//			msg = MSG_BASE_ATTACK_COMPLETE;
//			RemoveDecal();
//			// This method needs to keep being sent. If the lander
//			// is deployed, the UFO will wait until return. The
//			// AI will be set to ORBIT when the message is handled.
//		}
//		else {
//			Decal( jobTimer, 0.1f, ICON2_UFO_CITY_ATTACKING );
//		}
//	}
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
		//GLASSERT( 0 );
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

	/*
	if ( TVMode() ) {
		model[0]->ClearFlag( Model::MODEL_INVISIBLE );
		model[1]->ClearFlag( Model::MODEL_INVISIBLE );
		if ( model[0]->Pos().x < 0 )
			model[0]->SetFlag( Model::MODEL_INVISIBLE );
		if ( model[1]->Pos().x > GEO_MAP_XF )
			model[1]->SetFlag( Model::MODEL_INVISIBLE );
	}
	*/
	return msg;
}


CropCircle::CropCircle( SpaceTree* tree, const Vector2I& mapPos, U32 seed ) : Chit( tree )
{
	jobTimer = 0;
	pos.Set( (float)mapPos.x+0.5f, (float)mapPos.y+0.5f );
	Init( seed );
}


void CropCircle::Init( U32 seed )
{
	Texture* texture = TextureManager::Instance()->GetTexture( "cropcircle" );
	Random r( seed );
	float dx = (float)(r.Rand(CROP_CIRCLES_X)) / (float)CROP_CIRCLES_X;
	float dy = (float)(r.Rand(CROP_CIRCLES_Y)) / (float)CROP_CIRCLES_Y;

	for( int i=0; i<2; ++i ) {
		if ( model[i] ) tree->FreeModel( model[i] );
		model[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( "unitplate" ) );
		model[i]->SetTexture( texture );
		model[i]->SetTexXForm( 0, 1.f/(float)CROP_CIRCLES_X, 1.f/(float)CROP_CIRCLES_Y, dx, dy );
	}
	
	//if ( TVMode() )
	//	model[1]->SetFlag( Model::MODEL_INVISIBLE );

	Vector3F pos3 = { pos.x, UFO_HEIGHT*0.1f, pos.y };
	while( pos3.x < 0 ) pos3.x += GEO_MAP_X;	
	while( pos3.x >= GEO_MAP_X*2 ) pos3.x -= GEO_MAP_X;	// save/load bug
	model[0]->SetPos( pos3 );
	pos3.x += (float)GEO_MAP_X;
	model[1]->SetPos( pos3 );
}


CropCircle::~CropCircle()
{
	// nothing. model released by parent.
}


void CropCircle::Save( XMLPrinter* printer )
{
	printer->OpenElement( "CropCircle" );
	Chit::Save( printer );

	printer->PushAttribute( "jobTimer", jobTimer );

	printer->CloseElement();	// CropCirle
}


void CropCircle::Load( const XMLElement* doc )
{
	Chit::Load( doc );
	doc->QueryUnsignedAttribute( "jobTimer", &jobTimer );
	Init( jobTimer );
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
	Init();
}


void CityChit::Init()
{
	for( int i=0; i<2; ++i ) {
		if ( model[i] ) tree->FreeModel( model[i] );
		model[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( capital ? "capital" : "city" ) );
	}
	model[0]->SetPos( pos.x, 0, pos.y );
	model[1]->SetPos( pos.x+(float)GEO_MAP_X, 0, pos.y  );
	//if ( TVMode() )
	//	model[1]->SetFlag( Model::MODEL_INVISIBLE );
}


void CityChit::Save( XMLPrinter* printer )
{
	// Don't save or load. Generated from region data.
	/*
	XMLUtil::OpenElement( fp, depth, "CityChit" );
	Chit::Save( fp, depth );

	XMLUtil::Attribute( fp, "capital", capital );

	XMLUtil::SealCloseElement( fp );
	*/
}


void CityChit::Load( const XMLElement* doc )
{
	// Don't save or load. Generated from region data.
	/*
	Chit::Load( doc );
	doc->QueryBoolAttribute( "capital", &capital );
	Init();
	*/
}


BaseChit::BaseChit( SpaceTree* tree, const grinliz::Vector2I& posi, int index, const ItemDefArr& itemDefArr, bool firstBase, bool soldierBoost ) : Chit( tree )
{
	pos.Set( (float)posi.x+0.5f, (float)posi.y+0.5f );
	this->index = index;

	storage = new Storage( posi.x, posi.y, itemDefArr );

	memset( units, 0, sizeof(Unit)*MAX_TERRANS );	// some horrible bug in the
													// unit class. splatter fix.
	Random random;
	random.SetSeedFromTime();

	if ( firstBase ) {
		TacticalIntroScene::GenerateTerranTeam( units, MAX_TERRANS, soldierBoost ? 0.8f : 0, itemDefArr, random.Rand() );
	}

	for( int i=0; i<NUM_FACILITIES; ++i ) {
		facilityStatus[i] = firstBase ? 0 : -1;
	}
	nScientists = firstBase ? MAX_SCIENTISTS : 0;

	Init();
}


void BaseChit::Init()
{
	static const char* name[MAX_BASES] = { "baseA", "baseB", "baseC", "baseD" };
	for( int i=0; i<2; ++i ) {
		if ( model[i] ) tree->FreeModel( model[i] );
		model[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( name[index] ) );
	}

	model[0]->SetPos( pos.x, 0, pos.y );
	model[1]->SetPos( pos.x+(float)GEO_MAP_X, 0, pos.y  );
	//if ( TVMode() )
	//	model[1]->SetFlag( Model::MODEL_INVISIBLE );
}


BaseChit::~BaseChit()
{
	delete storage;
}


void BaseChit::Save( XMLPrinter* printer )
{
	printer->OpenElement( "BaseChit" );
	Chit::Save( printer );

	printer->PushAttribute( "index", index );
	printer->PushAttribute( "nScientists", nScientists );

	storage->Save( printer );
	
	printer->OpenElement( "Facilities" );
	for( int i=0; i<NUM_FACILITIES; ++i ) {
		printer->OpenElement( "Facility" );
		printer->PushAttribute( "facilityStatus", facilityStatus[i] );
		printer->CloseElement();
	}
	printer->CloseElement();	// Facilities

	printer->OpenElement( "Units" );
	for( int i=0; i<MAX_TERRANS; ++i ) {
		units[i].Save( printer );
	}
	printer->CloseElement();	// Units
	printer->CloseElement();	// BaseChit
}


void BaseChit::Load( const XMLElement* doc, Game* game )
{
	Chit::Load( doc );
	doc->QueryIntAttribute( "index", &index );
	doc->QueryIntAttribute( "nScientists", &nScientists );

	storage->Clear();
	storage->Load( doc );

	const XMLElement* facilities = doc->FirstChildElement( "Facilities" );
	if ( facilities ) {
		int i=0;
		for( const XMLElement* f=facilities->FirstChildElement( "Facility" ); f && i<NUM_FACILITIES; f=f->NextSiblingElement( "Facility" ), ++i ) {
			f->QueryIntAttribute( "facilityStatus", &facilityStatus[i] );
		}
	}

	const XMLElement* unitsEle = doc->FirstChildElement( "Units" );
	if ( unitsEle ) {
		int i=0;
		for( const XMLElement* unitEle=unitsEle->FirstChildElement( "Unit" ); unitEle && i<MAX_TERRANS; unitEle=unitEle->NextSiblingElement( "Unit" ), ++i ) {
			units[i].Load( unitEle, game->GetItemDefArr() );
		}
	}

	Init();
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

	/*
	if ( lander[0] ) {
		// Way out or way in?
		Vector2F target;
		if ( landerOutbound ) {
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
			if ( landerOutbound ) {
				landerPos = target;
				landerOutbound = false;
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
	*/
	return msg;
}


int BaseChit::NumUnits() const
{
	return Unit::Count( units, MAX_TERRANS, Unit::STATUS_ALIVE );
}


bool BaseChit::IssueUnitWarning() const
{
	int nUnits = NumUnits();
	if ( nUnits < 4 ) return true;
	for( int i=0; i<MAX_TERRANS; ++i ) {
		if ( units[i].IsAlive() ) {
			const Item* weapon = units[i].GetWeapon();
			if ( !weapon || weapon->IsNothing() )
				return true;
			if ( !units[i].HasGunAndAmmo( true ) )
				return true;
		}
	}
	return false;
}

	
const char* BaseChit::Name() const
{
	static const char* names[MAX_BASES] = { "Alpha", "Bravo", "Charlie", "Delta" };
	GLASSERT( index >=0 && index < MAX_BASES );
	return names[index];
}


CargoChit::CargoChit( SpaceTree* tree, int type, const grinliz::Vector2I& start, const grinliz::Vector2I& end ) : Chit( tree )
{
	this->type = type;
	this->outbound = true;
	this->origin = start;
	this->dest = end;
	SetPos( (float)start.x+0.5f, (float)start.y+0.5f );
	model[0] = 0; 
	model[1] = 0;
	Init();

}

CargoChit::~CargoChit()
{
}


void CargoChit::Init()
{
	//const char* desc = (type == TYPE_CARGO) ? "cargo" : "geolander";
	const char* desc = "geolander";
	for( int i=0; i<2; ++i ) {
		if ( model[i] ) tree->FreeModel( model[i] );
		model[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( desc ) );
	}
}


void CargoChit::Save( XMLPrinter* printer )
{
	printer->OpenElement( "CargoChit" );
	Chit::Save( printer );

	printer->PushAttribute( "type", type );
	printer->PushAttribute( "outbound", outbound );
	printer->PushAttribute( "origin.x", origin.x );
	printer->PushAttribute( "origin.y", origin.y );
	printer->PushAttribute( "dest.x", dest.x );
	printer->PushAttribute( "dest.y", dest.y );

	printer->CloseElement();
}


void CargoChit::Load( const XMLElement* doc )
{
	Chit::Load( doc );
	doc->QueryIntAttribute( "type", &type );
	doc->QueryBoolAttribute( "outbound", &outbound );
	doc->QueryIntAttribute( "origin.x", &origin.x );
	doc->QueryIntAttribute( "origin.y", &origin.y );
	doc->QueryIntAttribute( "dest.x", &dest.x );
	doc->QueryIntAttribute( "dest.y", &dest.y );

	Init();
}


void CargoChit::CheckDest( const ChitBag& chitBag )
{
	if (    outbound && ( type == TYPE_LANDER ) 
		 && MapPos() != Dest() ) 
	{
		if ( !chitBag.GetParkedChitAt( Dest() ) ) {
			outbound = false;	// go home.
		}
	}
}


int CargoChit::DoTick( U32 deltaTime )
{
	float distance = CARGO_SPEED * (float)deltaTime / 1000.0f;
	if ( type == TYPE_LANDER )
		distance = LANDER_SPEED * (float)deltaTime / 1000.0f;
	
	Vector2F d;
	if ( outbound ) {
		d.Set( (float)dest.x+0.5f, (float)dest.y+0.5f );
	}
	else {
		d.Set( (float)origin.x+0.5f, (float)origin.y+0.5f );

	}
	int msg = MSG_NONE;
	if ( Cylinder<float>::LengthSquared( pos, d ) <= (distance+0.1f)*(distance+0.1f) ) {
		if ( !outbound ) {
			msg = MSG_DONE;
		}
#ifndef IMMEDIATE_BUY
		else if ( type == TYPE_CARGO ) {
			msg = MSG_CARGO_ARRIVED;
		}
#endif
		else {
			msg = MSG_LANDER_ARRIVED;
		}
		outbound = false;
	}
	else {
		Vector2F normal;
		Cylinder<float>::ShortestPath( pos, d, &normal );
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

		/*
		if ( TVMode() ) {
			model[0]->ClearFlag( Model::MODEL_INVISIBLE );
			model[1]->ClearFlag( Model::MODEL_INVISIBLE );
			if ( model[0]->Pos().x < 0 )
				model[0]->SetFlag( Model::MODEL_INVISIBLE );
			if ( model[1]->Pos().x > GEO_MAP_XF )
				model[1]->SetFlag( Model::MODEL_INVISIBLE );
		}
		*/
	}
	return msg;
}


ChitBag::ChitBag() : sentinel( 0 )
{
	idPool = 1;
	sentinel.next = &sentinel;
	sentinel.prev = &sentinel;
	memset( baseChitArr, 0, sizeof(BaseChit*)*MAX_BASES );
	battleLanderID = 0;
	battleUFOID = 0;
	battleScenario = 0;
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
	if ( chit->id == 0 )
		chit->id = idPool++;

	chit->next = sentinel.next;
	chit->prev = &sentinel;

	sentinel.next->prev = chit;
	sentinel.next = chit;
}


void ChitBag::Remove( Chit* chit )
{
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
			Remove( it );
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


CargoChit* ChitBag::GetCargoGoingTo( int type, const grinliz::Vector2I& pos )
{
	for( Chit* chit = sentinel.next; chit != &sentinel; chit=chit->next ) {
		CargoChit* cargo = chit->IsCargoChit();
		if ( cargo && cargo->Type() == type && cargo->Dest() == pos ) {
			return chit->IsCargoChit();
		}
	}
	return 0;
}


CargoChit* ChitBag::GetCargoComingFrom( int type, const grinliz::Vector2I& from )
{
	for( Chit* chit = sentinel.next; chit != &sentinel; chit=chit->next ) {
		CargoChit* cargo = chit->IsCargoChit();
		if ( cargo && cargo->Type() == type && cargo->Origin() == from ) {
			return chit->IsCargoChit();
		}
	}
	return 0;
}


Chit* ChitBag::GetParkedChitAt( const grinliz::Vector2I& pos ) const
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
		if ( baseChitArr[i] && StrEqual( baseChitArr[i]->Name(), name ) )
			return baseChitArr[i];
	}
	return 0;
}


UFOChit* ChitBag::GetLandedUFOChitAt( const grinliz::Vector2I& pos ) const
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


void ChitBag::SetBattle( const UFOChit* ufo, const CargoChit* lander, int scenario )	
{ 
	this->battleUFOID = ufo->ID(); 
	GLASSERT( lander || (scenario == TERRAN_BASE ) );
	this->battleLanderID = lander ? lander->ID() : 0; 
	this->battleScenario = scenario; 
}


UFOChit* ChitBag::GetUFOBase()
{
	for( Chit* chit = sentinel.next; chit != &sentinel; chit=chit->next ) {
		if ( chit->IsUFOChit() && (chit->IsUFOChit()->Type() == UFOChit::BASE) ) {
			return chit->IsUFOChit();
		}
	}
	return 0;
}


void ChitBag::Save( XMLPrinter* printer )
{
	printer->OpenElement( "ChitBag" );
	printer->PushAttribute( "battleUFOID", battleUFOID );
	printer->PushAttribute( "battleLanderID", battleLanderID );
	printer->PushAttribute( "battleScenario", battleScenario );

	// Save all the chits. But the bases at the bottom for readability.
	for( int i=0; i<2; ++i ) {
		for( Chit* it=Begin(); it!=End(); it=it->Next() ) {
			if ( i ^ (it->IsBaseChit() ? 0 : 1 ) )
				it->Save( printer );
		}
	}
	printer->CloseElement();	// ChitBag

}


void ChitBag::Load( const XMLElement* doc, 
					SpaceTree* tree, 
					const ItemDefArr& arr, 
					Game* game )
{
	Vector2F v0 = {0,0}, v1 = {0,0};
	Vector2I vi0 = {0,0}, vi1 = {0,0};

	const XMLElement* bag = doc->FirstChildElement( "ChitBag" );
	Clear();
	if ( bag ) {
		bag->QueryIntAttribute( "battleUFOID", &battleUFOID );
		bag->QueryIntAttribute( "battleLanderID", &battleLanderID );
		bag->QueryIntAttribute( "battleScenario", &battleScenario );

		for( const XMLElement* chitEle = bag->FirstChildElement(); chitEle; chitEle=chitEle->NextSiblingElement() ) {
			if ( StrEqual( chitEle->Value(), "UFOChit" ) ) {
				UFOChit* ufoChit = new UFOChit( tree, 0, v0, v1 );
				ufoChit->Load( chitEle );
				Add( ufoChit );
			}
			else if ( StrEqual( chitEle->Value(), "CropCircle" ) ) {
				CropCircle* cropCircle = new CropCircle( tree, vi0, 0 );
				cropCircle->Load( chitEle );
				Add( cropCircle );
			}
			else if ( StrEqual( chitEle->Value(), "CityChit" ) ) {
				CityChit* cityChit = new CityChit( tree, vi0, false );
				cityChit->Load( chitEle );
				Add( cityChit );
			}
			else if ( StrEqual( chitEle->Value(), "BaseChit" )) {
				BaseChit* baseChit = new BaseChit( tree, vi0, 0, arr, false, false );
				baseChit->Load( chitEle, game );
				Add( baseChit );
			}
			else if ( StrEqual( chitEle->Value(), "CargoChit" )) {
				CargoChit* cargoChit = new CargoChit( tree, 0, vi0, vi1 );
				cargoChit->Load( chitEle );
				Add( cargoChit );
			}
			else {
				GLASSERT( 0 );
			}
		}
	}
	for( Chit* chit=Begin(); chit != End(); chit=chit->Next() ) {
		idPool = Max( idPool, chit->ID()+1 );
	}
}
