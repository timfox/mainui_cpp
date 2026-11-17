/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "Framework.h"
#include "keydefs.h"
#include "Bitmap.h"
#include "Field.h"
#include "CheckBox.h"
#include "PicButton.h"
#include "Table.h"
#include "Action.h"
#include "YesNoMessageBox.h"
#include "Table.h"
#include <new>

#define ART_BANNER		"gfx/shell/head_creategame"

static const char *MAX_CLIENTS_STRING()
{
	static CUtlString maxClientsStr;
	maxClientsStr.Format( "%d", MAX_CLIENTS );
	return maxClientsStr.String();
}

struct map_t
{
	char name[CS_SIZE];
	char desc[CS_SIZE];
};

class CMenuCreateGame;
class CMenuMapListModel : public CMenuBaseModel, public CUtlVector<map_t>
{
public:
	CMenuMapListModel( CMenuCreateGame *parent ) : parent( parent ) { }

	void Update() override;
	int GetColumns() const override { return 2; }
	int GetRows() const override { return Count(); }
	const char *GetCellText( int line, int column ) override
	{
		switch( column )
		{
		case 0: return Element( line ).name;
		case 1: return Element( line ).desc;
		}

		return NULL;
	}

	CMenuCreateGame *parent;
};

/* Panel geometry shared by Draw() and mouse hit-testing, so the rows stay
 * evenly spaced at any screen size. Everything is derived from the panel rect;
 * nothing is allocated. */
struct CreateGameLayout
{
	int x, y, pw, ph;		// panel
	int labelX;			// left column (row labels)
	int ctlX, ctlW;			// right column (value boxes)
	int rowY[4];			// server name, map, max players, password
	int rowH;
};

static CreateGameLayout CreateGame_GetLayout( void )
{
	CreateGameLayout l;
	int w = gpGlobals->scrWidth, h = gpGlobals->scrHeight;
	l.x = w * 325 / 1000;
	l.y = h * 80 / 1000;
	l.pw = w * 350 / 1000;
	l.ph = h * 840 / 1000;
	l.labelX = l.x + 12;
	l.ctlX = l.x + l.pw * 38 / 100;
	l.ctlW = l.x + l.pw - 14 - l.ctlX;
	l.rowH = 19;

	// four rows spread over the body of the panel (below the tab strip, above
	// the buttons) with a capped pitch so a tall panel does not look sparse
	int bodyTop = l.y + 38 + 14, bodyH = l.ph - 91 - 28;
	int pitch = bodyH / 4;
	if( pitch > 34 ) pitch = 34;
	if( pitch < l.rowH + 6 ) pitch = l.rowH + 6;
	for( int i = 0; i < 4; ++i )
		l.rowY[i] = bodyTop + i * pitch;
	return l;
}

class CMenuCreateGame : public CMenuFramework
{
public:
	CMenuCreateGame() : CMenuFramework("CMenuCreateGame"), mapsListModel( this )
	{ }
	static void Begin( CMenuBaseItem *pSelf, void *pExtra );

	void Show() override;
	void Reload( void ) override;
	void SaveCvars( void );
	bool KeyDown( int key ) override;
	bool KeyUp( int key ) override;
	void Char( int key ) override;

	CMenuField	maxClients;
	CMenuField	hostName;
	CMenuField	password;

	// newgame prompt dialog
	CMenuYesNoMessageBox msgBox;

	CMenuTable        mapsList;
	CMenuMapListModel mapsListModel;

	CMenuPicButton *done;
	CMenuPicButton *cancel;
protected:
	void Draw() override;
private:
	void _Init() override;
	// Pad navigation: the page is drawn by hand, so it keeps its own cursor.
	// rows: 0 server name, 1 map, 2 password, 3 Start, 4 Cancel
	int row = 3;
	int editing = 0;		// 0 no, 1 server name, 2 password
	char oldText[UI_MAX_FIELD_LINE];
	CMenuField *EditField() { return editing == 1 ? &hostName : &password; }
	void BeginEdit( int what );
	void EndEdit( bool keep );
	void _VidInit() override;
};

