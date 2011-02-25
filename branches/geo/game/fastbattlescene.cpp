#include "fastbattlescene.h"
#include "game.h"
#include "cgame.h"
#include "tacticalintroscene.h"


using namespace gamui;
using namespace grinliz;

FastBattleScene::FastBattleScene( Game* _game, FastBattleSceneData* data ) : Scene( _game )
{
	this->data = data;
	const Screenport& port = GetEngine()->GetScreenport();

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
		battle[i].SetPos( 100.f, 10.f+TEXT_SPACE*(float)i );
		//scenarioText[i].SetText( "round" );
	}

	const ButtonLook& look = game->GetButtonLook( Game::BLUE_BUTTON );
	button.Init( &gamui2D, look );
	button.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	button.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	button.SetText( "Okay" );

	TacticalIntroScene::GenerateAlienTeamUpper( data->scenario, data->crash, data->alienRank, aliens, game->GetItemDefArr(), 0 );

	RunSim( data->soldierUnits, aliens, data->dayTime );
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
		game->PopScene( 0 );
	}
}


void FastBattleScene::RunSim( Unit* soldier, Unit* alien, bool day )
{
	Random random;

	int soldierIndex = random.Rand( MAX_TERRANS );
	int alienIndex = random.Rand( MAX_ALIENS );

	int nAliens = 0;
	int nSoldiers = 0;
	int nBattle = 0;

	for( int i=0; i<MAX_TERRANS; ++i ) 
		if ( soldier[i].IsAlive() )
			++nSoldiers;
	for( int i=0; i<MAX_ALIENS; ++i ) 
		if ( alien[i].IsAlive() )
			++nAliens;

	static const int BUF_SIZE=40;
	char buf[BUF_SIZE];
	static const char* alienName[Unit::NUM_ALIEN_TYPES] = { "Green", "Prime", "Hornet", "Jackal", "Viper" };

	while ( nSoldiers && nAliens ) {
		GLASSERT( nBattle < NUM_BATTLE );

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
			--nSoldiers;
			continue;
		}
		if ( !pA->GetWeapon() ) {
			SNPrintf( buf, BUF_SIZE, "%s #%d no weapon. VICTORY.", alienName[pA->AlienType()], pA-alien );
			battle[nBattle++].SetText( buf );
			pA->Kill( 0 );
			--nAliens;
			continue;
		}

		//GLOUTPUT(( "%s %s %d %s vs. %d r%d %s\n",
		//	pS->FirstName(), pS->LastName(), pS->GetStats().Rank(), pS->GetWeapon() ? pS->GetWeapon()->Name() : "none",
		//	pA->AlienType(), pA->GetStats().Rank(), pA->GetWeapon() ? pA->GetWeapon()->Name() : "none" ));

		//float tuS = day ? random.Uniform() ? 5.0f;
		//float tuA = random.Uniform();

		Unit* att=pS;
		Unit* def=pA;
		if ( !day || random.Bit() ) {
			Swap( &att, &def );
		}

		while ( pS->IsAlive() && pA->IsAlive() ) 
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
			}

			Swap( &att, &def );
		}
		const char* result = 0;
		if ( pS->IsAlive() ) {
			result = "VICTORY";
			--nAliens;
		}
		else {
			result = "Defeat";
			--nSoldiers;
		}

		SNPrintf( buf, BUF_SIZE, "%s %s %s vs %s #%d %s.", pS->FirstName(), pS->LastName(), pS->GetWeapon()->Name(),
														 alienName[pA->AlienType()], pA-alien, result );
		battle[nBattle++].SetText( buf );
	}
}

