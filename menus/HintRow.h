/*
HintRow.h -- a hint line with controller button glyphs: "{A} join  {B} back  {X} refresh".

{A} {B} {X} {Y} {S} (Start) {Z} {L} {R} become the button glyphs the engine's hi-res text atlas carries (codes 128..135,
scripts/bake-hitext.py, original art). When the atlas is not in use (gc_hitext 0) the token is drawn as "[A]" text.
*/
#pragma once
#include "Action.h"
#include "Utils.h"
#include "FontManager.h"

class CMenuHintRow : public CMenuAction
{
public:
	void Draw() override
	{
		struct Btn { char key; int code; float ratio; unsigned int color; const char *name; };
		static const Btn btns[] = {
			{ 'A', 128, 1.0f, PackRGBA( 90, 210, 110, 255 ), "[A]" },
			{ 'B', 129, 1.0f, PackRGBA( 225, 80, 80, 255 ), "[B]" },
			{ 'X', 130, 1.0f, PackRGBA( 200, 205, 220, 255 ), "[X]" },
			{ 'Y', 131, 1.0f, PackRGBA( 225, 205, 90, 255 ), "[Y]" },
			{ 'S', 132, 2.4f, PackRGBA( 200, 200, 205, 255 ), "[Start]" },
			{ 'Z', 133, 1.35f, PackRGBA( 170, 120, 220, 255 ), "[Z]" },
			{ 'L', 134, 1.35f, PackRGBA( 220, 220, 225, 255 ), "[L]" },
			{ 'R', 135, 1.35f, PackRGBA( 220, 220, 225, 255 ), "[R]" },
		};
		const bool hires = EngFuncs::GetCvarFloat( "gc_hitext" ) != 0.0f;
		const char *p = szName;
		const int h = m_scChSize;
		int x = m_scPos.x;
		const int y = m_scPos.y;

		if( !p )
			return;
		while( *p )
		{
			char seg[96];
			int n = 0;

			if( p[0] == '{' && p[2] == '}' )
			{
				const Btn *b = nullptr;

				for( const Btn &c : btns )
					if( c.key == p[1] ) b = &c;
				if( b && hires )
				{
					const int w = (int)( h * b->ratio );

					EngFuncs::DrawCharacter( x, y, w, h, b->code, (int)b->color, 0 );
					x += w + h / 4;
					p += 3;
					continue;
				}
				if( b )
				{
					x += DrawSeg( x, y, b->name, b->color );
					p += 3;
					continue;
				}
			}
			while( p[n] && p[n] != '{' && n < (int)sizeof( seg ) - 1 )
			{
				seg[n] = p[n];
				n++;
			}
			if( !n ) { seg[n++] = *p; }	// a lone '{' that is not a token
			seg[n] = 0;
			p += n;
			x += DrawSeg( x, y, seg, colorBase );
		}
	}

private:
	int DrawSeg( int x, int y, const char *s, unsigned int color )
	{
		const int w = g_FontMgr->GetTextWideScaled( font, s, m_scChSize );

		UI_DrawString( font, Point( x, y ), Size( w + 4, m_scSize.h ), s, color, m_scChSize, QM_LEFT, ETF_SHADOW );
		return w;
	}
};
