/*
Copyright (c) 2000-2007 Lee Thomason (www.grinninglizard.com)
Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#include "glgeometry.h"
#include "glmath.h"

using namespace grinliz;


void SphericalXZ::FromCartesion( const Vector3F& in )
{
	theta = rho = 0.0f;
	r = in.Length();

	if ( r != 0.0f ) {
		rho = acosf( in.y / r );
	}
	if ( in.z != 0.0f ) {
		theta = atanf( -in.x / in.z );
	}
}


void SphericalXZ::ToCartesian( Vector3F *out )
{
	out->x = r * cosf( ToRadian( rho ) ) * cosf( ToRadian( theta ) );
	out->y = r * sinf( ToRadian( rho ) );
	out->z = -r * cosf( ToRadian( rho ) ) * sinf( ToRadian( theta ) );
}


void SphericalXZ::ToCartesian( Matrix4* out )
{
	Matrix4 roty;	// theta
	Matrix4 rotz;	// rho
	Matrix4 trans;

	roty.SetYRotation( theta );
	rotz.SetZRotation( rho );
	trans.SetTranslation( r, 0.f, 0.f );

	*out = roty * rotz * trans;
}


void grinliz::MinDeltaDegrees( float angle0, float angle1, float* distance, float* bias )
{
	float delta = angle1 - angle0;

	if ( delta >= 0.0f && delta <= 180.0f )
	{
		*distance = delta;
		*bias = 1.0f;
	}
	else if ( delta > 180.0f )
	{
		*distance = 360.0f - delta;
		*bias = -1.0f;
	}
	else if ( delta < 0.0f && delta >= -180.0f )
	{
		*distance = -delta;
		*bias = -1.0f;
	}
	else
	{
		*distance = 360.0f + delta;
		*bias = 1.0f;
	}
}


/*static*/ void Plane::CreatePlane( const Vector3F* vertex, Plane* plane )
{
	Vector3F a = vertex[1] - vertex[0];
	Vector3F b = vertex[2] - vertex[0];

	CrossProduct( a, b, &plane->n );
	plane->n.Normalize();

	// n dot p + d = 0
	plane->d = -DotProduct( vertex[0], plane->n );

	#ifdef DEBUG
	if ( plane->n.z > 0.1f )
	{
		GLASSERT( Equal( vertex[0].z, plane->Z( vertex[0].x, vertex[0].y ), 0.001f ) );
		GLASSERT( Equal( vertex[1].z, plane->Z( vertex[1].x, vertex[1].y ), 0.001f ) );
		GLASSERT( Equal( vertex[2].z, plane->Z( vertex[2].x, vertex[2].y ), 0.001f ) );
	}
	#endif
}


/*static*/ void Plane::CreatePlanes( const Rectangle3F& r, Plane* planes )
{
	Vector3F normal, point;

	normal.Set( -1, 0, 0 );
	point = r.pos + r.size;
	CreatePlane( normal, point, &planes[0] );

	normal.Set( 1, 0, 0 );
	point = r.pos;
	CreatePlane( normal, point, &planes[1] );

	normal.Set( 0, -1, 0 );
	point = r.pos + r.size;
	CreatePlane( normal, point, &planes[2] );

	normal.Set( 0, 1, 0 );
	point = r.pos;
	CreatePlane( normal, point, &planes[3] );

	normal.Set( 0, 0, -1 );
	point = r.pos + r.size;
	CreatePlane( normal, point, &planes[4] );

	normal.Set( 0, 0, 1 );
	point = r.pos;
	CreatePlane( normal, point, &planes[5] );
}


/*static*/ void Plane::CreatePlane( const Vector3F& normal, const Vector3F& point, Plane* plane )
{
	// n * p + d = 0
	// so: d = -n*p

	GLASSERT( Equal( normal.Length(), 1.0f, 0.001f ) );
	plane->n = normal;
	plane->d = -DotProduct( normal, point );
}


void Plane::ProjectToPlane( const Vector3F& _vector, Vector3F* projected ) const
{
	// A || B = B x (Ax B / |B|) / |B|
	Vector3F vector = _vector;
	vector.Normalize();

	Vector3F AcB;
	CrossProduct( vector, n, &AcB );
	CrossProduct( n, AcB, projected );
	projected->Normalize();


	/*
	GLASSERT( Equal( n.Length(), 1.0f, EPSILON ) );

	Vector3F vector = _vector;
	vector.Normalize();

	Vector3F pn;	
	CrossProduct( n, vector, &pn );
	CrossProduct( pn, n, projected );
	*/
	
	#ifdef DEBUG
	{
		float len = projected->Length();
		GLASSERT( Equal( len, 1.0f, EPSILON ) );

		Vector3F test = { 0.0f, 0.0f, Z( 0.0f, 0.0f ) };
		test = test + (*projected);
		float z = Z( projected->x, projected->y );
		GLASSERT( Equal( test.z, z, 0.0001f ) );
	}
	#endif
}


void LineLoop::Clear() 
{
	while( first ) {
		Delete( first );
	}
}

void LineLoop::Delete( LineNode* del )
{
	if ( first == del ) {
		if ( first->next != first )
			first = first->next;
		else
			first = 0;
	}

	del->next->prev = del->prev;
	del->prev->next = del->next;
	delete del;
}

void LineLoop::AddAtEnd( LineNode* node )
{
	if ( first ) {
		node->prev = first->prev;
		node->next = first;
		first->prev->next = node;
		first->prev = node;
	}
	else {
		node->next = node;
		node->prev = node;
		first = node;
	}
}

void LineLoop::AddAfter( LineNode* me, LineNode* add )
{
	add->prev = me;
	add->next = me->next;
	me->next->prev = add;
	me->next = add;
}


void LineLoop::SortToTop()
{
	if ( first ) {
		LineNode* newFirst = first;
		LineNode* node = first->next;
		do {
			if ( node->point.y < newFirst->point.y )
				newFirst = node;
			else if ( node->point.y == newFirst->point.y && node->point.x < newFirst->point.x )
				newFirst = node;

			node=node->next;
		} while ( node != first );

		first = newFirst;
	}
}


