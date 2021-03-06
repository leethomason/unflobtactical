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

#pragma warning ( disable : 4530 )		// Don't warn about unused exceptions.

#include "import.h"
extern "C" {
	#include "ac3d.h"
}

using namespace grinliz;

void ProcessAC3D( ACObject* ob, ModelBuilder* builder, const Matrix4& parent, const char* groupFilter )
{
	Matrix4 m, matrix;
	m.SetTranslation( ob->loc.x, ob->loc.y, ob->loc.z );

	MultMatrix4( parent, m, &matrix );
	builder->SetMatrix( matrix );
	builder->SetTexture( ob->textureName ? ob->textureName : "" );

	Vertex vertex[16];

	for( int i = 0; i < ob->num_surf; ++i )
	{
		ACSurface *surf = &ob->surfaces[i];
		Vector3F normal = { surf->normal.x, surf->normal.y, surf->normal.z };
		if ( normal.Length() == 0.f ) {

			GLASSERT( 0 );
			normal.Set( 0.0f, 1.0f, 0.0f );
		}

		int st = surf->flags & 0xf;
		if ( st == SURFACE_TYPE_POLYGON && surf->num_vertref<16 )		// hopefully convex and simple
		{
			for( int j=0; j<surf->num_vertref; ++j )
			{
			    ACVertex *v = &ob->vertices[surf->vertref[j]];
				float tx = 0, ty = 0;
	
				if (ob->texture )
				{
					float tu = surf->uvs[j].u;
					float tv = surf->uvs[j].v;

					tx = ob->texture_offset_x + tu * ob->texture_repeat_x;
					ty = ob->texture_offset_y + tv * ob->texture_repeat_y;
				}
				GLASSERT( j<16 );
				vertex[j].pos.Set( v->x, v->y, v->z );
				vertex[j].normal = normal;
				vertex[j].tex.Set( tx, ty );
			}
			int numTri = surf->num_vertref-2;
			for( int j=0; j<numTri; ++j ) {
				builder->AddTri( vertex[0], vertex[j+1], vertex[j+2] );
			}
	    }
		else
		{
			GLOUTPUT(( "Input polygon or type not supported.\n" ));
			GLASSERT( 0 );
		}
	}
	for ( int n = 0; n < ob->num_kids; n++) {
		if ( groupFilter ) {
			// Sleazy trick to only look at the top node. Set to node
			// after we find the top.
			if ( StrEqual( ob->kids[n]->name, groupFilter ) )
			    ProcessAC3D(ob->kids[n], builder, matrix, 0 ); 
		}
		else {
		    ProcessAC3D(ob->kids[n], builder, matrix, 0 ); 
		}
	}
}


bool ImportAC3D(	const std::string& filename, 
					ModelBuilder* builder, 
					const grinliz::Vector3F origin,
					const std::string& group )
{
	ACObject* acObject = ac_load_ac3d( (char*) filename.c_str() );
	GLASSERT( acObject );
	if ( acObject )
	{
		// Kind of a wicked recursive thing.
		Matrix4 matrix;
		matrix.SetTranslation( -origin );
		ProcessAC3D( acObject, builder, matrix, group.empty() ? 0 : group.c_str() );	
	}
	builder->Flush();

	bool importOk = ( acObject != 0 );
	if ( acObject ) 
		ac_object_free( acObject );
	return importOk;
}
