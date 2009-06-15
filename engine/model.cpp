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

#include "model.h"
#include "surface.h"
#include "platformgl.h"
#include "loosequadtree.h"
#include "renderQueue.h"

#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glperformance.h"

#include <float.h>

using namespace grinliz;



void ModelResource::Free()
{
	for( int i=0; i<header.nGroups; ++i ) {
		glDeleteBuffers( 1, (const GLuint*) &atom[i].indexID );		
		glDeleteBuffers( 1, (const GLuint*) &atom[i].vertexID );
		memset( &atom[i], 0, sizeof( ModelAtom ) );
		CHECK_GL_ERROR;
	}
	delete [] allVertex;
	delete [] allIndex;
}



int ModelResource::Intersect(	const grinliz::Vector3F& point,
								const grinliz::Vector3F& dir,
								grinliz::Vector3F* intersect )
{
	float t;
	if ( IntersectRayAABB( point, dir, header.bounds, intersect, &t ) == grinliz::INTERSECT ) {
		float close2 = FLT_MAX;
		Vector3F test;

		for( int i=0; i<header.nGroups; ++i ) {
			for( unsigned j=0; j<atom[i].nIndex; j+=3 ) {
				int r = IntersectRayTri( point, dir, 
										 atom[i].vertex[ atom[i].index[j+0] ].pos,
										 atom[i].vertex[ atom[i].index[j+1] ].pos,
										 atom[i].vertex[ atom[i].index[j+2] ].pos,
										 &test );
				if ( r == grinliz::INTERSECT ) {
					float c2 =  (point.x-test.x)*(point.x-test.x) +
								(point.y-test.y)*(point.y-test.y) +
								(point.z-test.z)*(point.z-test.z);
					if ( c2 < close2 ) {
						close2 = c2;
						*intersect = test;
					}
				}
			}
		}
		if ( close2 < FLT_MAX ) {
			return grinliz::INTERSECT;
		}
	}
	return grinliz::REJECT;
}