void LineLoop::Bounds( Rectangle2F* bounds )
{
	if ( first ) {
		bounds->Set( first->point );

		for( LineNode* node = first->next; node != first; node = node->next )
		{
			bounds->DoUnion( node->point );
		}
	}
}


void LineLoop::Render( float* surface, int width, int height, bool fill )
{
	if ( First() ) 
	{
		Rectangle2F lineBounds;
		Bounds( &lineBounds );
		SortToTop();

		int yStart = Max( (int)ceilf( lineBounds.Y0() ), 0 );
		int yEnd   = Min( (int)floorf( lineBounds.Y1() ), height-1 );

		LineNode *left0	 = First();
		LineNode *left1	 = left0->prev;			// left goes prev
		LineNode *right0 = First();
		LineNode *right1 = right0->next;		// right goes next

		// Deal with a horizontal starting line:
		if ( right1->point.y == right0->point.y ) {
			right0 = right1;
			right1 = right1->next;
		}

		bool done = false;
		int y;
		if ( fill ) {
			for( y=0; y<yStart; ++y ) {
				float* row = surface + y*width;
				for( int x=0; x<width; ++x ) {
					row[x] = left0->value;
				}
			}
		}
		for( y=yStart; y<=yEnd && !done; ++y )
		{
			// Make sure we are in range on the left and right edges.
			float yF = (float)y;
			while ( yF > left1->point.y ) {
				// Are we done?
				if ( left1->prev->point.y <= left1->point.y ) {
					// both left and right should meet at same point or line.
					done = true;
					break;
				}
				left0 = left1;
				left1 = left1->prev;
			}
			if ( done )
				break;
			while ( yF > right1->point.y ) {
				GLASSERT( right1->next->point.y >= right1->point.y );
				right0 = right1;
				right1 = right1->next;
			}
			// interp( y, x, ... )
			float x0F = left0->point.x;
			if ( left0->point.y != left1->point.y ) {
				x0F = Interpolate(	left0->point.y, left0->point.x, 
									left1->point.y, left1->point.x,
									yF );
			}

			float x1F = right0->point.x;
			if ( right0->point.y != right1->point.y ) {
				x1F = Interpolate(	right0->point.y, right0->point.x, 
									right1->point.y, right1->point.x,
									yF );
			}

			GLASSERT( x0F <= x1F );
			int x0 = (int) (x0F);
			int x1 = (int) (x1F);
			GLASSERT( x0 <= x1 );

			float v0 = left0->value;
			if ( left0->point.y != left1->point.y )
				v0 = Interpolate( left0->point.y, left0->value,
								  left1->point.y, left1->value,
								  yF );
			float v1 = right0->value;
			if ( right0->point.y != right1->point.y )
				v1 = Interpolate( right0->point.y, right0->value,
								  right1->point.y, right1->value,
								  yF );
			float dv = 0.0f;
			float v = v0;

			if ( x1 > x0 )
				dv = ( v1-v0 ) / (float)(x1-x0);
			else
				v = Mean( v0, v1 );

			int x = x0;
			while ( x < 0 ) {
				++x;
				v += dv;
			}

			float* row = surface + y*width;

			if ( fill ) {
				int xFill;
				for( xFill=0; xFill<x0; ++xFill )
					row[xFill] = v0;
				for( xFill=x1+1; xFill<width; ++xFill )
					row[xFill] = v1;
			}

			for( ; x<=x1; ++x, v += dv )
			{
				row[x] = v;
			}
		}
		if ( fill ) {
			for( ; y<height; ++y ) {
				float* row = surface + y*width;
				for( int x=0; x<width; ++x ) {
					row[x] = left1->value;
				}
			}
		}
	}
}


#ifdef DEBUG
void LineLoop::Dump()
{
	GLOUTPUT(( "LineLoop: " ));
	if ( first ) {
		LineNode* node = first;
		do {
			GLOUTPUT(( "(%.2f,%.2f) ", node->point.x, node->point.y ));
			node = node->next;
		} while ( node != first );
	}
	GLOUTPUT(( "\n" ));
}
#endif


float grinliz::CalcSphereTexture( float x, float y, bool high )
{
	float theta = atan2f( y, x );
	if ( theta < 0.0f ) 
		theta += 2.0f*PI;
	if ( theta < 0.0001f && high )
		return 1.0f;
	return theta / (2.0f*PI);
}

