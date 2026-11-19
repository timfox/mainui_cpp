/*
HintRow.h -- a hint line with controller button glyphs: "{A} join  {B} back  {X} refresh".

{A} {B} {X} {Y} {S} (Start) {Z} {L} {R} become the button glyphs the engine's hi-res text atlas carries (codes 128..135,
scripts/bake-hitext.py, original art). When the atlas is not in use (gc_hitext 0) the token is drawn as "[A]" text.
*/
#pragma once
#include "Action.h"
#include "Utils.h"
#include "FontManager.h"

// Draws str (with {A}-style tokens) at screen pixel x, y with character height h; returns the width drawn.
inline int UI_DrawButtonText( HFont font, int x, int y, int h, const char *str, unsigned int color, uint flags = ETF_SHADOW )
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
	const char *p = str;
	const int x0 = x;

	if( !p )
		return 0;
	while( *p )
	{
		char seg[96];
		int n = 0;
		const char *text = nullptr;
		unsigned int tcol = color;

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
				text = b->name;
				tcol = b->color;
				p += 3;
			}
		}
		if( !text )
		{
			while( p[n] && p[n] != '{' && n < (int)sizeof( seg ) - 1 )
			{
				seg[n] = p[n];
				n++;
			}
			if( !n ) { seg[n++] = *p; }	// a lone '{' that is not a token
			seg[n] = 0;
			p += n;
			text = seg;
		}
		{
			const int w = g_FontMgr->GetTextWideScaled( font, text, h );

			UI_DrawString( font, Point( x, y ), Size( w + 4, h * 2 ), text, tcol, h, QM_LEFT, flags );
			x += w;
		}
	}
	return x - x0;
}

class CMenuHintRow : public CMenuAction
{
public:
	void Draw() override
	{
		UI_DrawButtonText( font, m_scPos.x, m_scPos.y, m_scChSize, szName, colorBase );
	}
};
