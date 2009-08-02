
#ifndef BATTLESCENESTREAM_INCLUDED
#define BATTLESCENESTREAM_INCLUDED

class Game;
class UFOStream;
class Unit;

class BattleSceneStream
{
public:
	BattleSceneStream( Game* _game ) : game( _game )	{}
	~BattleSceneStream() {}

	void Save(	int nUnit,
				int selectedUnit,
				const Unit* units );
	void Load(	int nUnit,
				int *selectedUnit,
				Unit* units );

	void SaveSelectedUnit( const Unit* unit );
	void LoadSelectedUnit( Unit* unit );

private:
	Game* game;
};

#endif // BATTLESCENESTREAM_INCLUDED
