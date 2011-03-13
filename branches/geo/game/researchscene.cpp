#include "researchscene.h"
#include "../engine/engine.h"
#include "research.h"

#include "gamelimits.h"
#include "game.h"
#include "cgame.h"

using namespace gamui;
using namespace grinliz;

ResearchScene::ResearchScene( Game* _game, ResearchSceneData* _data ) : Scene( _game ), data( _data )
{
	const Screenport& port = GetEngine()->GetScreenport();

	backgroundUI.Init( game, &gamui2D, false );

	static const float SPACE = 5.0f;
	static const float X0 = GAME_BUTTON_SIZE_F + GAME_GUTTER;
	const ButtonLook& blue = game->GetButtonLook( Game::BLUE_BUTTON );

	mainDescription.Init( &gamui2D );
	mainDescription.SetSize( port.UIWidth() - GAME_GUTTER - X0, 200 );
	mainDescription.SetPos( X0, GAME_GUTTER );
	mainDescription.SetText( "description" );

	for( int i=0; i<MAX_OPTIONS; ++i ) {
		float y = port.UIHeight() - (SPACE+GAME_BUTTON_SIZE_F)*(float)(MAX_OPTIONS-i) + SPACE;

		optionButton[i].Init( &gamui2D, blue );
		optionButton[i].SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
		optionButton[i].SetPos( X0, y );
		optionButton[i].SetText( "test" );

		optionName[i].Init( &gamui2D );
		optionName[i].SetPos( X0+GAME_GUTTER+GAME_BUTTON_SIZE_F, y+5 );
		optionName[i].SetText( "Research" );

		optionRequires[i].Init( &gamui2D );
		optionRequires[i].SetPos(  X0+GAME_GUTTER+GAME_BUTTON_SIZE_F, y+25 );
		optionRequires[i].SetText( "Requires" );

		if ( i ) {
			optionButton[0].AddToToggleGroup( &optionButton[i] );
		}
	}

	okayButton.Init( &gamui2D, blue );
	okayButton.SetSize( GAME_BUTTON_SIZE_F, GAME_BUTTON_SIZE_F );
	okayButton.SetPos( 0, port.UIHeight()-GAME_BUTTON_SIZE_F );
	okayButton.SetText( "Done" );

	SetOptions();
	SetDescription();
}


void ResearchScene::Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )
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

	if ( item == &okayButton ) {
		data->research->SetCurrent( 0 );
		for( int i=0; i<MAX_OPTIONS; i++ ) {
			if ( optionButton[i].Down() && optionButton[i].Enabled() ) {
				data->research->SetCurrent( optionName[i].GetText() );
				break;
			}
		}
		game->PopScene();
	}
}


void ResearchScene::SetDescription()
{
	Research* r = data->research;
	if ( r->ResearchReady() ) {
		const Research::Task* task = r->Current();
		mainDescription.SetText( r->GetDescription( task ) );
	}
	else {
		if ( r->ResearchInProgress() ) {
			mainDescription.SetText( "Research in progress." );
		}
		else {
			int i;
			for( i=0; i<MAX_OPTIONS; i++ ) {
				if ( optionButton[i].Down() && optionButton[i].Enabled() ) {
					mainDescription.SetText( "Research in progress." );
					break;
				}
			}
			if ( i == MAX_OPTIONS ) {
				mainDescription.SetText( "Research requirements not met." );
			}
		}
	};
}


void ResearchScene::SetOptions()
{
	Research* r = data->research;
	int count = 0;
	int i=0;
	const Research::Task* taskArr = r->TaskArr();
	static const int SZ=64;
	char buf[SZ];
	bool downSet = false;

	for( i=0; i<r->NumTasks() && count < MAX_OPTIONS; ++i ) {
		if ( !taskArr[i].IsComplete() && taskArr[i].HasPreReq() ) {
			int percent = (int)(100.0f * (float)taskArr[i].rp / (float)taskArr[i].rpRequired );
			SNPrintf( buf, SZ, "%d%%", percent );
			optionButton[count].SetText( buf );

			optionName[count].SetText( taskArr[i].name );

			if ( r->Current() && StrEqual( r->Current()->name, taskArr[i].name ) ) {
				optionButton[count].SetDown();
				downSet = true;
			}

			GLASSERT( Research::MAX_ITEMS_REQUIRED == 4 );
			SNPrintf( buf, SZ, "Requires: %s %s %s %s", 
						taskArr[i].item[0] ? taskArr[i].item[0] : "",
						taskArr[i].item[1] ? taskArr[i].item[1] : "",
						taskArr[i].item[2] ? taskArr[i].item[2] : "",
						taskArr[i].item[3] ? taskArr[i].item[3] : "" );
			optionRequires[count].SetText( buf );

			if ( taskArr[i].HasItems() ) {
				optionRequires[count].SetEnabled( false );
				optionButton[count].SetEnabled( true );
			}
			else {
				optionRequires[count].SetEnabled( true );
				optionButton[count].SetEnabled( false );
			}
			++count;
		}
	}
	for( count; count<MAX_OPTIONS; ++count ) {
		optionButton[count].SetEnabled( false );
		optionButton[count].SetText( "" );
		optionName[count].SetText( "" );
		optionRequires[count].SetText( "" );
	}
	if ( !downSet ) {
		for( int i=0; i<MAX_OPTIONS; ++i ) {
			if ( optionButton[i].Enabled() ) {
				optionButton[i].SetDown();
				break;
			}
		}
	}
}

