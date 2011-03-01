#include "fastbattlescene.h"
#include "game.h"
#include "cgame.h"
#include "tacticalintroscene.h"
#include "tacticalendscene.h"


using namespace gamui;
using namespace grinliz;

FastBattleScene::FastBattleScene( Game* _game, FastBattleSceneData* data ) : Scene( _game ), foundStorage( 0, 0, _game->GetItemDefArr() )
{
	this->data = data;
	const Screenport& port = GetEngine()->GetScreenport();
	random.SetSeed( data->seed );

	backgroundUI.Init( _game, &gamui2D, false );

	static const float TEXT_SPACE = 16.0f;
	
	for( int i=0; i<NUM_TL; ++i ) {
		scenarioText[i].Init( &gamui2D );
		scenarioText[i].SetPos( 10.f, 10.f+TEXT_SPACE*(float)i );
	}
	const char* scenarioName[] = {
		"Farm Scout", "Tundra Scout", "Forest Scout", "Desert Scout", 
		"Farm Frigate", "Tundra Frigate", "Forest Frigate", "Desert Frigate",
		"City", "Battleship"
	};

	char buf[32];
	SNPrintf( buf, 32, "Rank=%d", data->alienRank );

	scenarioText[TL_SCENARIO].SetText( scenarioName[ data->scenario - TacticalIntroScene::FARM_SCOUT ] );
	scenarioText[TL_CRASH].SetText( data->crash ? "Crash" : "Normal" );
	scenarioText[TL_DAYTIME].SetText( data->dayTime ? "Day" : "Night" );
	scenarioText[TL_ALIEN_RANK].SetText( buf );

	for( int i=0; i<NUM_BATTLE; ++i ) {
		battle[i].Init( &gamui2D );
		battle[i].SetPos( 150.f, 10.f+TEXT_SPACE*(float)i );
	}

	const ButtonLook& look = game->GetButtonLook( Game::BLUE_BUTTON );
	button.Init( &gamui2D, look );
	button.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	button.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	button.SetText( "Okay" );

	TacticalIntroScene::GenerateAlienTeamUpper( data->scenario, data->crash, data->alienRank, aliens, game->GetItemDefArr(), random.Rand() );

	memset( civs, 0, sizeof(Unit)*MAX_CIVS );
	int nCiv = 0;
	// fixme: handle terran base! should be # of scientists.

	switch ( data->scenario ) {
	case TacticalIntroScene::FARM_SCOUT:
	case TacticalIntroScene::FARM_DESTROYER:
	case TacticalIntroScene::CITY:
		nCiv = MAX_CIVS;
		break;

	case TacticalIntroScene::FRST_SCOUT:
	case TacticalIntroScene::FRST_DESTROYER:
		nCiv = MAX_CIVS / 2;
		break;

	default:
		break;
	}
	TacticalIntroScene::GenerateCivTeam( civs, nCiv, game->GetItemDefArr(), random.Rand() );

	battleResult = RunSim( data->soldierUnits, aliens, data->dayTime );
	static const char* battleResultName[] = { "Victory", "Defeat", "Tie" };
	scenarioText[TL_RESULT].SetText( battleResultName[battleResult] );

}


void FastBattleScene::Tap(	int action, 
							const grinliz::Vector2F& screen,
							const grinliz::Ray& world )
{
	grinliz::Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( screen, &ui );

	const UIItem* item = 0;
	if ( action == GAME_TAP_DOWN ) {
		gamui2D.TapDown( ui.x, ui.y );
		return;
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		return;
	}
	else if ( action == GAME_TAP_UP ) {
		item = gamui2D.TapUp( ui.x, ui.y );
	}

	if ( item == &button ) {
		for( int i=0; i<MAX_ALIENS; ++i ) {
			aliens[i].GetInventory()->RestoreClips();
			const Inventory* inv = aliens[i].GetInventory();

			for( int j=0; j<Inventory::NUM_SLOTS; ++j ) {
				Item item = inv->GetItem( j );
				if ( item.IsSomething() )
					foundStorage.AddItem( item );
			}
		}

		TacticalEndSceneData* d = new TacticalEndSceneData();
		d->aliens   = aliens;
		d->soldiers = data->soldierUnits;
		d->civs     = civs;
		d->dayTime  = data->dayTime;
		d->result =  battleResult;
		d->storage = &foundStorage;
		// Don't pop ourselves, since this objects has 'alien' and 'civs' memory.
		game->PushScene( Game::END_SCENE, d );
	}
}


void FastBattleScene::SceneResult( int sceneID, int result )
{
	GLASSERT( sceneID == Game::UNIT_SCORE_SCENE );

	// add found storage to main storage
	for( int i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
		if ( foundStorage.GetCount( i ) > 0 ) {			
			data->storage->AddItem( game->GetItemDefArr().GetIndex( i ), foundStorage.GetCount( i ) );
		}
	}
	game->PopScene( battleResult );
}


