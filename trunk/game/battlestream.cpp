#include "battlestream.h"
#include "../engine/serialize.h"
#include "unit.h"
#include "game.h"

void BattleSceneStream::Save(	int nUnit,
								int selectedUnit,
								const Unit* units )
{
	UFOStream* s = game->OpenStream( "BattleScene", true );
	GLASSERT( s );
	// Selection
	// Units[]

	s->WriteU32( UFOStream::MAGIC0 );
	s->WriteU8( selectedUnit );

	int selectedUnitLoc = s->Tell();
	int selectedUnitOffset = 0;
	s->WriteU32( 0 );	// offset to selected unit.

	for( int i=0; i<nUnit; ++i ) {
		if ( i == selectedUnit ) {
			selectedUnitOffset = s->Tell();
		}
		units[i].Save( s );
	}
	s->WriteU32( UFOStream::MAGIC1 );

	// Patch up and write the location of the selected unit. (0 if none selected)
	s->SeekSet( selectedUnitLoc );
	s->WriteU32( selectedUnitOffset );
}


void BattleSceneStream::Load(	int nUnit,
								int *selectedUnit,
								Unit* units )
{
	UFOStream* s = game->OpenStream( "BattleScene", false );
	GLASSERT( s );

	U32 magic = s->ReadU32();
	GLASSERT( magic == UFOStream::MAGIC0 );
	
	*selectedUnit = s->ReadU8();
	s->ReadU32();

	for( int i=0; i<nUnit; ++i ) {
		units[i].Load( s, &game->engine, game );
	}
	magic = s->ReadU32();
	GLASSERT( magic == UFOStream::MAGIC1 );
}


void BattleSceneStream::LoadSelectedUnit( Unit* unit )
{
	UFOStream* s = game->OpenStream( "BattleScene", false );
	GLASSERT( s );

	U32 magic = s->ReadU32();
	GLASSERT( magic == UFOStream::MAGIC0 );	
	s->ReadU8();
	int offset = s->ReadU32();
	GLASSERT( offset );
	s->SeekSet( offset );
	unit->Load( s, &game->engine, game );
}


void BattleSceneStream::SaveSelectedUnit( const Unit* unit )
{
	UFOStream* s = game->OpenStream( "BattleScene", false );
	GLASSERT( s );

	U32 magic = s->ReadU32();
	GLASSERT( magic == UFOStream::MAGIC0 );	
	s->ReadU8();
	int offset = s->ReadU32();
	GLASSERT( offset );
	s->SeekSet( offset );
	unit->Save( s );
}
