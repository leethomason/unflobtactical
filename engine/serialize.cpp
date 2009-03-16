#include <stdio.h>
#include <stdlib.h>
#include "serialize.h"

void TextureHeader::Load( FILE* fp )
{
	fread( this, sizeof(TextureHeader), 1, fp );
}

void ModelHeader::Load( FILE* fp )
{
	fread( this, sizeof(ModelHeader), 1, fp );
}


void ModelGroup::Load( FILE* fp )
{
	fread( this, sizeof(ModelGroup), 1, fp );
}