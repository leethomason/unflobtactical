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

#include "tacticalunitscorescene.h"
#include "tacticalendscene.h"
#include "../engine/uirendering.h"
#include "../engine/engine.h"
#include "../engine/text.h"
#include "../grinliz/glrectangle.h"
#include "game.h"

using namespace grinliz;

TacticalUnitScoreScene::TacticalUnitScoreScene( Game* _game, const TacticalEndSceneData* d ) : Scene( _game )
{
	Engine* engine = GetEngine();
	data = d;
	nAwards = 0;

	engine->EnableMap( false );
	background = new UIImage( engine->GetScreenport() );

	// -- Background -- //
	Texture* bg = TextureManager::Instance()->GetTexture( "intro" );
	GLASSERT( bg );
	background->Init( bg, 480, 320 );

	//				  name		status  levelUp
	int widths[4] = { 14, 14,	10,		12		 };
	textTable = new UITextTable( engine->GetScreenport(), 4, MAX_TERRANS+1, widths );
	textTable->SetOrigin( 30, 220 );

	textTable->SetText( 0, 0, "Name" );
	textTable->SetText( 2, 0, "Status" );

	Texture* iconsTex = TextureManager::Instance()->GetTexture( "icons" );

	int count=1;
	for( int i=TERRAN_UNITS_START; i<TERRAN_UNITS_END; ++i ) {
		if ( data->units[i].Status() != Unit::STATUS_NOT_INIT ) {
			textTable->SetText( 0, count, data->units[i].FirstName() );
			textTable->SetText( 1, count, data->units[i].LastName() );
			if ( data->units[i].Status() == Unit::STATUS_ALIVE )
				textTable->SetText( 2, count, "active" );
			else
				textTable->SetText( 2, count, "KIA" );

			int kills = data->units[i].KillsCredited();
			bool purpleCircle = ( data->units[i].HP() != data->units[i].GetStats().TotalHP() && data->units[i].Status() == Unit::STATUS_ALIVE );
			int exp = kills + (purpleCircle ? 2 : 0);

			Rectangle2I rowBounds = textTable->GetRowBounds( count );
			int size = rowBounds.Height();
			Rectangle2I bounds;
			bounds.Set( rowBounds.max.x, 
						rowBounds.min.y, 
						rowBounds.max.x + size-1, 
						rowBounds.min.y + size-1 );

			if ( purpleCircle && nAwards < MAX_AWARDS ) {
				awards[nAwards] = new UIImage( engine->GetScreenport() );
				awards[nAwards]->Init( iconsTex, size, size );
				awards[nAwards]->SetOrigin( bounds.min.x, bounds.min.y );
				awards[nAwards]->SetTexCoord( 0.75f, 0.75f, 0.25f, 0.25f );

				++nAwards;
				bounds.min.x += size;
				bounds.max.x += size;
			}

			while ( kills && nAwards < MAX_AWARDS ) {
				awards[nAwards] = new UIImage( engine->GetScreenport() );
				awards[nAwards]->Init( iconsTex, size, size );
				awards[nAwards]->SetOrigin( bounds.min.x, bounds.min.y );

				if ( kills >= 10 ) {
					awards[nAwards]->SetTexCoord( 0.75f, 0.0f, 0.24f, 0.24f );
					kills -= 10;
				}
				else if ( kills >= 5 ) {
					awards[nAwards]->SetTexCoord( 0.75f, 0.25f, 0.24f, 0.24f );
					kills -= 5;
				}
				else {
					awards[nAwards]->SetTexCoord( 0.75f, 0.50f, 0.24f, 0.24f );
					kills -= 1;
				}
				++nAwards;
				bounds.min.x += size;
				bounds.max.x += size;
			}

			if ( exp > (data->units[i].GetStats().Rank()+2) && data->units[i].Status() == Unit::STATUS_ALIVE )
				textTable->SetText( 3, count, "LevelUp!" );

			++count;
		}
	}

	buttonBox = new UIButtonBox( engine->GetScreenport() );
	buttonBox->SetColumns( 1 );

	int icons[1] = { ICON_BLUE_BUTTON };

	const char* iconText[] = { "Done" };
	buttonBox->InitButtons( icons, 1 );
	buttonBox->SetOrigin( 400, 5 );
	buttonBox->SetText( iconText );
}


TacticalUnitScoreScene::~TacticalUnitScoreScene()
{
	GetEngine()->EnableMap( true );
	delete background;
	delete textTable;
	delete buttonBox;

	for( int i=0; i<nAwards; ++i ) 
		delete awards[i];
}


void TacticalUnitScoreScene::DrawHUD()
{
	background->Draw();
	textTable->Draw();

	//buttonBox->Draw();

	UFOText::Draw( 20, 25, "Game restart not yet implemented." );
	UFOText::Draw( 20, 10, "Close and select 'New' to play again." );

	for( int i=0; i<nAwards; ++i ) {
		awards[i]->Draw();
	}
}


void TacticalUnitScoreScene::Tap(	int count, 
							const grinliz::Vector2I& screen,
							const grinliz::Ray& world )
{
	int ux, uy;
	GetEngine()->GetScreenport().ViewToUI( screen.x, screen.y, &ux, &uy );
	int tap = buttonBox->QueryTap( ux, uy );
	if ( tap == 0 ) {
//		game->QueueReset();
	///	game->PopScene();
	}
}