/*
=================
CMenuCreateGame::Begin
=================
*/
void CMenuCreateGame::Begin( CMenuBaseItem *pSelf, void *pExtra )
{
	CMenuCreateGame *menu = (CMenuCreateGame*)pSelf->Parent();
	int item = menu->mapsList.GetCurrentIndex();
	if( !menu->mapsListModel.IsValidIndex( item ))
		return;

	if( item == 0 )
	{
		if( menu->mapsListModel.Count() <= 1 ) return;
		item = EngFuncs::RandomLong( 1, menu->mapsListModel.GetRows() - 1 );
	}

	const char *mapName = menu->mapsListModel[item].name;
	Con_Printf( "Xash3D GameCube: CreateGame Begin map=%s clients=%s active=%d\n",
		mapName, menu->maxClients.GetBuffer(), CL_IsActive() );

#if !XASH_GAMECUBE
	if( !EngFuncs::IsMapValid( mapName ))
		return;	// bad map
#endif

	const char *maxPlayersBuf = menu->maxClients.GetBuffer();
	int players = atoi( maxPlayersBuf );
	if( players < 2 || players > MAX_CLIENTS )
	{
		CUtlString msg;
		msg.Format( L( "Invalid maxplayers value entered.\nPlease enter a value from 2 to %d." ), MAX_CLIENTS );
		UI_ShowMessageBox( msg.String() );
		return;
	}
	if( EngFuncs::GetCvarFloat( "host_serverstate" ))
	{
		if( EngFuncs::GetCvarFloat( "maxplayers" ) == 1.0f )
			EngFuncs::HostEndGame( "end of the game" );
		else
			EngFuncs::HostEndGame( "starting new server" );
	}

	EngFuncs::CvarSetValue( "deathmatch", 1.0f );	// start deathmatch as default
	menu->SaveCvars();
	UI_SaveScriptConfig();
	UI_ApplyServerSettings();

	EngFuncs::PlayBackgroundTrack( NULL, NULL );

	// all done, start server
	const char *listenservercfg = EngFuncs::GetCvarString( "lservercfgfile" );

	char cmd[1024];
#if !XASH_GAMECUBE
	snprintf( cmd, sizeof( cmd ), "exec %s\n", listenservercfg );
	EngFuncs::ClientCmd( true, cmd );
#endif

	// dirty listenserver config form old xash may rewrite maxplayers
	menu->maxClients.WriteCvar();

	// wait three frames allowing server to completely shutdown, reapply maxplayers and start map
	char cmd2[256];
	Com_EscapeCommand( cmd2, mapName, sizeof( cmd2 ));
#if XASH_GAMECUBE
	/* Run the dedicated GameCube startup path now: it trims the menu/client
	 * allocations, paints the loading screen, then starts the local server. */
	snprintf( cmd, sizeof( cmd ), "gc_playstart %s\n", cmd2 );
	Con_Printf( "Xash3D GameCube: CreateGame direct play start map=%s\n", mapName );
	EngFuncs::ClientCmd( true, cmd );
	snprintf( cmd, sizeof( cmd ), "exec %s\n", listenservercfg );
	EngFuncs::ClientCmd( true, cmd );
#else
	snprintf( cmd, sizeof( cmd ), "disconnect;menu_connectionprogress localserver;wait;wait;wait;maxplayers %i;latch;map %s\n", atoi( menu->maxClients.GetBuffer() ), cmd2 );
	EngFuncs::ClientCmd( false, cmd );
#endif
#if XASH_GAMECUBE
	Con_Printf( "Xash3D GameCube: CreateGame start command returned map=%s\n", mapName );
#endif
}

