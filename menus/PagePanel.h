/*
PagePanel.h -- the retail-palette panel GameCube sub-pages put behind their items, so text never sits on the artwork.
ViewportBG 55 43 29, ClassMenuLight 175 127 1 (from the retail DoD resource/ClientScheme.res).
*/
#pragma once
#include "BaseItem.h"
#include "Utils.h"

class CMenuPagePanel : public CMenuBaseItem
{
public:
	// x, y, w, h in thousandths of the screen
	CMenuPagePanel( int x = 300, int y = 150, int w = 680, int h = 810 ) : px( x ), py( y ), pw( w ), ph( h ) { iFlags = QMF_INACTIVE; }
	void Draw() override
	{
		const int sw = gpGlobals->scrWidth, sh = gpGlobals->scrHeight;
		const int x = sw * px / 1000, y = sh * py / 1000, w = sw * pw / 1000, h = sh * ph / 1000;

		EngFuncs::FillRGBA( x, y, w, h, 55, 43, 29, 235 );
		UI_DrawRectangleExt( x, y, w, h, PackRGBA( 175, 127, 1, 255 ), 1 );
	}
private:
	int px, py, pw, ph;
};

// big text on a page item: the GameCube only has the default menu font handle, so keep it and raise the size
#define PAGE_BIG( item ) do { (item).SetCharSize( QM_BIGFONT ); (item).font = uiStatic.hDefaultFont; } while( 0 )
