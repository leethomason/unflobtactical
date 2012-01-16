#include "firewidget.h"
#include "../engine/uirendering.h"
#include "../engine/model.h"
#include "../engine/particle.h"
#include "../engine/engine.h"
#include "battlescene.h"
#include "stats.h"
#include "game.h"

using namespace gamui;
using namespace grinliz;

void FireWidget::Init( Gamui* gamui, const ButtonLook& red ) 
{
	for( int i=0; i<MAX_FIRE_BUTTONS; ++i ) {
		fireButton[i].Init( gamui, red );
		fireButton[i].SetSize( 120.f, 60.f );
		fireButton[i].SetDeco( Game::CalcIconAtom( ICON_GREEN_WALK_MARK, true ), 
			                   Game::CalcIconAtom( ICON_GREEN_WALK_MARK, false ) );
		fireButton[i].SetVisible( false );
		fireButton[i].SetText( " " );
		fireButton[i].SetText2( " " );
		fireButton[i].SetDecoLayout( gamui::Button::RIGHT, 25, 0 );
		fireButton[i].SetTextLayout( gamui::Button::LEFT, 20, 0 );
	}	
}


void FireWidget::Place( BattleScene* battle,
						const Unit* shooterUnit,
						const Unit* targetUnit, 
						const grinliz::Vector2I& targetPos )
{
	GLASSERT( targetUnit || targetPos.x >= 0 );

	const Model* targetModel = 0;
	const Model* targetWeapon = 0;
	
	Vector3F target;
	BulletTarget bulletTarget;

	if ( targetPos.x >= 0 ) {
		target.Set( (float)targetPos.x+0.5f, 1.0f, (float)targetPos.y+0.5f );
	}
	else {
		targetModel = battle->GetModel( targetUnit );
		targetWeapon = battle->GetWeaponModel( targetUnit );
		GLASSERT( targetModel );
		targetModel->CalcTarget( &target );
		targetModel->CalcTargetSize( &bulletTarget.width, &bulletTarget.height );
	}

	Vector3F distVector = target - shooterUnit->Pos();
	bulletTarget.distance = distVector.Length();

	const Inventory* inventory = shooterUnit->GetInventory();
	const WeaponItemDef* wid = shooterUnit->GetWeaponDef();

	float snapTU=0, autoTU=0, altTU=0;
	if ( wid ) {
		shooterUnit->AllFireTimeUnits( &snapTU, &autoTU, &altTU );
	}

	int numButtons = 0;
	if ( wid )
	{
		for( int i=0; i<WeaponItemDef::MAX_MODE && wid->HasWeapon(i); ++i ) 
		{
			float fraction, anyFraction, dptu, tu;

			shooterUnit->FireStatistics( i, bulletTarget, &fraction, &anyFraction, &tu, &dptu );
			int nRounds = inventory->CalcClipRoundsTotal( wid->GetClipItemDef( i) );

			anyFraction = Clamp( anyFraction, 0.0f, 0.95f );

			char buffer0[32];
			char buffer1[32];
			SNPrintf( buffer0, 32, "%s %d%%", wid->weapon[i]->desc, (int)LRintf( anyFraction*100.0f ) );
			SNPrintf( buffer1, 32, "%d/%d", wid->RoundsNeeded( i ), nRounds );

			fireButton[i].SetEnabled( true );
			fireButton[i].SetVisible( true );
			fireButton[i].SetText( buffer0 );
			fireButton[i].SetText2( buffer1 );

			if ( shooterUnit->CanFire(  i )) {
				// Reflect the TU left.
				float tuAfter = shooterUnit->TU() - tu;
				int tuIndicator = ICON_ORANGE_WALK_MARK;

				if ( tuAfter >= autoTU ) {
					tuIndicator = ICON_GREEN_WALK_MARK;
				}
				else if ( tuAfter >= snapTU ) {
					tuIndicator = ICON_YELLOW_WALK_MARK;
				}
				else if ( tuAfter < snapTU ) {
					tuIndicator = ICON_ORANGE_WALK_MARK;
				}
				fireButton[i].SetDeco( Game::CalcIconAtom( tuIndicator, true ), Game::CalcIconAtom( tuIndicator, false ) );
			}
			else {
				fireButton[i].SetEnabled( false );
				RenderAtom nullAtom;
				fireButton[i].SetDeco( nullAtom, nullAtom );
			}
			numButtons = i+1;
		}
	}
	else {
		fireButton[0].SetVisible( true );
		fireButton[0].SetEnabled( false );
		fireButton[0].SetText( "[none]" );
		fireButton[0].SetText2( "" );
		RenderAtom nullAtom;
		fireButton[0].SetDeco( nullAtom, nullAtom );
		numButtons = 1;
	}
	for( int i=numButtons; i<WeaponItemDef::MAX_MODE; ++i ) {
		fireButton[i].SetVisible( false );
	}

	Vector2F view, ui;
	const Screenport& port = battle->GetEngine()->GetScreenport();
	port.WorldToView( target, &view );
	port.ViewToUI( view, &ui );

	const int DX = 10;

	// Make sure it fits on the screen.
	UIRenderer::LayoutListOnScreen( fireButton, numButtons, sizeof(fireButton[0]), ui.x+DX, ui.y, FIRE_BUTTON_SPACING, port );

	ParticleSystem* ps = ParticleSystem::Instance();
	Game* game = battle->GetGame();
	Color4F color0 = Convert_4U8_4F( game->MainPaletteColor( 0, 5 ));
	Color4F color1 = Convert_4U8_4F( game->MainPaletteColor( 0, 4 ));

	const Model* model = battle->GetModel( shooterUnit );
	if ( model ) {
		Vector3F trigger;

		Vector2I t2 = { (int)target.x, (int)target.z };
		float fireRotation  = shooterUnit->AngleBetween( t2, false );
		model->CalcTrigger( &trigger, &fireRotation );

		Ray ray = { trigger, target-trigger };
		const Model* ignore[] = { model, battle->GetWeaponModel( shooterUnit ), 0 };
		Vector3F intersection;

		Model* m = battle->GetEngine()->IntersectModel( ray, TEST_TRI, 0, 0, ignore, &intersection );
		
		if ( !m || m == targetModel || m == targetWeapon ) {
			ps->EmitBeam( trigger, target, color0 );
		}
		else {
			ps->EmitBeam( trigger, intersection, color0 );
			ps->EmitBeam( intersection, target, color1 );
		}
	}
}


void FireWidget::Hide()
{
	for( int i=0; i<MAX_FIRE_BUTTONS; ++i ) {
		fireButton[i].SetVisible( false );
	}
}


bool FireWidget::ConvertTap( const gamui::UIItem* tapped, int* mode )
{
	for( int i=0; i<MAX_FIRE_BUTTONS; ++i ) {
		if ( tapped == fireButton+i ) {
			*mode = i;
			return true;
		}
	}
	return false;
}