#ifndef ANDROID_NDK
// http://student.ulb.ac.be/~claugero/sphere/index.html
//
void grinliz::TessellateSphere( int iterations, float radius, bool innerNormals,
								std::vector< Vector3F >* _vertex, 
								std::vector< U32 >* _index,
								std::vector< Vector3F >* _normal,
								std::vector< Vector2F >* _texture,
								TessellateSphereMode textureMode )
{
	GLASSERT( _vertex );
	GLASSERT( _index );
	_vertex->resize( 0 );
	_index->resize( 0 );
	if ( _normal ) _normal->resize( 0 );
	if ( _texture ) _texture->resize( 0 );

	// Non-const references as parameters should be banned from the language. Convert
	// here to reduce typing.
	std::vector< Vector3F >& vertex = *_vertex;
	std::vector< U32 >& index       = *_index;

	// Start with a octahedron
	// If not using texture, the indices can map to multiple vertices.
	// Texturing complicates things, and each triagle has to subdivide, and use a unique index.
	if ( !_texture )
	{
		vertex.resize( 6 );

		vertex[0].Set( 1.0f, 0.0f, 0.0f );
		vertex[1].Set( 0.0f, 1.0f, 0.0f );
		vertex[2].Set( -1.0f, 0.0f, 0.0f );
		vertex[3].Set( 0.0f, -1.0f, 0.0f );
		vertex[4].Set( 0.0f, 0.0f, 1.0f );
		vertex[5].Set( 0.0f, 0.0f, -1.0f );

		index.resize(3*8);
		unsigned i=0;
		index[i++] = 0;		index[i++] = 1;		index[i++] = 4;
		index[i++] = 1;		index[i++] = 2;		index[i++] = 4;
		index[i++] = 2;		index[i++] = 3;		index[i++] = 4;
		index[i++] = 3;		index[i++] = 0;		index[i++] = 4;

		index[i++] = 1;		index[i++] = 0;		index[i++] = 5;
		index[i++] = 2;		index[i++] = 1;		index[i++] = 5;
		index[i++] = 3;		index[i++] = 2;		index[i++] = 5;
		index[i++] = 0;		index[i++] = 3;		index[i++] = 5;
	}
	else {
		//std::vector< Vector2F >& texture = *_texture;
		vertex.resize(24);

		float xVal[] = { 1.0f, 0.0f, -1.0f, 0.0f, 1.0f };
		float yVal[] = { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f };

		int i;
		for( i=0; i<12; i+=3 ) 
		{
			vertex[i+0].Set( xVal[i/3],   yVal[i/3], 0.0f );
			vertex[i+1].Set( xVal[i/3+1], yVal[i/3+1], 0.0f );
			vertex[i+2].Set( 0.0f, 0.0f, 1.0f );
		}
		for( i=12; i<24; i+=3 ) 
		{
			vertex[i+0].Set( xVal[i/3-4+1], yVal[i/3-4+1], 0.0f );
			vertex[i+1].Set( xVal[i/3-4],   yVal[i/3-4],   0.0f );
			vertex[i+2].Set( 0.0f, 0.0f, -1.0f );
		}		
		index.resize( 24 );
		for( i=0; i<24; ++i ) 
		{
			index[i] = i;
		}
	}

	// After all that setup, subdivide.
	for( int k=0; k<iterations; ++k ) 
	{
		std::vector< U32 > indexSource = index;
		index.resize( 0 );
		GLASSERT( indexSource.size() % 3 == 0 );

		for( unsigned i=0; i<indexSource.size(); i+=3 ) 
		{								
			int i0 = indexSource[i+0];
			int i1 = indexSource[i+1];
			int i2 = indexSource[i+2];

			const Vector3F& p0 = vertex[ i0 ];
			const Vector3F& p1 = vertex[ i1 ];
			const Vector3F& p2 = vertex[ i2 ];

			Vector3F p01 = p0 + p1;		p01.Normalize();
			Vector3F p12 = p1 + p2;		p12.Normalize();
			Vector3F p20 = p2 + p0;		p20.Normalize();

			// Add the new vertex:
			unsigned i01 = (unsigned)vertex.size();	vertex.push_back( p01 );
			unsigned i12 = (unsigned)vertex.size();	vertex.push_back( p12 );
			unsigned i20 = (unsigned)vertex.size();	vertex.push_back( p20 );

			// 0-01-20
			// 01-1-12
			// 01-12-20
			// 20-12-2
			index.push_back( i0 );	index.push_back( i01 );	index.push_back( i20 );
			index.push_back( i01 );	index.push_back( i1 );	index.push_back( i12 );
			index.push_back( i01 );	index.push_back( i12 );	index.push_back( i20 );
			index.push_back( i20 );	index.push_back( i12 );	index.push_back( i2 );
		}
	}

	// The normals are just the vertices before the radius is applied.
	if ( _normal ) {
		*_normal = vertex;
	}

	// Patch up the textures
	if ( _texture ) {
		std::vector< Vector2F >& texture = *_texture;
		texture.resize( vertex.size() );

		for( unsigned i=0; i<texture.size(); ++i )
		{
			// The southern hem uses 1.0 tex rather than 0.0
			bool high = false;
			if (    i > 3 
				 && ( vertex[i].y < 0.0f || vertex[i-1].y < 0.0f || vertex[i-2].y < 0.0f ) )
			{
				high = true;
			}

			if ( vertex[i].x == 0.0f && vertex[i].y == 0.0f ) {
				GLASSERT( i >= 2 );
				texture[i].Set( Mean( texture[i-1].x, texture[i-2].x ), 
					            vertex[i].z );
			} 
			else {
				texture[i].Set( CalcSphereTexture( vertex[i].x, vertex[i].y, high ), 
								vertex[i].z );
			}
		}

		if ( textureMode == TESS_SPHERE_TOPHALF ) 
		{
			for( unsigned i=0; i<texture.size(); ++i ) {
				//texture[i].y = Clamp( texture[i].y*2.0f - 1.0f, 0.0f, 1.0f );
				//texture[i].y = texture[i].y*2.0f - 1.0f;
				texture[i].y = Clamp( texture[i].y, 0.0f, 1.0f );
			}
		}
		else if ( textureMode == TESS_SPHERE_NORMAL ) {
			for( unsigned i=0; i<texture.size(); ++i ) {
				texture[i].y = texture[i].y*0.5f + 0.5f;
			}
		}
	}

	// Apply the radius
	for( unsigned i=0; i<vertex.size(); ++i ) {
		vertex[i] = vertex[i] * radius;
	}
	// If we are *inside* the sphere, then flip the normals by re-ordering the indices
	if ( innerNormals ) {
		for( unsigned i=0; i<index.size(); i+=3 ) {
			Swap( &index[i+0], &index[i+2] );
		}
	}
}


