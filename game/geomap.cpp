#include "geomap.h"
#include "../engine/loosequadtree.h"
#include "../engine/model.h"

GeoMap::GeoMap( SpaceTree* _tree ) : tree( _tree )
{
	scrolling = 0;
	dayNightOffset = 0;

	dayNightSurface.Set( Surface::RGB16, DAYNIGHT_TEX_SIZE, 1 );
	{
		Surface::RGBA nc = { EL_NIGHT_RED_U8, EL_NIGHT_GREEN_U8, EL_NIGHT_BLUE_U8 };
		U16 night = Surface::CalcRGB16( nc );
		for( int i=0; i<DAYNIGHT_TEX_SIZE; ++i ) {
			dayNightSurface.SetTex16( i, 0, i<DAYNIGHT_TEX_SIZE/2 ? night : 0xffff );
		}
	}

	geoModel[0] = geoModel[1] = 0;
	dayNightTex = TextureManager::Instance()->CreateTexture( "GeoDayNight", DAYNIGHT_TEX_SIZE, 1, Surface::RGB16, Texture::PARAM_NONE, this );
}


GeoMap::~GeoMap()
{
	tree->FreeModel( geoModel[0] );
	tree->FreeModel( geoModel[1] );
	TextureManager::Instance()->DeleteTexture( dayNightTex );
}


bool GeoMap::GetDayTime( float x )
{
	float night0 = -dayNightOffset*(float)MAP_X;
	float night1 = night0 + 0.5f*(float)MAP_X;

	if ( grinliz::InRange( x, night0, night1 ) || grinliz::InRange( x, night0+(float)MAP_X, night1+(float)MAP_X ) ) {
		return false;
	}
	return true;
}


void GeoMap::LightFogMapParam( float* w, float* h, float* dx, float* dy )
{
	*w = (float)MAP_X;
	*h = (float)MAP_Y;
	*dx = -scrolling + dayNightOffset;
	*dy = 0;
}


void GeoMap::CreateTexture( Texture* t )
{
	if ( t == dayNightTex ) {
		t->Upload( dayNightSurface );
	}
}


void GeoMap::DoTick( U32 currentTime, U32 deltaTime )
{
	dayNightOffset += (float)deltaTime / (40.0f*1000.0f);

	// Create geoModel as needed. (Deleted when we deactivate.)
	if ( !geoModel[0] ) {
		for( int i=0; i<2; ++i ) {
			geoModel[i] = tree->AllocModel( ModelResourceManager::Instance()->GetModelResource( "geomap" ) );
			geoModel[i]->SetFlag( Model::MODEL_OWNED_BY_MAP );
		}
		geoModel[1]->SetPos( MAP_X, 0, 0 );
	}

	if ( dayNightOffset > 1.0f ) {
		double intpart;
		dayNightOffset = (float)modf( dayNightOffset, &intpart );
	}
}