int FastBattleScene::RunSim( Unit* soldier, Unit* alien, bool day )
{
	int soldierIndex = random.Rand( MAX_TERRANS );
	int alienIndex = random.Rand( MAX_ALIENS );

	static const int BUF_SIZE=60;
	char buf[BUF_SIZE];
	static const char* alienName[Unit::NUM_ALIEN_TYPES] = { "Grn", "Prme", "Hrnt", "Jckl", "Vpr" };
	
	int nBattle = 0;

	while ( true ) {
		if ( nBattle == NUM_BATTLE )
			break;
		int nSoldiers = Unit::Count( soldier, MAX_TERRANS, Unit::STATUS_ALIVE );
		int nAliens = Unit::Count( alien, MAX_ALIENS, Unit::STATUS_ALIVE );
		if ( nSoldiers == 0 || nAliens == 0 )
			break;

		int civIndex = random.Rand( MAX_CIVS );
		if ( civs[civIndex].IsAlive() ) {
			civs[civIndex].Kill( 0 );
		}

		while( true ) {
			++soldierIndex;
			if ( soldierIndex >= MAX_TERRANS ) soldierIndex = 0;
			if ( soldier[soldierIndex].IsAlive() )
				break;
		}
		while( true ) {
			++alienIndex;
			if ( alienIndex >= MAX_ALIENS ) alienIndex = 0;
			if ( alien[alienIndex].IsAlive() )
				break;
		}

		// We have our combatants.
		float range = 4.0f + 16.0f*random.Uniform();
		Unit* pS = &soldier[soldierIndex];
		Unit* pA = &alien[alienIndex];

		if ( !pS->GetWeapon() ) {
			SNPrintf( buf, BUF_SIZE, "%s %s no weapon. DEFEAT.", pS->FirstName(), pS->LastName() );
			battle[nBattle++].SetText( buf );
			pS->Kill( 0 );
			continue;
		}
		if ( !pA->GetWeapon() ) {
			SNPrintf( buf, BUF_SIZE, "%s #%d no weapon. VICTORY.", alienName[pA->AlienType()], pA-alien );
			battle[nBattle++].SetText( buf );
			pA->Kill( 0 );
			continue;
		}

		Unit* att=pS;
		Unit* def=pA;
		if ( !day || random.Bit() ) {
			Swap( &att, &def );
		}

		int subCount = 0;
		while ( pS->IsAlive() && pA->IsAlive() && subCount < 20 ) 
		{
			float tu = att->GetStats().TotalTU() * (0.5f+0.5f*random.Uniform());	// cut some TU for movement

			BulletTarget target( range );
			float chanceToHit, chanceAnyHit, tuNeeded, dptu;
			WeaponMode mode[2] = { kAutoFireMode, kSnapFireMode };

			int m=0;
			while( m < 2 ) {
				att->FireStatistics( kAutoFireMode, target, &chanceToHit, &chanceAnyHit, &tuNeeded, &dptu );

				if ( tuNeeded <= tu && att->CanFire( mode[m] ) )
				{
					const WeaponItemDef* wid = att->GetWeapon()->IsWeapon();
					const ClipItemDef* cid = wid->GetClipItemDef( mode[m] );
					int nShots = wid->RoundsNeeded( mode[m] );

					tu -= tuNeeded;
					for( int shot=0; shot<nShots; ++shot ) {
						if ( !def->IsAlive() )
							break;
						if ( att->GetInventory()->CalcClipRoundsTotal( cid ) == 0 )
							break;

						att->GetInventory()->UseClipRound( cid );

						if ( random.Uniform() < chanceToHit ) {
							DamageDesc d;
							wid->DamageBase( mode[m], &d );
							def->DoDamage( d, 0 );
						}
					}
				}
				else {
					++m;
				}

				if ( !def->IsAlive() ) {
					att->CreditKill();
					break;
				}
			}

			++subCount;
			Swap( &att, &def );
		}

		SNPrintf( buf, BUF_SIZE, "%s%s %s %s%s vs %s%s #%d %s%s", pS->IsAlive() ? "[" : "",
																  pS->FirstName(), pS->LastName(), pS->GetWeapon()->Name(),
																  pS->IsAlive() ? "]" : "",
																  pA->IsAlive() ? "[" : "",
														          alienName[pA->AlienType()], pA-alien, pA->GetWeapon()->Name(),
																  pA->IsAlive() ? "]" : "" );
		battle[nBattle++].SetText( buf );
	}

	int nSoldiers = Unit::Count( soldier, MAX_TERRANS, Unit::STATUS_ALIVE );
	int nAliens = Unit::Count( alien, MAX_ALIENS, Unit::STATUS_ALIVE );

	if ( nSoldiers == 0 ) {
		for( int i=0; i<MAX_TERRANS; ++i ) {
			if ( soldier[i].InUse() )
				soldier[i].Leave();
		}
	}

	int result = TacticalEndSceneData::TIE;
	if ( nSoldiers > 0 && nAliens == 0 )
		result = TacticalEndSceneData::VICTORY;
	else if ( nSoldiers == 0 && nAliens > 0 )
		result = TacticalEndSceneData::DEFEAT;
	return result;
}