void grinliz::TessellateCube(	const Vector3F& center,
								const Vector3F& size,
								std::vector< Vector3F >* _vertex, 
								std::vector< U32 >* _index,
								std::vector< Vector3F >* _normal,
								std::vector< Vector2F >* _texture )
{
	GLASSERT( _vertex );
	GLASSERT( _index );
	_vertex->resize( 0 );
	_index->resize( 0 );
	if ( _normal ) _normal->resize( 0 );
	if ( _texture ) _texture->resize( 0 );

	const Vector3F X = { size.x*0.5f, 0.0f, 0.0f };
	const Vector3F Y = { 0.0f, size.y*0.5f, 0.0f };
	const Vector3F Z = { 0.0f, 0.0f, size.z*0.5f };

	Vector3F xyz[8] = { center - X - Y - Z,		// 0
						center + X - Y - Z,		// 1
						center - X + Y - Z,		// 2
						center + X + Y - Z,		// 3
						center - X - Y + Z,		// 4
						center + X - Y + Z,		// 5
						center - X + Y + Z,		// 6
						center + X + Y + Z };		// 7

	// Front, right, back, left, top, bottom
	// positive would from lower left
	int face[] = {
		4, 5, 7, 6,		// Front
		5, 1, 3, 7,		// Right
		1, 0, 2, 3,		// Back
		0, 4, 6, 2,		// Left
		6, 7, 3, 2,		// Top
		0, 1, 5, 4 };	// Bottom

	Vector3F normal[] = {  { 0.f, 0.f, 1.f },
							{ 1.f, 0.f, 0.f },
							{ 0.f, 0.f, -1.f },
							{ -1.f, 0.f, 0.f },
							{ 0.f, 1.f, 0.f },
							{ 0.f, -1.f, 0.f } };
								
	for( int i=0; i<6; ++i ) {
		U32 index = (U32)_vertex->size();
		_vertex->push_back( xyz[ face[i*4+0] ] );
		_vertex->push_back( xyz[ face[i*4+1] ] );
		_vertex->push_back( xyz[ face[i*4+2] ] );
		_vertex->push_back( xyz[ face[i*4+3] ] );
		
		_index->push_back( index + 0 );
		_index->push_back( index + 1 );
		_index->push_back( index + 2 );
		_index->push_back( index + 0 );
		_index->push_back( index + 2 );
		_index->push_back( index + 3 );

		if ( _normal ) {
			for( int k=0; k<4; ++k )
				_normal->push_back( normal[i] );
		}
		if ( _texture ) {
			Vector2F uv;
			uv.Set( 0.0f, 0.0f );	_texture->push_back( uv );
			uv.Set( 1.0f, 0.0f );	_texture->push_back( uv );
			uv.Set( 1.0f, 1.0f );	_texture->push_back( uv );
			uv.Set( 0.0f, 1.0f );	_texture->push_back( uv );
		}
	}	
}
#endif

const void Quaternion::ToMatrix( Matrix4* mat ) const
{
	GLASSERT( Equal( 1.0f, x*x + y*y + z*z + w*w, 0.0001f ) );

	float fTx  = 2.0f*x;
	float fTy  = 2.0f*y;
	float fTz  = 2.0f*z;
	float fTwx = fTx*w;
	float fTwy = fTy*w;
	float fTwz = fTz*w;
	float fTxx = fTx*x;
	float fTxy = fTy*x;
	float fTxz = fTz*x;
	float fTyy = fTy*y;
	float fTyz = fTz*y;
	float fTzz = fTz*z;
	mat->x[0] = 1.0f-(fTyy+fTzz);
	mat->x[1] = fTxy-fTwz;
	mat->x[2] = fTxz+fTwy;
	mat->x[3] = 0.0f;

	mat->x[4] = fTxy+fTwz;
	mat->x[5] = 1.0f-(fTxx+fTzz);
	mat->x[6] = fTyz-fTwx;
	mat->x[7] = 0.0f;

	mat->x[8] = fTxz-fTwy;
	mat->x[9] = fTyz+fTwx;
	mat->x[10] = 1.0f-(fTxx+fTyy);
	mat->x[11] = 0.0f;

	mat->x[12] = 0.0f;
	mat->x[13] = 0.0f;
	mat->x[14] = 0.0f;
	mat->x[15] = 1.0f;

	GLASSERT( mat->IsRotation() );
}


void Quaternion::FromRotationMatrix( const Matrix4& m )
{
	GLASSERT( m.IsRotation() );

    float trace = m.x[0] + m.x[5] + m.x[10];
	const int next[3] = { 1, 2, 0 };

    if ( trace > 0.0f )
    {
		float s = (float)sqrt(trace + 1.0f);
		w = s / 2.0f;
		s = 0.5f / s;
		
		x = ( m.x[9] - m.x[6] ) * s;
		y = ( m.x[2] - m.x[8] ) * s;
		z = ( m.x[4] - m.x[1] ) * s;
	}
	else
	{
		int i=0;

		if (m.x[5]  > m.x[0])		i = 1;
		if (m.x[10] > m.x[i*4+i])	i = 2;
    
		int j = next[i];
		int k = next[j];

		float s = (float) sqrt ((m.x[i*4+i] - (m.x[j*4+j] + m.x[k*4+k])) + 1.0f);
      
		float q[4];
		q[i] = s * 0.5f;

		if (s != 0.0f) s = 0.5f / s;

		q[3] = (m.x[k*4+j] - m.x[j*4+k]) * s;
		q[j] = (m.x[j*4+i] + m.x[i*4+j]) * s;
		q[k] = (m.x[k*4+i] + m.x[i*4+k]) * s;

		x = q[0];
		y = q[1];
		z = q[2];
		w = q[3];
	}
	GLASSERT( Equal( 1.0f, x*x + y*y + z*z + w*w, 0.0001f ) );
}


const void Quaternion::ToAxisAngle( Vector3F* axis, float* angleOut ) const
{
	GLASSERT( Equal( sqrt( (double)(x*x + y*y + z*z + w*w) ), 1.0, 0.001 ) );
    
	float len2 = x*x + y*y + z*z;

    if ( len2 > 0.0 )
    {
		*angleOut = (2.0f* (float)acosf(w) * RAD_TO_DEG );
		float invLen = 1.0f / (float)sqrtf( len2 );
        axis->x = x*invLen;
        axis->y = y*invLen;
        axis->z = z*invLen;
    }
    else
    {
        axis->x = 1.0f;
        axis->y = 0.0f;
        axis->z = 0.0f;
		*angleOut = 0.0f;
    }
}



void Quaternion::FromAxisAngle( const Vector3F& axis, float angle )
{
	angle *= DEG_TO_RAD;
    float halfAngle = 0.5f*angle;
    float sinHalfAngle = (float) sin(halfAngle);
    w = (float)cos(halfAngle);
    x = sinHalfAngle*axis.x;
    y = sinHalfAngle*axis.y;
    z = sinHalfAngle*axis.z;
	GLASSERT( Equal( 1.0f, x*x + y*y + z*z + w*w, 0.0001f ) );
}


void Quaternion::Normalize()
{
	//float length = (float) sqrt( x*x + y*y + z*z + w*w );
	float invLength = 1.0f / Length( x, y, z, w );
	x = x * invLength;
	y = y * invLength;
	z = z * invLength;
	w = w * invLength;
}


