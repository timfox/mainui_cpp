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
	// x, y, w, h in menu coordinates (1024x768). The default fits the 5% title-safe area (x 52..972, y 38..730).
	CMenuPagePanel( int x = 310, int y = 120, int w = 660, int h = 600 ) : px( x ), py( y ), pw( w ), ph( h ) { iFlags = QMF_INACTIVE; }
	void Draw() override
	{
		const int x = (int)( px * uiStatic.scaleX ), y = (int)( py * uiStatic.scaleY );
		const int w = (int)( pw * uiStatic.scaleX ), h = (int)( ph * uiStatic.scaleY );

		EngFuncs::FillRGBA( x, y, w, h, 55, 43, 29, 235 );
		UI_DrawRectangleExt( x, y, w, h, PackRGBA( 175, 127, 1, 255 ), 1 );
	}
private:
	int px, py, pw, ph;
};

// big text on a page item: the GameCube only has the default menu font handle, so keep it and raise the size
#define PAGE_BIG( item ) do { (item).SetCharSize( QM_BIGFONT ); (item).font = uiStatic.hDefaultFont; } while( 0 )
