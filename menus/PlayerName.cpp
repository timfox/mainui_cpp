/*
PlayerName.cpp -- GameCube player name page (Options > Player)
Edits the "name" cvar with the built-in keyboard. The full PC player setup page
needs a model preview and logo picker, which the GameCube does not have memory for.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "Framework.h"
#include "keydefs.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Field.h"
#include "Action.h"
#include "PagePanel.h"

#define ART_BANNER "gfx/shell/head_customize"

class CMenuPlayerName : public CMenuFramework
{
public:
	CMenuPlayerName() : CMenuFramework( "CMenuPlayerName" ) { }
	bool KeyDown( int key ) override;

private:
	void _Init() override;
	void SaveCb();
	void RestoreCb();

	CMenuPagePanel panel;
	CMenuField  name;
	CMenuAction hint;
};

bool CMenuPlayerName::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ))
		name.DiscardChanges();
	return CMenuFramework::KeyDown( key );
}

void CMenuPlayerName::SaveCb()
{
	if( !UI::Names::CheckIsNameValid( name.GetBuffer() ))
		return; // stay on the page, an empty or reserved name is not saved
	name.WriteCvar();
	SaveAndPopMenu();
}

void CMenuPlayerName::RestoreCb()
{
	name.DiscardChanges();
	Hide();
}

void CMenuPlayerName::_Init()
{
	banner.SetPicture( ART_BANNER );

	name.SetNameAndStatus( L( "GameUI_PlayerName" ), L( "The name other players see. A edit, then Done" ));
	name.iMaxLength = 32;
	PAGE_BIG( name );
	name.LinkCvar( "name" );
	name.SetRect( 340, 300, 560, 52 );

	hint.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	hint.colorBase = uiColorHelp;
	PAGE_BIG( hint );
	hint.szName = L( "A edit    B back" );
	hint.SetRect( 340, 380, 600, 40 );

	AddItem( banner );
	AddItem( panel );
	AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuPlayerName::SaveCb ));
	AddButton( L( "GameUI_Cancel" ), nullptr, PC_CANCEL, VoidCb( &CMenuPlayerName::RestoreCb ));
	AddItem( name );
	AddItem( hint );
}

ADD_MENU( menu_playername, CMenuPlayerName, UI_PlayerName_Menu );