void Quaternion::Multiply( const Quaternion& a, const Quaternion& b, Quaternion* r )
{
	r->w = a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z;
	r->x = a.w*b.x + a.x*b.w + a.y*b.z + a.z*b.y;
	r->y = a.w*b.y + a.y*b.w + a.z*b.x + a.x*b.z;
	r->z = a.w*b.z + a.z*b.w + a.x*b.y + a.y*b.x;
}


void Quaternion::SLERP( const Quaternion& start, const Quaternion& end, float t, Quaternion* r )
{
	float         to[4];
    float        omega, cosOmega, sinOmega, scale0, scale1;

	cosOmega = start.x*end.x + start.y*end.y + start.z*end.z + start.w*end.w;

    // adjust signs (if necessary)
    
	if ( cosOmega < 0.0f )
	{ 
		cosOmega = -cosOmega; 
		to[0] = -end.x;
		to[1] = -end.y;
		to[2] = -end.z;
		to[3] = -end.w;
	} 
	else  
	{
		to[0] = end.x;
		to[1] = end.y;
		to[2] = end.z;
		to[3] = end.w;
   }
       
	const float EPSILON = 0.0001f;

	if ( (1.0f - cosOmega) > EPSILON ) 
	{
		// standard case (slerp)
        omega = (float)acosf(cosOmega);
        sinOmega = (float)sinf(omega);
        scale0 = (float)sinf((1.0f - t) * omega) / sinOmega;
        scale1 = (float)sinf(t * omega) / sinOmega;
	} 
	else 
	{      
		// approaching singularity... :)  
        scale0 = 1.0f - t;
        scale1 = t;
    }
	r->x = scale0 * start.x + scale1 * to[0];
	r->y = scale0 * start.y + scale1 * to[1];
	r->z = scale0 * start.z + scale1 * to[2];
	r->w = scale0 * start.w + scale1 * to[3];
	GLASSERT( Equal( 1.0f, r->x*r->x + r->y*r->y + r->z*r->z + r->w*r->w, 0.0001f ) );
}


int grinliz::ComparePlaneAABB( const Plane& plane, const Rectangle3F& aabb )
{
	// Only need to check the most positive and most negative point,
	// or minimum and maximum depending on point of view. The octant
	// of the plane normal implies a vertex to check.

	Vector3F posPoint, negPoint;

	for( int i=0; i<3; ++i ) {
		if ( plane.n.X(i) > 0.0f )	{
			posPoint.X(i) = aabb.X1(i);
			negPoint.X(i) = aabb.X0(i);
		} else {
			posPoint.X(i) = aabb.X0(i);
			negPoint.X(i) = aabb.X1(i);
		}
	}

	// RTR 735
	// f(P) = N*P + d

	// If the most negative point is still on the positive
	// side, it is a positive, and vice versa.
	float fp = DotProduct( plane.n, negPoint ) + plane.d;
	if ( fp > 0.0f )
	{
		return POSITIVE;
	}

	fp = DotProduct( plane.n, posPoint ) + plane.d;
	if ( fp < 0.0f )
	{
		return NEGATIVE;
	}
	return INTERSECT;
}


int grinliz::ComparePlanePoint( const Plane& plane, const Vector3F& p )
{
	float fp = DotProduct( plane.n, p ) + plane.d;
	if ( fp > 0.0f )
	{
		return POSITIVE;
	}
	else if ( fp < 0.0f )
	{
		return NEGATIVE;
	}
	return INTERSECT;
}


//
// Moller - Trumbore approach.
//
int grinliz::IntersectRayTri(	const Vector3F& orig, const Vector3F& dir,
								const Vector3F& vert0, const Vector3F& vert1, const Vector3F& vert2,
								Vector3F* intersect )
{	
	Vector3F pvec, qvec;
	float det, invDet;
	float t, u, v;

	// The vectors for the 2 edges sharing vertex 0.
	Vector3F edge1 = vert1 - vert0;
	Vector3F edge2 = vert2 - vert0;

	// Used for u and the determinant.
	CrossProduct( dir, edge2, &pvec );

	// If the determinant is near 0, then we are in plane. Also, this
	// is a culling check, so if its negative, we're inside a manifold.
	det = DotProduct( edge1, pvec );

	if ( det < EPSILON )
		return REJECT;

	// Now, vertex0 (on the triangle) back to the ray origin.
	Vector3F tvec = orig - vert0;

	// Calculate the U parameter and test the initial bounds.
	u = DotProduct( tvec, pvec );
	if ( u < 0.0f || u > det )
		return REJECT;

	// And now the V parameter.
	CrossProduct( tvec, edge1, &qvec );

	// Test the V bounds.
	v = DotProduct( dir, qvec );
	if ( v < 0.0f || ( u + v ) > det )
		return REJECT;

	// We did intersect! Calculate the t, and then the intersection. This 
	// is pretty expensive if the actually point of intersection doesn't
	// matter.
	t = DotProduct( edge2, qvec );

	// Lee: it seems t being negative is valid here, which seems odd
	// and missing from the original paper??
	if ( t < 0.0f )
		return REJECT;

	if ( intersect )
	{
		GLASSERT( t >= 0.0f );
		invDet = 1.0f / det;

		t *= invDet;
		u *= invDet;
		v *= invDet;

		// T = ( 1-u-v )V0 + uV1 + vV2
		float comp = 1.0f - u - v;
		intersect->x = comp*vert0.x + u*vert1.x + v*vert2.x;
		intersect->y = comp*vert0.y + u*vert1.y + v*vert2.y;
		intersect->z = comp*vert0.z + u*vert1.z + v*vert2.z;
	}
	return INTERSECT;
}


int grinliz::IntersectRayAAPlane(	const Vector3F& point, const Vector3F& dir,
									int planeType, float loc,
									Vector3F* intersect,
									float* t )
{
	GLASSERT( planeType >= 0 && planeType <= 2 );

	// Abstract out the plane, to avoid 'if' and 
	// code duplication.
	float p = *( &point.x + planeType );
	float d = *( &dir.x + planeType );

	if ( d < EPSILON && d > -EPSILON )
		return REJECT;

	*t = ( loc - p ) / d;
	if ( *t < 0.0f )
		return REJECT;

	intersect->x = point.x + dir.x * (*t);
	intersect->y = point.y + dir.y * (*t);
	intersect->z = point.z + dir.z * (*t);

	// Make the plane intersection exact:
	GLASSERT( Equal( *(&intersect->x + planeType), loc, 0.001f ) );
	*(&intersect->x + planeType) = loc;

	return INTERSECT;
}


