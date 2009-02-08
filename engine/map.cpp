#include "platformgl.h"
#include "map.h"
#include "model.h"

void Map::Draw()
{
	model->Draw();
}


void Map::SetModel( Model* m )
{
	model = m;
	model->SetDraggable( false );
	ModelResource* res = model->GetResource();

	width = res->bounds[1].x - res->bounds[0].x;
	height = res->bounds[1].z - res->bounds[0].z;
}