/*
=================
CMenuMapListModel::Update
=================
*/
void CMenuMapListModel::Update( void )
{
	char *afile;

	if( !uiStatic.needMapListUpdate )
		return;

	RemoveAll();

	/* The disc staging step writes the exact DoD map list. Re-scanning BSPs
	 * on every Create Game open needlessly parses multi-megabyte lumps on GX. */
	const bool dodMaps = !stricmp( gMenu.m_gameinfo.gamefolder, "dod" );
	if(( !dodMaps && !EngFuncs::CreateMapsList( true ) )
		|| (afile = (char *)EngFuncs::COM_LoadFile( "maps.lst", NULL )) == NULL )
	{
		parent->done->SetGrayed( true );
		Con_Printf( "Cmd_GetMapsList: can't open maps.lst\n" );
		return;
	}

	{
		map_t map;
		Q_strncpy( map.name, "< Random Map >", sizeof( map.name ));
		Q_strncpy( map.desc, "", sizeof( map.desc ));
		AddToTail( map );
	}

	char *pfile = afile;
	char token[1024];

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ))) != NULL )
	{
		map_t map;

		Q_strncpy( map.name, token, sizeof( map.name ));
		if(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ))) == NULL )
		{
			Q_strncpy( map.desc, map.name, sizeof( map.desc ));
			AddToTail( map );
			break; // unexpected end of file
		}
		Q_strncpy( map.desc, token, sizeof( map.desc ));
		AddToTail( map );
	}

	if( Count( ) <= 1 )
		parent->done->SetGrayed( true );

	EngFuncs::COM_FreeFile( afile );
	uiStatic.needMapListUpdate = false;
}

/*
=================
CMenuCreateGame::Init
=================
*/
void CMenuCreateGame::_Init( void )
{
	uiStatic.needMapListUpdate = true;
	banner.SetPicture( ART_BANNER );

	// add them here, so "done" button can be used by mapsListModel::Update
	AddItem( banner );
	CMenuPicButton *advOpt = AddButton( L( "Adv. Options" ), nullptr, PC_ADV_OPT, UI_AdvServerOptions_Menu );
	advOpt->Hide();

	done = AddButton( "Start", nullptr, PC_OK, Begin );
	done->SetPicture( (const char *)NULL );
	done->onReleasedClActive = msgBox.MakeOpenEvent();

	mapsList.SetCharSize( QM_SMALLFONT );
	mapsList.SetupColumn( 0, L( "GameUI_Map" ), 0.5f );
	mapsList.SetupColumn( 1, L( "Title" ), 0.5f );
	mapsList.SetModel( &mapsListModel );

	hostName.szName = L( "GameUI_ServerName" );
	hostName.iMaxLength = 28;
	SET_EVENT_MULTI( hostName.onCvarGet,
	{
		CMenuField *self = (CMenuField*)pSelf;
		self->SetBuffer( UI_GetScriptCvar( self->CvarName() ) );
	});
	hostName.LinkCvar( "hostname" );

	maxClients.iMaxLength = 3;
	maxClients.bNumbersOnly = true;
	maxClients.szName = L( "GameUI_MaxPlayers" );
	SET_EVENT_MULTI( maxClients.onChanged,
	{
		CMenuField *self = (CMenuField*)pSelf;
		const char *buf = self->GetBuffer();
		if( buf[0] == 0 ) return;

		int players = atoi( buf );
		#if XASH_GAMECUBE
		if( players < 2 || players > 2 )
			self->SetBuffer( "2" ); // two BSS-backed clients fit the GameCube listen server
		#else
		if( players > MAX_CLIENTS )
			self->SetBuffer( MAX_CLIENTS_STRING() );
		#endif
	});
	SET_EVENT_MULTI( maxClients.onCvarGet,
	{
		CMenuField *self = (CMenuField*)pSelf;
		const char *val = UI_GetScriptCvar( self->CvarName() );
		self->SetBuffer( val );

		const char *buf = self->GetBuffer();

		int players = atoi( buf );
		#if XASH_GAMECUBE
		if( players != 2 )
			self->SetBuffer( "2" );
		#else
		if( players <= 1 )
			self->SetBuffer( "16" );
		else if( players > MAX_CLIENTS )
			self->SetBuffer( MAX_CLIENTS_STRING() );
		#endif
	});
	maxClients.LinkCvar( "maxplayers" );

	password.szName = L( "GameUI_Password" );
	password.iMaxLength = 16;
	password.eTextAlignment = QM_CENTER;
	password.bHideInput = true;
	SET_EVENT_MULTI( password.onCvarGet,
	{
		CMenuField *self = (CMenuField*)pSelf;
		self->SetBuffer( UI_GetScriptCvar( self->CvarName() ) );
	});
	password.LinkCvar( "sv_password" );

	msgBox.onPositive = Begin;
	msgBox.SetMessage( L( "Starting a new game will exit any current game, OK to exit?" ) );
	msgBox.Link( this );

	cancel = AddButton( "Cancel", nullptr, PC_CANCEL, VoidCb( &CMenuCreateGame::Hide ) );
	cancel->SetPicture( (const char *)NULL );
	AddItem( hostName );
	AddItem( maxClients );
	AddItem( password );
	AddItem( mapsList );
}