void ModelLoader::Load( FILE* fp, ModelResource* res )
{
	res->header.Load( fp );

	// Compute the bounding sphere. The model is always rotated around the
	// Y axis, so be sure to put the sphere there, else it won't be invariant
	// to rotation. Then check the 8 corners for the furthest.
	//res->boundSphere.origin.x = 0;
	//res->boundSphere.origin.y = ( res->header.bounds.min.y + res->header.bounds.max.y )*0.5f;
	//res->boundSphere.origin.z = 0;

	/*float longestR2 = 0.0f;

	for( int k=0; k<2; ++k ) {
		for( int j=0; j<2; ++j ) {
			for( int i=0; i<2; ++i ) {
				float dx = res->header.bounds.Vec(i).x - res->boundSphere.origin.x;
				float dy = res->header.bounds.Vec(j).y - res->boundSphere.origin.y;
				float dz = res->header.bounds.Vec(k).z - res->boundSphere.origin.z;
				float d2 = dx*dx + dy*dy + dz*dz;

				if ( d2 > longestR2 ) {
					longestR2 = d2;
				}
			}
		}
	}
	res->boundSphere.radius = sqrtf( longestR2 );
*/
	// And the bounding circle.
/*	longestR2 = 0;

	for( int k=0; k<2; ++k ) {
		for( int i=0; i<2; ++i ) {
			float dx = res->header.bounds.Vec(i).x - res->boundSphere.origin.x;
			float dz = res->header.bounds.Vec(k).z - res->boundSphere.origin.z;
			float d2 = dx*dx + dz*dz;

			if ( d2 > longestR2 ) {
				longestR2 = d2;
			}
		}
	}
	res->boundRadius2D = sqrtf( longestR2 );
*/
	// compute the hit testing AABB
	float ave = grinliz::Max( res->header.bounds.SizeX(), res->header.bounds.SizeZ() )*0.5f;
	//float ave = Max( res->header.bounds.SizeX(), res->header.bounds.SizeZ() );
	res->hitBounds.min.Set( -ave, res->header.bounds.min.y, -ave );
	res->hitBounds.max.Set( ave, res->header.bounds.max.y, ave );

	//GLASSERT( nTotalVertices <= EL_MAX_VERTEX_IN_GROUP );
	//GLASSERT( nTotalIndices <= EL_MAX_INDEX_IN_GROUP );

	GLOUTPUT(( "Load Model: %s\n", res->header.name ));
	/*
	GLOUTPUT(( "  sphere (%.2f,%.2f,%.2f) rad=%.2f  hit(%.2f,%.2f,%.2f)-(%.2f,%.2f,%.2f)\n",
				(float)res->boundSphere.origin.x,
				(float)res->boundSphere.origin.y,
				(float)res->boundSphere.origin.z,
				(float)res->boundSphere.radius,
				(float)res->hitBounds.min.x,
				(float)res->hitBounds.min.y,
				(float)res->hitBounds.min.z,
				(float)res->hitBounds.max.x,
				(float)res->hitBounds.max.y,
				(float)res->hitBounds.max.z ));
	*/

	for( U32 i=0; i<res->header.nGroups; ++i )
	{
		ModelGroup group;
		group.Load( fp );

		const Texture* t = 0;
		const char* textureName = group.textureName;
		if ( !textureName[0] ) {
			textureName = "white";
		}
		
		// Remove extension.
		std::string base, name, extension;
		StrSplitFilename( std::string( textureName ), &base, &name, &extension );

		for( int j=0; j<nTextures; ++j ) {
			if ( strcmp( name.c_str(), texture[j].name ) == 0 ) {
				t = &texture[j];
				break;
			}
		}
		GLASSERT( t );                       
		res->atom[i].texture = t;

		res->atom[i].nVertex = group.nVertex;
		res->atom[i].nIndex = group.nIndex;

		GLOUTPUT(( "  '%s' vertices=%d tris=%d\n", textureName, res->atom[i].nVertex, res->atom[i].nIndex/3 ));
	}

	res->allVertex = new Vertex[ res->header.nTotalVertices ];
	res->allIndex  = new U16[ res->header.nTotalIndices ];

	size_t r = fread( res->allVertex, sizeof(Vertex), res->header.nTotalVertices, fp );
	(void)r;
	GLASSERT( r == res->header.nTotalVertices );
	GLASSERT( sizeof(Vertex) == sizeof(U32)*8 );
	fread( res->allIndex, sizeof(U16), res->header.nTotalIndices, fp );

#ifdef DEBUG
	/*
	for( U32 i=0; i<nTotalIndices; i+=3 ) {
		GLOUTPUT(( "%d %d %d\n", index[i+0], index[i+1], index[i+2] ));
	}
	*/
#endif
	

	// Load to VBO
	int vOffset = 0;
	int iOffset = 0;
	for( U32 i=0; i<res->header.nGroups; ++i )
	{
		// Index buffer
		glGenBuffers( 1, (GLuint*) &res->atom[i].indexID );
		glGenBuffers( 1, (GLuint*) &res->atom[i].vertexID );
		CHECK_GL_ERROR
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, res->atom[i].indexID );
		glBindBuffer( GL_ARRAY_BUFFER, res->atom[i].vertexID );
		CHECK_GL_ERROR
		U32 indexSize = sizeof(U16)*res->atom[i].nIndex;
		U32 dataSize  = sizeof(Vertex)*res->atom[i].nVertex;

		res->atom[i].index  = res->allIndex+iOffset;
		res->atom[i].vertex = res->allVertex+vOffset;

		glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, res->atom[i].index, GL_STATIC_DRAW );
		glBufferData( GL_ARRAY_BUFFER, dataSize, res->atom[i].vertex, GL_STATIC_DRAW );
		CHECK_GL_ERROR

		iOffset += res->atom[i].nIndex;
		vOffset += res->atom[i].nVertex;
	}
	// unbind
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}


void Model::Init( ModelResource* resource, SpaceTree* tree )
{
	this->resource = resource; 
	this->tree = tree;
	stats = 0;
	pos.Set( 0, 0, 0 );
	rot = 0;
	texMatSet = false;
	setTexture = 0;
	Modify();

	if ( tree ) {
		tree->Update( this );
	}

	//xformValid = false;
	flags = 0;
}