// From: http://astronomy.swin.edu.au/~pbourke/geometry/planeline/
//
// plane: A x + B y + C z + D = 0
//
int grinliz::IntersectLinePlane( const Vector3F& p1, const Vector3F& p2,
						const Plane& plane, 
						Vector3F* out,
						float* t )
{
	float denom = plane.n.x*(p1.x-p2.x) + plane.n.y*(p1.y-p2.y) + plane.n.z*(p1.z-p2.z);
	if ( fabsf( denom ) < 0.001 )
		return REJECT;

	float num = plane.n.x*p1.x + plane.n.y*p1.y + plane.n.z*p1.z + plane.d;

	*t = num / denom;

	*out = p1 + (*t)*(p2-p1);
	return INTERSECT;
}


int grinliz::IntersectRayAABB(	const Vector3F& origin, 
								const Vector3F& dir,
								const Rectangle3F& aabb,
								Vector3F* intersect,
								float* t )
{
	enum
	{
		RIGHT,
		LEFT,
		MIDDLE
	};

	bool inside = true;
	int quadrant[3];
	int i;
	int whichPlane;
	float maxT[3];
	float candidatePlane[3];

	const float *pOrigin = &origin.x;
	const float *pBoxMin = &aabb.pos.x;
	Vector3F aabbMax = aabb.pos + aabb.size;
	const float *pBoxMax = &aabbMax.x;
	const float *pDir    = &dir.x;
	float *pIntersect = &intersect->x;

	// Find candidate planes
	for (i=0; i<3; ++i)
	{
		if( pOrigin[i] < pBoxMin[i] ) 
		{
			quadrant[i] = LEFT;
			candidatePlane[i] = pBoxMin[i];
			inside = false;
		}
		else if ( pOrigin[i] > pBoxMax[i] ) 
		{
			quadrant[i] = RIGHT;
			candidatePlane[i] = pBoxMax[i];
			inside = false;
		}
		else	
		{
			quadrant[i] = MIDDLE;
		}
	}
	// Ray origin inside bounding box
	if( inside )	
	{
		*intersect = origin;
		*t = 0.0f;
		return INSIDE;
	}


	// Calculate T distances to candidate planes
	for (i = 0; i < 3; i++)
	{
		if (   quadrant[i] != MIDDLE 
		    && ( pDir[i] > EPSILON || pDir[i] < -EPSILON ) )
		{
			maxT[i] = ( candidatePlane[i]-pOrigin[i] ) / pDir[i];
		}
		else
		{
			maxT[i] = -1.0f;
		}
	}

	// Get largest of the maxT's for final choice of intersection
	whichPlane = 0;
	for (i = 1; i < 3; i++)
		if (maxT[whichPlane] < maxT[i])
			whichPlane = i;

	// Check final candidate actually inside box
	if ( maxT[whichPlane] < 0.0f )
		 return REJECT;

	for (i = 0; i < 3; i++)
	{
		if (whichPlane != i ) 
		{
			pIntersect[i] = pOrigin[i] + maxT[whichPlane] * pDir[i];
			if (pIntersect[i] < pBoxMin[i] || pIntersect[i] > pBoxMax[i])
				return REJECT;
		} 
		else 
		{
			pIntersect[i] = candidatePlane[i];
		}
	}
	*t = maxT[whichPlane];
	return INTERSECT;
}	


int grinliz::IntersectRayAllAABB(	const Vector3F& origin, const Vector3F& dir,
									const Rectangle3F& aabb,
									int* inTest,
									Vector3F* inIntersect,
									int *outTest,
									Vector3F* outIntersect )
{
	// Could be more efficient, but looking for simplicity as I write this.
	float t;

	// First check if we hit the box at all, and that establishes in test.
	*inTest = IntersectRayAABB( origin, dir, aabb, inIntersect, &t );
	if ( *inTest == grinliz::REJECT ) {
		*outTest = grinliz::REJECT;
		return grinliz::REJECT;
	}

	// Could get fancy and intersect from the inside. But that's hard, so I'll run
	// the ray out and shoot it backwards.
	float deltaLen = aabb.size.x + aabb.size.y + aabb.size.z;

	Vector3F dirNormal = dir;
	dirNormal.Normalize();
	Vector3F invOrigin = origin + dir*deltaLen;
	Vector3F invDir = -dirNormal;

	*outTest = IntersectRayAABB( invOrigin, invDir, aabb, outIntersect, &t );
	
	GLASSERT( *outTest != grinliz::INSIDE );	// bad algorith.
	if ( *outTest == grinliz::REJECT ) {
		// some strange floating point thing. Hit a corner. Reject everything.
		*inTest = grinliz::REJECT;
		return grinliz::REJECT;
	}
	return grinliz::INTERSECT;
}


int grinliz::IntersectRayPlane(	const Vector3F& origin, const Vector3F& dir,
								int plane, float x,
								Vector3F* intersect )
{
	GLASSERT( plane >=0 && plane < 3 );

	if ( dir.X(plane) > -EPSILON && dir.X(plane) < EPSILON )
		return REJECT;
	if ( dir.X(plane) > 0.0f && origin.X(plane) >= x )
		return REJECT;
	if ( dir.X(plane) < 0.0f && origin.X(plane) <= x )
		return REJECT;

	float t = ( x - origin.X(plane) ) / dir.X(plane);

	if ( intersect ) {
		intersect->x = origin.x + dir.x * t;
		intersect->y = origin.y + dir.y * t;
		intersect->z = origin.z + dir.z * t;
	}
	return INTERSECT;
}


float grinliz::PointBetweenPoints( const Vector3F& p0, const Vector3F& p1, const Vector3F& pi )
{
	float validCount = 0.0f;
	float t = 0.0f;

	// p0.x + t*(p1.x-p0.x) = pi.x
	for( int i=0; i<3; ++i ) {
		float denom = p1.X(i) - p0.X(i);
		if ( !Equal( denom, 0.0f, 0.001f ) ) {
			float t0 = ( pi.X(i) - p0.X(i) ) / denom;
			GLASSERT( t0 >= 0.0f && t0 <= 1.0f );
			t += t0;
			validCount += 1.0f;
		}
	}
	t /= validCount;
	GLASSERT( t >= 0.0f && t <= 1.0f );
	return t;
}


