/*
PadLayout.cpp -- GameCube controller layout (Options > Controls)
A read-only list of what each button does. The PC page is a keyboard and mouse binding table.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "Framework.h"
#include "keydefs.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Action.h"
#include "PagePanel.h"

#define ART_BANNER "gfx/shell/head_customize"

class CMenuPadLayout : public CMenuFramework
{
public:
	CMenuPadLayout() : CMenuFramework( "CMenuPadLayout" ) { }

private:
	void _Init() override;

	static const int LINES = 13; // 13 rows of 40 units fit the panel (190..710)
	CMenuPagePanel panel;
	CMenuAction button[LINES], action[LINES];
};

void CMenuPadLayout::_Init()
{
	// docs/GAMECUBE_CONTROLS.md
	static const char *const rows[LINES][2] =
	{
		{ "Main stick", "Move" },
		{ "C-stick", "Look" },
		{ "A", "Jump / OK" },
		{ "B", "Use / back" },
		{ "X", "Reload" },
		{ "Y", "Voice menu" },
		{ "Z", "Duck" },
		{ "Z + Y", "Chat" },
		{ "R", "Fire" },
		{ "L", "Alt fire" },
		{ "D-pad U/D", "Weapons" },
		{ "D-pad right", "Scoreboard" },
		{ "Start", "Menu" },
	};

	banner.SetPicture( ART_BANNER );
	AddItem( banner );
	AddItem( panel );
	AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuPadLayout::Hide ));

	for( int i = 0; i < LINES; i++ )
	{
		button[i].iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
		button[i].colorBase = uiColorHelp;
		PAGE_BIG( button[i] );
		button[i].szName = rows[i][0];
		button[i].SetRect( 330, 190 + i * 40, 260, 36 );

		action[i].iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
		PAGE_BIG( action[i] );
		action[i].szName = rows[i][1];
		action[i].SetRect( 600, 190 + i * 40, 360, 36 );

		AddItem( button[i] );
		AddItem( action[i] );
	}
}

ADD_MENU( menu_padlayout, CMenuPadLayout, UI_PadLayout_Menu );