void Model::SetPos( const grinliz::Vector3F& pos )
{ 
	if ( pos != this->pos ) {
		Modify();
		this->pos = pos;	
		tree->Update( this ); 
	}
}


void Model::SetYRotation( float rot )
{
	while( rot < 0 )		{ rot += 360.0f; }
	while( rot >= 360 )		{ rot -= 360.0f; }

	if ( rot != this->rot ) {
		Modify();
		this->rot = rot;		
		tree->Update( this );	// call because bound computation changes with rotation
	}
}


void Model::SetSkin( int armor, int skin, int hair )
{
	texMatSet = false;
	if ( armor > 0 || skin > 0 || hair > 0 ) {
	
		float textureOffsetX = 0.0f;

		armor = Clamp( armor, 0, 3 );
		skin = Clamp( skin, 0, 3 );
		hair = Clamp( hair, 0, 3 );

		textureOffsetX += float( armor ) / float( 4 );
		textureOffsetX += float( skin ) / float( 16 );
		textureOffsetX += float( hair ) / float( 64 );

		texMat.Identity();
		texMat.x = textureOffsetX;
		texMatSet = true;
	}
}


void Model::SetTexXForm( float a, float d, float x, float y )
{
	texMat.Identity();
	texMatSet = false;
	if ( a!=1.0f || d!=1.0f || x!=0.0f || y!=0.0f ){
		texMat.a = a;
		texMat.d = d;
		texMat.x = x;
		texMat.y = y;
		texMatSet = true;
	}
}



/*
void Model::CalcBoundSphere( Sphere* sphere ) const
{
	*sphere = resource->boundSphere;
	sphere->origin += pos;
}
*/

/*
void Model::CalcBoundCircle( Circle* circle ) const 
{
	circle->origin.x = pos.x;
	circle->origin.y = pos.z;
	circle->radius = resource->boundRadius2D;
}
*/


void Model::CalcHitAABB( Rectangle3F* aabb ) const
{
	// This is already an approximation - ignore rotation.
	aabb->min = pos + resource->hitBounds.min;
	aabb->max = pos + resource->hitBounds.max;
}


void Model::CalcTrigger( grinliz::Vector3F* trigger ) const
{
	const Matrix4& xform = XForm();
	*trigger = xform * resource->header.trigger;
}


void Model::CalcTarget( grinliz::Vector3F* target ) const
{
	const Matrix4& xform = XForm();
	Vector3F t = { 0, resource->header.target, 0 };
	*target = xform * t;
}


/*
bool Model::CalcAABB( grinliz::Rectangle3F* aabb ) const
{
	const Vector3F& a = resource->header.bounds.max;
	const Vector3F& b = resource->header.bounds.min;
	Vector3F min = { 0, 0, 0 };
	Vector3F max = { 0, 0, 0 };
	bool result = false;

	if ( rot == 0.0f ) {
		// x' = x
		// z' = z
		min = b;
		max = a;
		result = true;
	}
	else if ( rot == 90.0f ) {
		// x' = z
		// z' = -x;
		min.x = b.z;
		min.z = -a.x;
		max.x = a.z;
		max.z = -b.x;
		result = true;
	}
	else if ( rot == 180.0f ) {
		// x' = -x
		// z' = -z
		min.x = -a.x;
		min.z = -a.z;
		max.x = -b.x;
		max.z = -b.z;
		result = true;
	}
	else if ( rot == 270.0f ) {
		// x' = -z
		// z' = x
		min.x = -a.z;
		min.z = b.x;
		max.x = -b.z;
		max.z = a.x;
		result = true;
	}
	if ( result ) {
		GLASSERT( min.x <= max.x );
		GLASSERT( min.y <= max.y );
		GLASSERT( min.z <= max.z );
		aabb->min = pos + min;
		aabb->max = pos + max;
	}
	return result;
}
*/