void CMenuCreateGame::_VidInit()
{
	mapsList.SetRect( 590, 230, -20, 465 );
	mapsList.Hide();

	hostName.SetRect( 350, 260, 205, 32 );
	maxClients.SetRect( 350, 360, 205, 32 );
	password.SetRect( 350, 460, 205, 32 );
	hostName.Hide(); maxClients.Hide(); password.Hide();
	banner.Hide();
	done->SetRect( 350, 535, 72, 38 );
	cancel->SetRect( 425, 535, 72, 38 );
	mapsList.Hide(); done->Hide(); cancel->Hide();
}

void CMenuCreateGame::Draw()
{
	int w = gpGlobals->scrWidth, h = gpGlobals->scrHeight;
	int x = w * 325 / 1000, y = h * 80 / 1000, pw = w * 350 / 1000, ph = h * 840 / 1000;
	EngFuncs::FillRGBA( x, y, pw, ph, 74, 86, 69, 255 );
	UI_DrawRectangleExt( x, y, pw, ph, PackRGBA( 139, 148, 127, 255 ), 1 );
	EngFuncs::FillRGBA( x + 4, y + 38, pw - 8, ph - 91, 72, 82, 66, 255 );
	UI_DrawRectangleExt( x + 4, y + 38, pw - 8, ph - 91, PackRGBA( 72, 82, 66, 255 ), 1 );
	int fs = 8;
	uint textFlags = ETF_FORCECOL | ETF_NOSIZELIMIT;
	UI_DrawString( uiStatic.hDefaultFont, x + 22, y + 7, pw - 24, 13, "Create Server", uiColorWhite, fs, QM_LEFT, textFlags );
	/* Retail's compact close control and map selector arrow are simple shell
	 * details. Draw them directly so this dialog stays asset-light on GC. */
	int closeX = x + pw - 12, closeY = y + 5;
	EngFuncs::FillRGBA( closeX, closeY, 9, 9, 105, 109, 100, 255 );
	UI_DrawRectangleExt( closeX, closeY, 9, 9, PackRGBA( 139, 148, 127, 255 ), 1 );
	for( int i = 0; i < 4; ++i )
	{
		EngFuncs::FillRGBA( closeX + 2 + i, closeY + 2 + i, 1, 1, 224, 224, 214, 255 );
		EngFuncs::FillRGBA( closeX + 6 - i, closeY + 2 + i, 1, 1, 224, 224, 214, 255 );
	}
	UI_DrawString( uiStatic.hDefaultFont, x + 8, y + 29, 56, 14, "Server", PackRGBA( 205, 190, 66, 255 ), fs, QM_LEFT, textFlags );
	UI_DrawString( uiStatic.hDefaultFont, x + 69, y + 32, 48, 13, "Game", uiColorWhite, fs, QM_LEFT, textFlags );
	UI_DrawRectangleExt( x + 5, y + 27, 60, 22, PackRGBA( 139, 148, 127, 255 ), 1 );
	CreateGameLayout L = CreateGame_GetLayout();
	const uint labelCol = PackRGBA( 205, 190, 66, 255 );
	static const char *const labels[4] = { "Server name", "Map", "Max players", "Password" };
	char passMask[17];
	int index = mapsList.GetCurrentIndex();
	const char *map = index <= 0 || !mapsListModel.IsValidIndex( index ) ? "< Random Map >" : mapsListModel.Element( index ).name;
	const char *pass = password.GetBuffer();
	int pn = 0;
	for( ; pass && pass[pn] && pn < 16; ++pn ) passMask[pn] = '*';
	passMask[pn] = 0;
	const char *values[4] = { hostName.GetBuffer(), map, maxClients.GetBuffer(), passMask };
	for( int i = 0; i < 4; ++i )
	{
		int ry = L.rowY[i];
		UI_DrawString( uiStatic.hDefaultFont, L.labelX, ry + 3, L.ctlX - L.labelX - 4, 13, labels[i], labelCol, fs, QM_LEFT, textFlags );
		EngFuncs::FillRGBA( L.ctlX, ry, L.ctlW, L.rowH, 57, 68, 54, 255 );
		UI_DrawRectangleExt( L.ctlX, ry, L.ctlW, L.rowH, PackRGBA( 139, 148, 127, 255 ), 1 );
		UI_DrawString( uiStatic.hDefaultFont, L.ctlX + 4, ry + 3, L.ctlW - 8, 13, values[i] ? values[i] : "", uiColorWhite, fs, QM_LEFT, textFlags );
	}
	// pad cursor: gold frame on the selected row, caret while editing
	{
		const uint gold = PackRGBA( 255, 210, 80, 255 );
		static const int rowToLayout[3] = { 0, 1, 3 };

		if( row <= 2 )
		{
			int ry = L.rowY[rowToLayout[row]];
			UI_DrawRectangleExt( L.ctlX - 2, ry - 2, L.ctlW + 4, L.rowH + 4, gold, 2 );
			if( editing )
				UI_DrawString( uiStatic.hDefaultFont, L.ctlX + L.ctlW - 12, ry + 3, 10, 13, "_", gold, fs, QM_LEFT, textFlags );
		}
	}
	// selector arrow on the map row
	int arrowX = L.ctlX + L.ctlW - 8, arrowY = L.rowY[1] + 8;
	for( int i = 0; i < 3; ++i )
		EngFuncs::FillRGBA( arrowX - i, arrowY + i, 1 + i * 2, 1, 174, 178, 165, 255 );
	int by = y + ph - 38, sx = x + pw - 76, kx = x + pw - 39;
	EngFuncs::FillRGBA( sx, by, 36, 16, 65, 77, 61, 255 ); EngFuncs::FillRGBA( kx, by, 36, 16, 65, 77, 61, 255 );
	UI_DrawRectangleExt( sx, by, 36, 16, PackRGBA( 139, 148, 127, 255 ), 1 ); UI_DrawRectangleExt( kx, by, 36, 16, PackRGBA( 139, 148, 127, 255 ), 1 );
	UI_DrawString( uiStatic.hDefaultFont, sx + 3, by + 2, 33, 12, "Start", uiColorWhite, 6, QM_LEFT, textFlags );
	UI_DrawString( uiStatic.hDefaultFont, kx + 3, by + 2, 33, 12, "Cancel", uiColorWhite, 6, QM_LEFT, textFlags );
	if( row >= 3 )
	{
		int fx = row == 3 ? sx : kx;
		UI_DrawRectangleExt( fx - 2, by - 2, 40, 20, PackRGBA( 255, 210, 80, 255 ), 2 );
	}
	// button hints, so the page has no dead ends
	UI_DrawString( uiStatic.hDefaultFont, x + 8, y + ph - 16, pw - 16, 12,
		editing ? "Keyboard: Start OK   Z cancel" : "A select  B back  D-pad move",
		PackRGBA( 190, 190, 170, 255 ), 8, QM_LEFT, textFlags );
}

