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

#include <stdlib.h>
#include "ufoutil.h"
#include "../grinliz/glutil.h"

using namespace grinliz;

void CEnsureCap( unsigned needInBytes, unsigned* capInBytes, void** stack )
{
	if ( needInBytes > *capInBytes ) {
		*capInBytes = grinliz::CeilPowerOf2( needInBytes );
		*stack = realloc( *stack, *capInBytes );
	}
}


void Matrix2I::SinCos( int rotation, int* sinTheta, int* cosTheta ) const
{
	switch ( rotation ) {
		case 0:
			*sinTheta = 0;
			*cosTheta = 1;
			break;
		case 90:
			*sinTheta = 1;
			*cosTheta = 0;
			break;
		case 180:
			*sinTheta = 0;
			*cosTheta = -1;
			break;
		case 270:
			*sinTheta = -1;
			*cosTheta = 0;
			break;
		default:	
			GLASSERT( 0 );
	}
}


void Matrix2I::SetRotation( int r )
{
	int cosTheta = 1, sinTheta = 0;
	SinCos( r, &sinTheta, &cosTheta );

	//  cosT	-sinT
	//	sinT	cosT
	a = cosTheta;	b = -sinTheta;
	c = sinTheta;	d = cosTheta;
}


void Matrix2I::SetXZRotation( int r )
{
	int cosTheta = 1, sinTheta = 0;
	SinCos( r, &sinTheta, &cosTheta );

	//  cosT	-sinT
	//	sinT	cosT
	a = cosTheta;	b = sinTheta;
	c = -sinTheta;	d = cosTheta;
}


//int Matrix2I::CalcXZRotation() const
//{
//	int sinTheta, cosTheta;
//	for( int r=0; r<360; r+=90 ) {
//		SinCos( r, &sinTheta, &cosTheta );
//		if ( a== cosTheta && b == sinTheta && c == -sinTheta && d == cosTheta ) {
//			return r;
//		}
//	}
//	GLASSERT( 0 );
//	return 0;
//}


void Matrix2I::Invert( Matrix2I* inverse ) const
{
	/*
	| a b x |-1             |   1d-0y  -(1b-0x)   yb-dx  |
	| c d y |    =  1/DET * | -(1c-0y)   1a-0x  -(ya-cx) |
	| 0 0 1 |               |   0c-0d  -(0a-0b)   da-cb  |

	DET  =  a(1d-0y)-c(1b-0x)+0(yb-dx)
	*/

	int det = a*d - c*b;
	GLASSERT( det != 0 );

	inverse->a =  d / det;
	inverse->b = -b / det;
	inverse->c = -c / det;
	inverse->d =  a / det;
	inverse->x   =  (y*b-d*x) / det;
	inverse->y   = -(y*a-c*x) / det;

#ifdef DEBUG
	Matrix2I test;
	test = (*this) * (*inverse);
	GLASSERT( test.a == 1 && test.d == 1 && test.b == 0 && test.c == 0 && test.x == 0 && test.y == 0 );
#endif
}


void MultMatrix2I( const Matrix2I& a, 
	               const grinliz::Rectangle2I& b, 
				   grinliz::Rectangle2I* c )
{
	Vector2I out = a * b.Corner(0);
	c->pos = out;
	c->size.Zero();

	for( int i=1; i<4; ++i ) {
		Vector2I w = a * b.Corner(i);
		fixme integer union
		c->DoUnion( w );
	}
}



LineWalk::LineWalk(int x0, int y0, int x1, int y1)
{
	int dx = x1-x0;
	int dy = y1-y0;
	axisDir = 1;

	GLASSERT( dx || dy );

	if ( abs( dy ) > abs(dx) ) {
		// y is major axis. delta = dx per distance y
		axis = 1;
		nSteps = abs(dy)-1;
		if ( dy < 0 )
			axisDir = -1;
		delta = Fixed( dx ) / Fixed( abs(dy) );
		GLASSERT( delta < 1 && delta > -1 );
	}
	else {
		// x is the major aris. delta = dy per distance x
		axis = 0;
		nSteps = abs(dx)-1;
		if ( dx < 0 )
			axisDir = -1;
		delta = Fixed( dy ) / Fixed( abs(dx) );
		GLASSERT( delta <= 1 && delta >= -1 );
	}
	step = 0;
	p.Set( Fixed(x0)+Fixed(0.5f), Fixed(y0)+Fixed(0.5f) );
	q = p;
	q.X(axis) += axisDir;
	q.X(!axis) += delta;
}


void LineWalk::Step( int n )
{
	GLASSERT( step+n <= nSteps+1 );
	GLASSERT( n > 0 );

	if ( n > 1 ) {
		q.X(axis)  += axisDir*(n-1);
		q.X(!axis) += delta*(n-1);
	}
	p = q;
	q.X(axis)  += axisDir;
	q.X(!axis) += delta;

	step += n;
}