void Model::Queue( RenderQueue* queue, int textureMode )
{
	for( U32 i=0; i<resource->header.nGroups; ++i ) 
	{
		int flags = 0;
		const Texture* texture = setTexture ? setTexture : resource->atom[i].texture;

		if ( textureMode != Model::NO_TEXTURE ) {
			if ( texture->alphaTest ) 
				flags |= RenderQueue::ALPHA_BLEND;
		}
		U32 textureID = (textureMode == Model::MODEL_TEXTURE) ? texture->glID : 0;

		queue->Add( flags, 
					textureID,
					this, 
					&resource->atom[i] );
	}
}


void ModelAtom::Bind( /*bool bindTextureToVertex*/ ) const
{
	glBindBuffer( GL_ARRAY_BUFFER, vertexID );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexID );

	glVertexPointer(   3, GL_FLOAT, sizeof(Vertex), (const GLvoid*)Vertex::POS_OFFSET);			// last param is offset, not ptr
	glNormalPointer(      GL_FLOAT, sizeof(Vertex), (const GLvoid*)Vertex::NORMAL_OFFSET);		

//	if ( bindTextureToVertex ) {
//		for( int i=0; i<2; ++i ) {
//			glClientActiveTexture( GL_TEXTURE0+i );
//			glTexCoordPointer( 3, GL_FLOAT, sizeof(Vertex), (const GLvoid*)Vertex::POS_OFFSET);  
//		}
//	}
//	else {
		glTexCoordPointer( 2, GL_FLOAT, sizeof(Vertex), (const GLvoid*)Vertex::TEXTURE_OFFSET);  
//	}
	CHECK_GL_ERROR;
}


void ModelAtom::Draw() const
{
	GRINLIZ_PERFTRACK
	// mode, count, type, indices
	glDrawElements( GL_TRIANGLES, 
					nIndex,
					GL_UNSIGNED_SHORT, 
					0 );
	CHECK_GL_ERROR;
	trisRendered += nIndex / 3;
}


void Model::PushMatrix( /*bool bindTextureToVertex*/ ) const
{
#if 0
	if ( !xformValid ) {
		Matrix4 t, r;
		t.SetTranslation( pos.x, pos.y, pos.z );
		if ( rot != 0 )
			r.SetYRotation( rot );
		xform = t*r;
		xformValid = true;
	}
	glPushMatrix();
	glMultMatrixf( xform.x );
#else
	glPushMatrix();
	//glTranslatef( pos.x, pos.y, pos.z );
	//if ( rot != 0 ) {
	//	glRotatef( rot, 0.f, 1.f, 0.f );
	//}
	const Matrix4& xform = XForm();
	glMultMatrixf( xform.x );

#endif

#if 0
	if ( bindTextureToVertex ) {
		/*
		// This code is correct, if too slow. But good reference!
		Vector3F lightDirection = { 0.7f, 3.0f, 1.4f };
		lightDirection.Normalize();
		Matrix4 light, r, t, mat;

		light.m12 = -lightDirection.x/lightDirection.y;
		light.m22 = 0.0f;
		light.m32 = -lightDirection.z/lightDirection.y;
		
		r.SetYRotation( rot );
		t.SetTranslation( pos.x, pos.y, pos.z );

		mat = (*textureMat)*light*t*r;

		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		
		glMultMatrixf( mat.x );
		*/
		glMatrixMode(GL_TEXTURE);
		for( int i=1; i>=0; --i ) {
			glActiveTexture( GL_TEXTURE0 + i );
			glPushMatrix();

			glTranslatef( pos.x, pos.y, pos.z );
			if ( rot != 0 ) {
				glRotatef( rot, 0.f, 1.f, 0.f );
			} 
		}
	}
	else 
#endif
		
	if ( texMatSet ) {
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		
		float m[16] = {
			texMat.a, 0,	0,	0,		// column 1
			0,	texMat.d, 0, 0,
			0, 0, 0, 0,
			texMat.x, texMat.y, 0, 1
		};
		glMultMatrixf( m );
	}
	CHECK_GL_ERROR;
}