void CMenuCreateGame::BeginEdit( int what )
{
	CreateGameLayout L = CreateGame_GetLayout();

	editing = what;
	Q_strncpy( oldText, EditField()->GetBuffer(), sizeof( oldText ));
	// tell the built-in keyboard where the field is and what it holds
	EngFuncs::SetTextInputRect( L.ctlX, L.rowY[what == 1 ? 0 : 3], L.ctlW, L.rowH );
	EngFuncs::SetTextInputBuffer( EditField()->GetBuffer() );
	UI_EnableTextInput( true );
}

void CMenuCreateGame::EndEdit( bool keep )
{
	if( !editing ) return;
	if( !keep ) EditField()->SetBuffer( oldText );
	editing = 0;
	UI_EnableTextInput( false );
}

void CMenuCreateGame::Char( int key )
{
	if( !editing || key < 32 || key > 126 ) return;

	CMenuField *f = EditField();
	char buf[UI_MAX_FIELD_LINE];
	size_t len = strlen( f->GetBuffer() );

	if( len + 1 >= sizeof( buf ) || (int)len >= f->iMaxLength ) return;
	Q_strncpy( buf, f->GetBuffer(), sizeof( buf ));
	buf[len] = (char)key;
	buf[len + 1] = 0;
	f->SetBuffer( buf );
}