/*
int grinliz::IntersectionRayAABB(	const Ray& ray,
									const Rectangle3F& aabb,
									Rectangle3F* result )
{
	bool hasStart = false;
	bool hasEnd = false;
	Vector3F start, end;

	GLASSERT( Equal( ray.direction.Length(), 1.0f, 0.001f ) );

	if ( aabb.Contains( ray.origin ) )
	{
		hasStart = true;
		start = ray.origin;
	}

	// Check for an intersection with each face. If t>0 and it is a 
	// positive dot then it is an 'end'. If t>0 and it is a negative
	// dot, then it is a 'start'.

	Vector3F at;
	float t;			

	for( int k=0; k<=1; ++k )
	{
		Vector3F originToPlane;
		if ( k == 0 ) {
			// min
			originToPlane.Set(	aabb.min.x - ray.origin.x,
								aabb.min.y - ray.origin.y,
								aabb.min.z - ray.origin.z );
		}
		else {
			// max
			originToPlane.Set(	aabb.max.x - ray.origin.x,
								aabb.max.y - ray.origin.y,
								aabb.max.z - ray.origin.z );
		}

		for ( int i=0; i<3 && !(hasEnd && hasStart); ++i ) {
			int i1 = (i + 1)%3;
			int i2 = (i + 2)%3;

			Vector3F planeNormal = { 0.0f, 0.0f, 0.0f };
			planeNormal.X(i) = ( k == 0 ) ? -1.0f : 1.0f;
			float dot = DotProduct( originToPlane, planeNormal );

			int hit = IntersectRayAAPlane(	ray.origin, ray.direction,
											i, 
											(k==0) ? aabb.min.X(i) : aabb.max.X(i),
											&at, &t );
			if (    hit == INTERSECT 
				 && t > 0.0f
				 && InRange( at.X(i1), aabb.min.X(i1), aabb.max.X(i1) )
				 && InRange( at.X(i2), aabb.min.X(i2), aabb.max.X(i2) ) )
			{
				// We have hit a face of the AABB
				if ( dot > 0.0f ) {
					GLASSERT( !hasEnd );
					hasEnd = true;
					if ( t > ray.length )
						end = ray.origin + ray.direction*t;
					else
						end = at;
				}
				else if ( dot < 0.0f ) {
					GLASSERT( !hasStart );
					hasStart = true;
					if ( t > ray.length ) {
						return REJECT;	// the start never gets to the AABB
					}
					start = at;
				}
			}
		}
	}
	if( !hasStart || !hasEnd ) {
		return REJECT;
	}
	for( int i=0; i<3; ++i ) {
		result->min.X(i) = Min( start.X(i), end.X(i) );
		result->max.X(i) = Max( start.X(i), end.X(i) );
	}
	return INTERSECT;
}
*/


// This (amazing) little algorithm derived from:
// http://astronomy.swin.edu.au/~pbourke/geometry/lineline3d/
//
int grinliz::IntersectLineLine(	const Vector3F& p1, const Vector3F& p2,
								const Vector3F& p3, const Vector3F& p4,
								Vector3F* pa, 
								Vector3F* pb )
{
	Vector3F p13,p43,p21;
	float d1343,d4321,d1321,d4343,d2121;
	float numer,denom;

	p13.x = p1.x - p3.x;
	p13.y = p1.y - p3.y;
	p13.z = p1.z - p3.z;
	p43.x = p4.x - p3.x;
	p43.y = p4.y - p3.y;
	p43.z = p4.z - p3.z;

	if (   fabsf(p43.x)  < EPSILON 
		&& fabsf(p43.y)  < EPSILON 
		&& fabsf(p43.z)  < EPSILON )
	{
		return REJECT;
	}

	p21.x = p2.x - p1.x;
	p21.y = p2.y - p1.y;
	p21.z = p2.z - p1.z;
	if (	fabsf(p21.x)  < EPSILON 
	     && fabsf(p21.y)  < EPSILON 
		 && fabsf(p21.z)  < EPSILON )
	{
		return REJECT;
	}

	d1343 = p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
	d4321 = p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
	d1321 = p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
	d4343 = p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
	d2121 = p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

	denom = d2121 * d4343 - d4321 * d4321;
	if (fabsf(denom) < EPSILON)
	{
		return REJECT;
	}
	numer = d1343 * d4321 - d1321 * d4343;

	float mua = numer / denom;
	float mub = (d1343 + d4321 * mua) / d4343;

	pa->x = p1.x + mua * p21.x;
	pa->y = p1.y + mua * p21.y;
	pa->z = p1.z + mua * p21.z;

	pb->x = p3.x + mub * p43.x;
	pb->y = p3.y + mub * p43.y;
	pb->z = p3.z + mub * p43.z;

	return INTERSECT;
}

// Based on http://exaflop.org/docs/cgafaq/cga1.html
//
int grinliz::IntersectLineLine(	const Vector2F& a, const Vector2F& b,
								const Vector2F& c, const Vector2F& d,
								Vector2F* out,
								float* t0,
								float* t1 )
{
	//	AB=A+r(B-A), r in [0,1]
    //	CD=C+s(D-C), s in [0,1]

    //        (Ay-Cy)(Dx-Cx)-(Ax-Cx)(Dy-Cy)
    //    r = -----------------------------  (eqn 1)
    //        (Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx)            
	//
	//		  (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
    //    s = -----------------------------  (eqn 2)
    //        (Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx)

	float denom = (b.x-a.x)*(d.y-c.y) - (b.y-a.y)*(d.x-c.x);

	if ( fabsf( denom ) < EPSILON )
		return REJECT;

	float aymcy = a.y-c.y;
	float axmcx = a.x-c.x;
	float r = ( aymcy*(d.x-c.x) - axmcx*(d.y-c.y) ) / denom;
	float s = ( aymcy*(b.x-a.x) - axmcx*(b.y-a.y) ) / denom;

	// P=A+r(B-A) or

	*out = a + r*(b-a);
	*t0 = r;
	*t1 = s;

	#ifdef DEBUG
	if ( InRange( r, -1.0f, 2.0f ) && InRange( s, -1.0f, 2.0f ) )
	{
		Vector2F r2 = c + s*(d-c);
		GLASSERT( Equal( out->x, r2.x, 0.001f ) );
		GLASSERT( Equal( out->y, r2.y, 0.001f ) );
	}
	#endif

	return INTERSECT;
}