void Model::PopMatrix( /*bool bindTextureToVertex*/ ) const
{
	if ( texMatSet ) {
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
	CHECK_GL_ERROR;
}


const grinliz::Rectangle3F& Model::AABB() const
{
	XForm();	// just makes sure the cache is good.
	return aabb;
}


const grinliz::Matrix4& Model::XForm() const
{
	if ( !xformValid ) {
		Matrix4 t;
		t.SetTranslation( pos );

		Matrix4 r;
		r.SetYRotation( rot );

		_xform = t*r;

		// compute the AABB. Take advatage that we only have translation and y-rotation.
		Vector3F p[4], q;
		const Rectangle3F& resB = resource->header.bounds;
		float y = resB.min.y;
		p[0] = resB.min;
		p[1].Set( resB.max.x, y, resB.min.z );
		p[2] = resB.max;
		p[3].Set( resB.min.x, y, resB.max.z );

		aabb.max = aabb.min = _xform * p[0];
		for( int i=1; i<4; ++i ) {
			q = _xform*p[i];
			aabb.DoUnion( q );
		}
		
		xformValid = true;
	}
	return _xform;
}


const grinliz::Matrix4& Model::InvXForm() const
{
	if ( !invValid ) {
		const Matrix4& xform = XForm();

		const Vector3F& u = *((const Vector3F*)&xform.x[0]);
		const Vector3F& v = *((const Vector3F*)&xform.x[4]);
		const Vector3F& w = *((const Vector3F*)&xform.x[8]);

		_invXForm.m11 = u.x;	_invXForm.m12 = u.y;	_invXForm.m13 = u.z;	_invXForm.m14 = -DotProduct( u, pos );
		_invXForm.m21 = v.x;	_invXForm.m22 = v.y;	_invXForm.m23 = v.z;	_invXForm.m24 = -DotProduct( v, pos );
		_invXForm.m31 = w.x;	_invXForm.m32 = w.y;	_invXForm.m33 = w.z;	_invXForm.m34 = -DotProduct( w, pos );
		_invXForm.m41 = 0;		_invXForm.m42 = 0;		_invXForm.m43 = 0;		_invXForm.m44 = 1;

		invValid = true;
	}
	return _invXForm;
}


// TestHitTest(PC)
// 1.4 fps (eek!)
// sphere test: 4.4
// tweaking: 4.5
// AABB testing in model: 8.0
// cache the xform: 8.4
// goal: 30. Better but ouchie.

int Model::IntersectRay(	const Vector3F& _origin, 
							const Vector3F& _dir,
							Vector3F* intersect ) const
{
	Vector4F origin = { _origin.x, _origin.y, _origin.z, 1.0f };
	Vector4F dir    = { _dir.x, _dir.y, _dir.z, 0.0f };
	int result = grinliz::REJECT;

	//Sphere sphere;
	//CalcBoundSphere( &sphere );
	//int initTest = IntersectRaySphere( sphere, _origin, _dir );
	Vector3F dv;
	float dt;
	int initTest = IntersectRayAABB( _origin, _dir, AABB(), &dv, &dt );

	if ( initTest == grinliz::INTERSECT || initTest == grinliz::INSIDE )
	{
		const Matrix4& inv = InvXForm();

		Vector4F objOrigin4 = inv * origin;
		Vector4F objDir4    = inv * dir;

		Vector3F objOrigin = { objOrigin4.x, objOrigin4.y, objOrigin4.z };
		Vector3F objDir    = { objDir4.x, objDir4.y, objDir4.z };
		Vector3F objIntersect;

		result = resource->Intersect( objOrigin, objDir, &objIntersect );
		if ( result == grinliz::INTERSECT ) {
			// Back to this coordinate system. What a pain.
			const Matrix4& xform = XForm();

			Vector4F objIntersect4 = { objIntersect.x, objIntersect.y, objIntersect.z, 1.0f };
			Vector4F intersect4 = xform*objIntersect4;
			intersect->Set( intersect4.x, intersect4.y, intersect4.z );
		}
	}
	return result;
}