bool CMenuCreateGame::KeyDown( int key )
{
	if( editing )
	{
		// text arrives through Char(); Enter ends the edit, Escape puts the old text back
		if( UI::Key::IsEscape( key ) ) { EndEdit( false ); return true; }
		if( UI::Key::IsEnter( key ) ) { EndEdit( true ); return true; }
		if( key == K_BACKSPACE )
		{
			CMenuField *f = EditField();
			char buf[UI_MAX_FIELD_LINE];
			size_t len = strlen( f->GetBuffer() );

			if( len > 0 )
			{
				Q_strncpy( buf, f->GetBuffer(), sizeof( buf ));
				buf[len - 1] = 0;
				f->SetBuffer( buf );
			}
			return true;
		}
		return true;
	}

	if( UI::Key::IsEscape( key ) || key == 'B' ) { Hide(); return true; }

	if( UI::Key::IsUpArrow( key ) ) { row = ( row + 4 ) % 5; return true; }
	if( UI::Key::IsDownArrow( key ) ) { row = ( row + 1 ) % 5; return true; }

	if( UI::Key::IsLeftArrow( key ) || UI::Key::IsRightArrow( key ) )
	{
		if( row >= 3 ) { row = row == 3 ? 4 : 3; return true; } // Start <-> Cancel
		int n = mapsListModel.Count(), i = mapsList.GetCurrentIndex();
		if( n > 0 ) mapsList.SetCurrentIndex( ( i + ( UI::Key::IsRightArrow( key ) ? 1 : n - 1 ) ) % n );
		return true;
	}
	if( UI::Key::IsEnter( key ) || key == 'A' )
	{
		switch( row )
		{
		case 0: BeginEdit( 1 ); return true;
		case 2: BeginEdit( 2 ); return true;
		case 1: // next map
		{
			int n = mapsListModel.Count(), i = mapsList.GetCurrentIndex();
			if( n > 0 ) mapsList.SetCurrentIndex( ( i + 1 ) % n );
			return true;
		}
		case 4: Hide(); return true;
		}
		Con_Printf( "Xash3D GameCube: CreateGame confirm active=%d\n", CL_IsActive() );
		if( CL_IsActive() ) msgBox.Show(); else Begin( done, NULL );
		return true;
	}
	return CMenuFramework::KeyDown( key );
}