int grinliz::IntersectRayAABB(	const Vector2F& origin, const Vector2F& dir,
								const Rectangle2F& aabb,
								Vector2F* intersect,
								float* t, int* planeIndex, int* quad )
{
	enum
	{
		RIGHT,
		LEFT,
		MIDDLE
	};

	bool inside = true;
	int quadrant[2];
	int i;
	int whichPlane;
	float maxT[2];
	float candidatePlane[2];

	const float *pOrigin = &origin.x;
	const float *pBoxMin = &aabb.pos.x;
	Vector2F boxMax = aabb.pos + aabb.size;
	const float *pBoxMax = &boxMax.x;
	const float *pDir    = &dir.x;
	float *pIntersect = &intersect->x;

	// Find candidate planes
	for (i=0; i<2; ++i)
	{
		if( pOrigin[i] < pBoxMin[i] ) 
		{
			quadrant[i] = LEFT;
			candidatePlane[i] = pBoxMin[i];
			inside = false;
		}
		else if ( pOrigin[i] > pBoxMax[i] ) 
		{
			quadrant[i] = RIGHT;
			candidatePlane[i] = pBoxMax[i];
			inside = false;
		}
		else	
		{
			quadrant[i] = MIDDLE;
		}
	}

	// Ray origin inside bounding box
	if( inside )	
	{
		*intersect = origin;
		*t = 0.0f;
		return INSIDE;
	}


	// Calculate T distances to candidate planes
	for (i = 0; i < 2; i++)
	{
		if (   quadrant[i] != MIDDLE 
		    && ( pDir[i] > EPSILON || pDir[i] < -EPSILON ) )
		{
			maxT[i] = ( candidatePlane[i]-pOrigin[i] ) / pDir[i];
		}
		else
		{
			maxT[i] = -1.0f;
		}
	}

	// Get largest of the maxT's for final choice of intersection
	whichPlane = 0;
	for (i = 1; i < 2; i++)
		if (maxT[whichPlane] < maxT[i])
			whichPlane = i;

	// Check final candidate actually inside box
	if ( maxT[whichPlane] < 0.0f )
		 return REJECT;

	for (i = 0; i < 2; i++)
	{
		if (whichPlane != i ) 
		{
			pIntersect[i] = pOrigin[i] + maxT[whichPlane] * pDir[i];
			if (pIntersect[i] < pBoxMin[i] || pIntersect[i] > pBoxMax[i])
				return REJECT;
		} 
		else 
		{
			pIntersect[i] = candidatePlane[i];
		}
	}
	if ( t )
		*t = maxT[whichPlane];
	if ( planeIndex )
		*planeIndex = whichPlane;
	if ( quad )
		*quad = quadrant[whichPlane];

	return INTERSECT;
}


int grinliz::PointInPolygon( const Vector2F& p, const Vector2F* vertex, int numVertex )
{
	for( int i=0; i<numVertex; ++i )
	{
		// Inside if every point is to the left.
		int j = (i+1)%numVertex;

		Vector2F line = vertex[j]-vertex[i];
		line.RotatePos90();

		if ( DotProduct( line, p - vertex[i] ) < 0.0f )
			return REJECT;
	}
	return INTERSECT;
}


int grinliz::Intersect3Planes( const Plane& p1, const Plane& p2, const Plane& p3, Vector3F* intersect )
{
	// http://astronomy.swin.edu.au/~pbourke/geometry/3planes/
	// Nothing that grinliz defines the "d" term with the opposite sign.

	float denom = -DotProduct( p1.n, CrossProduct( p2.n, p3.n ) );
	if ( fabsf( denom ) < EPSILON )
		return REJECT;

	float inv = 1.0f / denom;

	Vector3F num =  (   p1.d*CrossProduct( p2.n, p3.n )
			 	      + p2.d*CrossProduct( p3.n, p1.n )
				      + p3.d*CrossProduct( p1.n, p2.n ) );
	intersect->Set( num.x*inv, num.y*inv, num.z*inv );
	return INTERSECT;
}


int grinliz::ComparePlaneSphere( const Plane& plane, const Sphere& sphere )
{
	// Take advantage of the unit normally to get the distance.
	float d = plane.n.x*sphere.origin.x + plane.n.y*sphere.origin.y + plane.n.z*sphere.origin.z + plane.d;

	if ( d > sphere.radius ) {
		return grinliz::POSITIVE;
	}
	else if ( d < -sphere.radius ) {
		return grinliz::NEGATIVE;
	}
	return grinliz::INTERSECT;
}


int grinliz::IntersectRaySphere(	const Sphere& sphere,
									const Vector3F& p,
									const Vector3F& dir )
{
	Vector3F raySphere = sphere.origin - p;
	float raySphereLen2 = DotProduct( raySphere, raySphere );
	float sphereR2 = sphere.radius*sphere.radius;

	if (raySphereLen2 < sphereR2) 
	{	
		// Origin is inside the sphere.
		return INSIDE;
	} 
	else 
	{
		// Clever idea: what is the rays closest approach to the sphere?
		// see: http://www.devmaster.net/wiki/Ray-sphere_intersection

		float closest = DotProduct(raySphere, dir);
		if (closest < 0) {
			// Then we are pointing away from the sphere (and we know we aren't inside.)
			return REJECT;
		}

		float halfCordLen = (sphereR2 - raySphereLen2) / DotProduct(dir, dir) + (closest*closest);
		if ( halfCordLen > 0 ) {
			//*t = closest - halfCordLen.Sqrt();
			return INTERSECT;
		}
		/*
		// haldCordLen *= Dot
		float det = (sphereR2 - raySphereLen2) + (closest*closest)*DotProduct( dir, dir );
		if ( det > 0 ) {
			return INTERSECT;
		}	
		*/
	}
	return REJECT;
}