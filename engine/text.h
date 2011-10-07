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

#ifndef UFOATTACK_TEXT_INCLUDED
#define UFOATTACK_TEXT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "screenport.h"
#include "texture.h"
#include "vertex.h"
#include "../gamui/gamui.h"
#include "../shared/gamedbreader.h"

class GPUShader;

class UFOText
{
public:
	static UFOText* Instance() 	{ GLASSERT( instance ); return instance; }

	void Draw( int x, int y, const char* format, ... );
	void Metrics(	int c, int c1,
					float lineHeight,
					gamui::IGamuiText::GlyphMetrics* metric );

	static void Create( const gamedb::Reader*, Texture* texture, Screenport* screenport );
	static void Destroy();

private:
	UFOText( const gamedb::Reader*, Texture* texture, Screenport* screenport );
	~UFOText()	{}

	struct Metric
	{
		S16 isSet;
		S16 advance;
		S16 x, y, width, height;
		S16 xoffset, yoffset;
	};

	void CacheMetric( int c );
	void CacheKern( int c, int cPrev );

	void TextOut( GPUShader* shader, const char* str, int x, int y, int h, int *w );

	static UFOText* instance;
	
	const gamedb::Reader* database;
	Texture* texture;
	Screenport* screenport;

	float fontSize;
	float texWidthInv;
	float texHeight;
	float texHeightInv;

	enum {
		BUF_SIZE = 30,
		CHAR_OFFSET = 32,
		CHAR_RANGE  = 128 - CHAR_OFFSET
	};

	int MetricIndex( int c )          { return c - CHAR_OFFSET; }
	int KernIndex( int c, int cPrev ) { return (c-CHAR_OFFSET)*CHAR_RANGE + (cPrev-CHAR_OFFSET); }

	PTVertex2	vBuf[BUF_SIZE*4];
	U16			iBuf[BUF_SIZE*6];
	Metric		metricCache[ CHAR_RANGE ];
	S8			kerningCache[ CHAR_RANGE*CHAR_RANGE ];
};

#endif // UFOATTACK_TEXT_INCLUDED