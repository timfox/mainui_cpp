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

#define ART_BANNER "gfx/shell/head_customize"

class CMenuPadLayout : public CMenuFramework
{
public:
	CMenuPadLayout() : CMenuFramework( "CMenuPadLayout" ) { }

private:
	void _Init() override;

	static const int LINES = 13;
	CMenuAction button[LINES], action[LINES];
};

void CMenuPadLayout::_Init()
{
	// docs/GAMECUBE_CONTROLS.md
	static const char *const rows[LINES][2] =
	{
		{ "Main stick", "Move" },
		{ "C-stick", "Look" },
		{ "A", "Jump / confirm" },
		{ "B", "Use / back" },
		{ "X", "Reload" },
		{ "Y", "Voice menu" },
		{ "Z", "Duck" },
		{ "Z + Y", "Chat" },
		{ "R", "Fire" },
		{ "L", "Alt fire" },
		{ "D-pad up / down", "Weapons" },
		{ "D-pad right", "Scoreboard" },
		{ "Start", "Menu" },
	};

	banner.SetPicture( ART_BANNER );
	AddItem( banner );
	AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuPadLayout::Hide ));

	for( int i = 0; i < LINES; i++ )
	{
		button[i].iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
		button[i].colorBase = uiColorHelp;
		button[i].SetCharSize( QM_DEFAULTFONT );
		button[i].szName = rows[i][0];
		button[i].SetRect( 300, 210 + i * 36, 260, 30 );

		action[i].iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
		action[i].SetCharSize( QM_DEFAULTFONT );
		action[i].szName = rows[i][1];
		action[i].SetRect( 580, 210 + i * 36, 320, 30 );

		AddItem( button[i] );
		AddItem( action[i] );
	}
}

ADD_MENU( menu_padlayout, CMenuPadLayout, UI_PadLayout_Menu );
