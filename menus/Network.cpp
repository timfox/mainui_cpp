/*
Network.cpp -- GameCube network configuration (DHCP / static IP, DNS)
Writes the archived gc_net_* cvars read by the engine's GameCube network
bring-up. Values apply when the link is first brought up (first multiplayer
use); a reboot is needed to change them after a failed or completed bring-up.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "Framework.h"
#include "keydefs.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "CheckBox.h"
#include "Field.h"
#include "Action.h"
#include "PagePanel.h"
#include "HintRow.h"

#define ART_BANNER "gfx/shell/head_advoptions"

class CMenuNetwork : public CMenuFramework
{
public:
	CMenuNetwork() : CMenuFramework( "CMenuNetwork" ) { }
	bool KeyDown( int key ) override;
	void Show() override;

private:
	void _Init() override;
	void SaveCb();
	void RestoreCb();
	void Restore();
	void UpdateEnabled();

	CMenuPicButton *doneBtn = nullptr;
	CMenuPagePanel panel;
	CMenuCheckBox dhcp;
	CMenuField    ip, mask, gateway, dns;
	CMenuAction   status, note;
	CMenuHintRow  hint;
};

bool CMenuNetwork::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ))
		Restore();
	return CMenuFramework::KeyDown( key );
}

void CMenuNetwork::Show()
{
#if XASH_GAMECUBE
	// the pad has no pointer, but the virtual cursor still hovers: park it off the fields so it does not
	// grab focus (and open the keyboard) the moment the page opens
	uiStatic.cursorX = 4;
	uiStatic.cursorY = gpGlobals->scrHeight - 4;
#endif
	CMenuFramework::Show();
	// start on Done: a focused field opens the keyboard over the page
	if( doneBtn ) SetCursorToItem( *doneBtn );
	status.szName = EngFuncs::GetCvarString( "gc_net_status" );
}

// static address fields only matter when DHCP is off
void CMenuNetwork::UpdateEnabled()
{
	if( dhcp.bChecked )
	{
		ip.iFlags |= QMF_GRAYED;
		mask.iFlags |= QMF_GRAYED;
		gateway.iFlags |= QMF_GRAYED;
	}
	else
	{
		ip.iFlags &= ~QMF_GRAYED;
		mask.iFlags &= ~QMF_GRAYED;
		gateway.iFlags &= ~QMF_GRAYED;
	}
}

void CMenuNetwork::SaveCb()
{
	dhcp.WriteCvar();
	ip.WriteCvar();
	mask.WriteCvar();
	gateway.WriteCvar();
	dns.WriteCvar();
	SaveAndPopMenu();
}

void CMenuNetwork::Restore()
{
	dhcp.DiscardChanges();
	ip.DiscardChanges();
	mask.DiscardChanges();
	gateway.DiscardChanges();
	dns.DiscardChanges();
}

void CMenuNetwork::RestoreCb()
{
	Restore();
	Hide();
}

void CMenuNetwork::_Init()
{
	banner.SetPicture( ART_BANNER );

	dhcp.SetNameAndStatus( L( "Automatic address (DHCP)" ), L( "Get the IP address from your router. Turn off to enter one by hand" ));
	dhcp.LinkCvar( "gc_net_dhcp" );
	PAGE_BIG( dhcp );
	dhcp.SetCoord( 340, 165 );
	SET_EVENT_MULTI( dhcp.onChanged,
	{
		pSelf->GetParent( CMenuNetwork )->UpdateEnabled();
	});

	ip.SetNameAndStatus( L( "IP address" ), L( "Console address, for example 192.168.1.50" ));
	PAGE_BIG( ip );
	ip.LinkCvar( "gc_net_ip" );
	ip.SetRect( 340, 265, 560, 46 );

	mask.SetNameAndStatus( L( "Subnet mask" ), L( "Usually 255.255.255.0" ));
	PAGE_BIG( mask );
	mask.LinkCvar( "gc_net_mask" );
	mask.SetRect( 340, 360, 560, 46 );

	gateway.SetNameAndStatus( L( "Gateway" ), L( "Your router address, for example 192.168.1.1" ));
	PAGE_BIG( gateway );
	gateway.LinkCvar( "gc_net_gw" );
	gateway.SetRect( 340, 455, 560, 46 );

	dns.SetNameAndStatus( L( "DNS server (optional)" ), L( "Used to find internet servers. Empty = gateway, then 1.1.1.1 and 8.8.8.8" ));
	PAGE_BIG( dns );
	dns.LinkCvar( "gc_net_dns" );
	dns.SetRect( 340, 550, 560, 46 );

	status.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	status.colorBase = uiColorHelp;
	PAGE_BIG( status );
	status.SetRect( 340, 610, 600, 34 );

	note.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	note.colorBase = uiColorHelp;
	PAGE_BIG( note );
	note.szName = L( "Applies at next start" );
	note.SetRect( 340, 646, 600, 34 );

	hint.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	hint.colorBase = uiColorHelp;
	PAGE_BIG( hint );
	hint.szName = L( "{A} edit    {B} back" );
	hint.SetRect( 340, 684, 600, 34 );

	AddItem( banner );
	AddItem( panel );
	doneBtn = AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuNetwork::SaveCb ));
	AddButton( L( "GameUI_Cancel" ), nullptr, PC_CANCEL, VoidCb( &CMenuNetwork::RestoreCb ));
	AddItem( dhcp );
	AddItem( ip );
	AddItem( mask );
	AddItem( gateway );
	AddItem( dns );
	AddItem( status );
	AddItem( note );
	AddItem( hint );

	UpdateEnabled();
}

ADD_MENU( menu_network, CMenuNetwork, UI_Network_Menu );
