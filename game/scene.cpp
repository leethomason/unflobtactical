#include "game.h"
#include "scene.h"

Engine* Scene::GetEngine()
{
	return &game->engine;
}