bool CMenuCreateGame::KeyUp( int key )
{
	if( UI::Key::IsLeftMouse( key ) )
	{
		int x = gpGlobals->scrWidth * 325 / 1000, y = gpGlobals->scrHeight * 181 / 1000;
		int pw = gpGlobals->scrWidth * 350 / 1000, ph = gpGlobals->scrHeight * 639 / 1000, by = y + ph - 38;
		if( uiStatic.cursorX >= x + pw - 12 && uiStatic.cursorX <= x + pw - 3 &&
			uiStatic.cursorY >= y + 5 && uiStatic.cursorY <= y + 14 )
		{
			Hide();
			return true;
		}
		if( uiStatic.cursorY >= by && uiStatic.cursorY <= by + 16 )
		{
			if( uiStatic.cursorX >= x + pw - 76 && uiStatic.cursorX <= x + pw - 40 ) { if( CL_IsActive() ) msgBox.Show(); else Begin( done, NULL ); return true; }
			if( uiStatic.cursorX >= x + pw - 39 && uiStatic.cursorX <= x + pw - 3 ) { Hide(); return true; }
		}
		CreateGameLayout L = CreateGame_GetLayout();
		int mx = L.ctlX, my = L.rowY[1], mw = L.ctlW, n = mapsListModel.Count();
		if( n > 0 && uiStatic.cursorX >= mx && uiStatic.cursorX <= mx + mw && uiStatic.cursorY >= my && uiStatic.cursorY <= my + L.rowH )
		{
			int i = mapsList.GetCurrentIndex();
			mapsList.SetCurrentIndex( ( i + ( uiStatic.cursorX > mx + mw / 2 ? 1 : n - 1 ) ) % n );
			return true;
		}
	}
	return CMenuFramework::KeyUp( key );
}

void CMenuCreateGame::Show()
{
	UI_LoadScriptConfig();

	hostName.UpdateCvar( true );
	maxClients.UpdateCvar( true );
	password.UpdateCvar( true );

	row = 3;
	editing = 0;

	CMenuBaseWindow::Show();
}

void CMenuCreateGame::SaveCvars()
{
	hostName.WriteCvar();
	maxClients.WriteCvar();
	password.WriteCvar();

	// update script variables so they are saved to settings.scr
	UI_SetScriptCvar( "hostname", hostName.GetBuffer() );
	UI_SetScriptCvar( "maxplayers", maxClients.GetBuffer() );
	UI_SetScriptCvar( "sv_password", password.GetBuffer() );

	EngFuncs::CvarSetValue( "sv_nat", 0 );	// Steam networking is not available on GameCube
}

void CMenuCreateGame::Reload( void )
{
	mapsListModel.Update();
	mapsList.SetCurrentIndex( 0 );
}

/* Keep storage for this 6.5 KiB form out of the small GameCube menu heap.
 * Construct it on first use, after the UI runtime has initialized. */
alignas( CMenuCreateGame ) static unsigned char menuCreateGameStorage[sizeof( CMenuCreateGame )];
static CMenuCreateGame *menu_creategame = NULL;

static void menu_creategame_Precache( void )
{
	if( !menu_creategame )
	{
		menu_creategame = new ( menuCreateGameStorage ) CMenuCreateGame();
	}
}

static void menu_creategame_Shutdown( void )
{
	if( menu_creategame )
	{
		menu_creategame->~CMenuCreateGame();
		menu_creategame = NULL;
	}
}

ADD_MENU4( menu_creategame, menu_creategame_Precache, UI_CreateGame_Menu, menu_creategame_Shutdown );

void UI_CreateGame_Menu( void )
{
	menu_creategame_Precache();
	menu_creategame->Show();
}
