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

#ifndef UFOATTACK_BATTLESCENEDATA_INCLUDED
#define UFOATTACK_BATTLESCENEDATA_INCLUDED

class Unit;
class Storage;

class BattleSceneData : public SceneData
{
public:
	// Input to battle scene
	int	  seed;
	int   scenario;			// FARM_SCOUT, etc. Can be 0 to defer loading.
	bool  crash;			// is a crashed alien
	bool  dayTime;
	float alienRank;

	// Input/output
	Unit* soldierUnits;		// pointer to MAX_TERRAN units
	int nScientists;
	Storage* storage;
};


#endif // UFOATTACK_BATTLESCENEDATA_INCLUDED