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
#include "Action.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "MenuStrings.h"
#include "PlayerIntroduceDialog.h"
#include "gameinfo.h"
#include "AnimatedBanner.h"
#include "MovieBanner.h"

#define ART_MINIMIZE_N	"gfx/shell/min_n"
#define ART_MINIMIZE_F	"gfx/shell/min_f"
#define ART_MINIMIZE_D	"gfx/shell/min_d"
#define ART_CLOSEBTN_N	"gfx/shell/cls_n"
#define ART_CLOSEBTN_F	"gfx/shell/cls_f"
#define ART_CLOSEBTN_D	"gfx/shell/cls_d"

static bool UI_IsDayOfDefeat()
{
	return !stricmp( gMenu.m_gameinfo.gamefolder, "dod" )
		|| !stricmp( gMenu.m_gameinfo.title, "Day of Defeat" );
}

/*
=================
MOHAA room menu

When the game folder has resource/gc_menu/mohaa_main.txt (written onto the SD card by mohaa-gc's
converter from the player's own Medal of Honor: Allied Assault), the main menu is MOHAA's retail
one: the war room is the background, and parts of the room are the buttons. The part under the
cursor lights up and the green sign names it, as in ui/main.urc. Without the file the stock menu
shows.

	screen 640 480                      the room's screen (rects below are on it)
	sign x y w h                        where the sign art goes
	spot <action> x y w h hover sign    a part of the room: its lit image, its sign ('-': none)
	button <action> x y w h pic focus   a picture button (Quit, Back to game)
	sound launch|out <path>             the menu's click sounds
=================
*/
#define MOHAA_ROOM_LAYOUT	"resource/gc_menu/mohaa_main.txt"
#define MOHAA_ROOM_MAX_SPOTS	10

struct mohaaRect_t
{
	int x, y, w, h;
};

// The room drawn the way CMenuBackgroundBitmap draws the background sheet: scaled to cover the
// screen keeping its aspect (or stretched, with ui_background_stretch), centred.
static void UI_MohaaRoomToScreen( const mohaaRect_t &r, int roomW, int roomH, Point &pt, Size &sz )
{
	float xScale, yScale;
	int xOffset = 0, yOffset = 0;

	if( roomW <= 0 || roomH <= 0 )
		roomW = 640, roomH = 480;
	if( EngFuncs::GetCvarFloat( "ui_background_stretch" ))
	{
		xScale = (float)ScreenWidth / roomW;
		yScale = (float)ScreenHeight / roomH;
	}
	else
	{
		if( ScreenWidth * roomH > ScreenHeight * roomW )
			xScale = yScale = (float)ScreenWidth / roomW;
		else
			xScale = yScale = (float)ScreenHeight / roomH;
		xOffset = (int)(( ScreenWidth - roomW * xScale ) / 2 );
		yOffset = (int)(( ScreenHeight - roomH * yScale ) / 2 );
	}
	pt = Point( xOffset + (int)( r.x * xScale ), yOffset + (int)( r.y * yScale ));
	sz = Size( (int)( r.w * xScale + 0.5f ), (int)( r.h * yScale + 0.5f ));
}

#define MOHAA_ROOM_NAV_REPEAT_MS 150

class CMenuMohaaSpot : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuMohaaSpot() : BaseClass(), isButton( false ), loaded( false ), roomW( 640 ), roomH( 480 )
	{
		rect.x = rect.y = rect.w = rect.h = 0;
		signRect = rect;
		picPath[0] = focusPath[0] = '\0';
	}

	void VidInit( void ) override
	{
		iFlags |= QMF_DISABLESCAILING;
		UI_MohaaRoomToScreen( rect, roomW, roomH, pos, size );
		UI_MohaaRoomToScreen( signRect, roomW, roomH, signPos, signSize );
		BaseClass::VidInit();
	}

#if XASH_GAMECUBE
	// The pad drives the room: no pointer may take focus. The map and the war records desk
	// overlap in ui/main.urc, and a cursor update landing in the overlap pulled focus off the
	// map onto the desk, so the D-pad never stopped on New Game.
	bool MouseMove( int x, int y ) override
	{
		static int logs;
		if( logs < 8 )
		{
			logs++;
			Con_DPrintf( "MOHAA main menu: pointer at %d %d ignored on %s\n", x, y, picPath );
		}
		return false;
	}
#endif

	bool KeyDown( int key ) override
	{
		if(( UI::Key::IsEnter( key ) && !( iFlags & QMF_MOUSEONLY ))
			|| ( UI::Key::IsLeftMouse( key ) && ( iFlags & QMF_HASMOUSEFOCUS )))
		{
			_Event( QM_RELEASED );
			return true;
		}
		return false;
	}

	bool KeyUp( int key ) override
	{
		if(( UI::Key::IsEnter( key ) && !( iFlags & QMF_MOUSEONLY ))
			|| ( UI::Key::IsLeftMouse( key ) && ( iFlags & QMF_HASMOUSEFOCUS )))
		{
			_Event( QM_PRESSED );
			PlayLocalSound( uiStatic.sounds[SND_LAUNCH] );
			return true;
		}
		return false;
	}

	void Draw( void ) override
	{
		const bool focused = this == m_pParent->ItemAtCursor() && !( iFlags & QMF_GRAYED );

		if( isButton )
		{
			if( !loaded )
			{
				pic.Load( picPath );
				focusPic.Load( focusPath );
				loaded = true;
			}
			UI_DrawPic( m_scPos, m_scSize, uiColorWhite, focused && focusPic.IsValid() ? focusPic : pic );
			return;
		}

		// A spot's art is only in memory while it is lit: one hover image and one sign at a time.
		if( !focused )
		{
			if( loaded )
			{
				pic.ForceUnload();
				focusPic.ForceUnload();
				loaded = false;
			}
			return;
		}
		if( !loaded )
		{
			pic.Load( picPath );
			if( focusPath[0] )
				focusPic.Load( focusPath );
			loaded = true;
		}
		if( pic.IsValid( ))
			UI_DrawPic( m_scPos, m_scSize, uiColorWhite, pic );
		if( focusPic.IsValid( ) && signSize.w > 0 )
			UI_DrawPic( signPos, signSize, uiColorWhite, focusPic );
	}

	bool isButton;
	bool loaded;
	int roomW, roomH;
	mohaaRect_t rect, signRect;
	char picPath[96];	// spot: lit image; button: normal
	char focusPath[96];	// spot: sign; button: focused
	CImage pic, focusPic;
	Point signPos;
	Size signSize;
};

class CMenuMain: public CMenuFramework
{
public:
	CMenuMain() : CMenuFramework( "CMenuMain" ) { }

	bool KeyDown( int key ) override;
#if XASH_GAMECUBE
	void Draw() override;
#endif

private:
	void _Init() override;
	void _VidInit( ) override;
	void Think() override;

	void VidInit(bool connected);

	void QuitDialogCb();
	void DisconnectCb();
	void DisconnectDialogCb();
	void HazardCourseDialogCb();
	void HazardCourseCb();

	CMenuAnimatedBanner animatedBanner;
	CMenuMovieBanner movieBanner;
#if XASH_GAMECUBE
	CMenuBitmap	titleLogo;
#endif

	CMenuPicButton	console;
	CMenuPicButton	resumeGame;
	CMenuPicButton	disconnect;
	CMenuPicButton	newGame;
	CMenuPicButton	hazardCourse;
	CMenuPicButton	configuration;
	CMenuPicButton	saveRestore;
	CMenuPicButton	multiPlayer;
	CMenuPicButton	customGame;
	CMenuPicButton	previews;
	CMenuPicButton	quit;

	// buttons on top right. Maybe should be drawn if fullscreen == 1?
	CMenuBitmap	minimizeBtn;
	CMenuBitmap	quitButton;

	// quit dialog
	CMenuYesNoMessageBox dialog;

	bool bTrainMap;
	bool bCustomGame;

	// MOHAA room menu (resource/gc_menu/mohaa_main.txt)
	bool LoadMohaaRoom( void );
	void MohaaVidInit( bool connected );
	void MohaaAction( CMenuMohaaSpot &item, const char *action );
	bool mohaaRoom;
	int mohaaSpotCount;
	CMenuMohaaSpot mohaaSpots[MOHAA_ROOM_MAX_SPOTS];
	CMenuMohaaSpot mohaaQuit;
	CMenuMohaaSpot mohaaResume;
};

void CMenuMain::MohaaAction( CMenuMohaaSpot &item, const char *action )
{
	if( !stricmp( action, "newgame" ))
		item.onReleased = ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY || UI_IsDayOfDefeat() || !gMenu.m_gameinfo.startmap[0] )
			? UI_CreateGame_Menu : UI_NewGame_Menu;
	else if( !stricmp( action, "multiplayer" ))
		item.onReleased = UI_InternetGames_Menu;
	else if( !stricmp( action, "options" ))
		item.onReleased = UI_Options_Menu;
	else if( !stricmp( action, "records" ))
	{
		// War records: MOHAA's load/save and medal case. Saves need the game dll.
		item.onReleased = UI_SaveLoad_Menu;
		if( !EngFuncs::CheckGameDll() || gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
			item.SetGrayed( true );
	}
	else if( !stricmp( action, "credits" ))
		item.onReleased = UI_Credits_Menu;
	else if( !stricmp( action, "quit" ))
		item.onReleased = VoidCb( &CMenuMain::QuitDialogCb );
	else if( !stricmp( action, "resume" ))
		item.onReleased = UI_CloseMenu;
	else
		item.SetGrayed( true );
}

bool CMenuMain::LoadMohaaRoom( void )
{
	char *afile, *pfile;
	char token[256];
	mohaaRect_t sign = { 0, 0, 0, 0 };
	int roomW = 640, roomH = 480;

	mohaaRoom = false;
	mohaaSpotCount = 0;
	if( !EngFuncs::FileExists( MOHAA_ROOM_LAYOUT ))
		return false;
	afile = (char *)EngFuncs::COM_LoadFile( MOHAA_ROOM_LAYOUT );
	if( !afile )
		return false;

	pfile = afile;
	while(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ))))
	{
		if( !strcmp( token, "screen" ) || !strcmp( token, "sign" ))
		{
			const bool isSign = token[1] == 'i';
			int v[4] = { 0, 0, 0, 0 };
			for( int i = 0; i < ( isSign ? 4 : 2 ) && pfile; i++ )
			{
				pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ));
				v[i] = atoi( token );
			}
			if( isSign )
				sign.x = v[0], sign.y = v[1], sign.w = v[2], sign.h = v[3];
			else if( v[0] > 0 && v[1] > 0 )
				roomW = v[0], roomH = v[1];
		}
		else if( !strcmp( token, "spot" ) || !strcmp( token, "button" ))
		{
			const bool isButton = token[0] == 'b';
			char action[32];
			mohaaRect_t r;
			char pic[96], focus[96];

			if( !( pfile = EngFuncs::COM_ParseFile( pfile, action, sizeof( action )))) break;
			if( !( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token )))) break;
			r.x = atoi( token );
			if( !( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token )))) break;
			r.y = atoi( token );
			if( !( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token )))) break;
			r.w = atoi( token );
			if( !( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token )))) break;
			r.h = atoi( token );
			if( !( pfile = EngFuncs::COM_ParseFile( pfile, pic, sizeof( pic )))) break;
			if( !( pfile = EngFuncs::COM_ParseFile( pfile, focus, sizeof( focus )))) break;
			if( !strcmp( focus, "-" ))
				focus[0] = '\0';

			CMenuMohaaSpot *item;
			if( isButton && !stricmp( action, "quit" ))
				item = &mohaaQuit;
			else if( isButton && !stricmp( action, "resume" ))
				item = &mohaaResume;
			else if( !isButton && mohaaSpotCount < MOHAA_ROOM_MAX_SPOTS )
				item = &mohaaSpots[mohaaSpotCount++];
			else
				continue;
			item->isButton = isButton;
			item->rect = r;
			Q_strncpy( item->picPath, pic, sizeof( item->picPath ));
			Q_strncpy( item->focusPath, focus, sizeof( item->focusPath ));
			item->szName = item->picPath;
			MohaaAction( *item, action );
		}
		else if( !strcmp( token, "sound" ))
		{
			char which[16];
			if( !( pfile = EngFuncs::COM_ParseFile( pfile, which, sizeof( which )))) break;
			if( !( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token )))) break;
			int snd = !stricmp( which, "launch" ) ? SND_LAUNCH : !stricmp( which, "out" ) ? SND_OUT : -1;
			if( snd >= 0 && EngFuncs::FileExists( token ))
				Q_strncpy( uiStatic.sounds[snd], token, sizeof( uiStatic.sounds[snd] ));
		}
		// anything else ("//" comments are skipped by the parser): ignore the token
	}
	EngFuncs::COM_FreeFile( afile );

	if( !mohaaSpotCount )
		return false;
	for( int i = 0; i < mohaaSpotCount; i++ )
	{
		mohaaSpots[i].signRect = sign;
		mohaaSpots[i].roomW = roomW, mohaaSpots[i].roomH = roomH;
	}
	mohaaQuit.roomW = mohaaResume.roomW = roomW;
	mohaaQuit.roomH = mohaaResume.roomH = roomH;
	mohaaRoom = true;
	Con_Printf( "MOHAA main menu: %d room spots from " MOHAA_ROOM_LAYOUT "\n", mohaaSpotCount );
	return true;
}

void CMenuMain::MohaaVidInit( bool connected )
{
	// Back to game only while a game runs; Quit only from the front (retail keeps both, but the
	// GameCube has no desktop to quit to while playing).
	mohaaResume.SetVisibility( connected && mohaaResume.picPath[0] );
	mohaaQuit.SetVisibility( !connected && mohaaQuit.picPath[0] );
}

#if XASH_GAMECUBE
// The main menu labels keep the look they have always had: no hardware resolution text on this page.
void CMenuMain::Draw()
{
	EngFuncs::CvarSetValue( "gc_hitext_pause", 1.0f );
	CMenuFramework::Draw();
	EngFuncs::CvarSetValue( "gc_hitext_pause", 0.0f );
}
#endif


void CMenuMain::QuitDialogCb()
{
	if( CL_IsActive() && EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) == 1.0f )
	{
		const char *ingame = L( "StringsList_235" );
		/* StringsList_235 may be empty before alias fill; never show the key. */
		if( !ingame || !ingame[0] || !strcmp( ingame, "StringsList_235" ))
			ingame = "Quit without saving the current game?";
		dialog.SetMessage( ingame );
	}
	else
	{
		const char *confirm = L( "GameUI_QuitConfirmationText" );
		if( !confirm || !strcmp( confirm, "GameUI_QuitConfirmationText" ))
			confirm = "Do you wish to stop playing now?";
		dialog.SetMessage( confirm );
	}

	dialog.onPositive.SetCommand( false, "quit\n" );
	dialog.Show();
}

void CMenuMain::DisconnectCb()
{
	EngFuncs::ClientCmd( false, "disconnect\n" );
	VidInit( false );
	CalcPosition();
	CalcSizes();
	VidInitItems();
}

void CMenuMain::DisconnectDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::DisconnectCb );
	dialog.SetMessage( L( "Really disconnect?" ) );
	dialog.Show();
}

void CMenuMain::HazardCourseDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::HazardCourseCb );;
	dialog.SetMessage( L( "StringsList_234" ) );
	dialog.Show();
}

/*
=================
CMenuMain::Key
=================
*/
bool CMenuMain::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ) )
	{
		if ( CL_IsActive( ))
		{
			if( !dialog.IsVisible() )
				UI_CloseMenu();
		}
		else
		{
#if XASH_GAMECUBE
			/* Pad B is Escape/"back". On the top-level main menu there is
			 * nothing to dismiss — do not open the quit confirm for a stray
			 * Back press. Dismiss an open dialog; otherwise only the Quit
			 * row should ask to exit. */
			if( dialog.IsVisible() )
				dialog.Hide();
#else
			QuitDialogCb( );
#endif
		}
		return true;
	}
	if( mohaaRoom && dialog.IsVisible() == false && UI::Key::IsNavigationKey( key ))
	{
		// One press, one step. In Dolphin a second D-pad press came 65-85 ms after the first one
		// that lit the map spot (its art loads on focus), and focus went straight past New Game.
		// Nobody taps the D-pad twice that fast on purpose.
		static int lastNavTime = -1000;
		const int now = uiStatic.realTime;
		if( now - lastNavTime >= 0 && now - lastNavTime < MOHAA_ROOM_NAV_REPEAT_MS )
		{
			Con_DPrintf( "MOHAA main menu: second step %d ms after the last one dropped\n", now - lastNavTime );
			return true;
		}
		lastNavTime = now;
	}
	return CMenuFramework::KeyDown( key );
}

/*
=================
UI_Main_HazardCourse
=================
*/
void CMenuMain::HazardCourseCb()
{
	if( EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) > 1 )
		EngFuncs::HostEndGame( "end of the game" );

	EngFuncs::CvarSetValue( "skill", 1.0f );
	EngFuncs::CvarSetValue( "deathmatch", 0.0f );
	EngFuncs::CvarSetValue( "teamplay", 0.0f );
	EngFuncs::CvarSetValue( "pausable", 1.0f ); // singleplayer is always allowing pause
	EngFuncs::CvarSetValue( "coop", 0.0f );
	EngFuncs::CvarSetValue( "maxplayers", 1.0f ); // singleplayer

	EngFuncs::PlayBackgroundTrack( NULL, NULL );

	EngFuncs::ClientCmd( false, "hazardcourse\n" );
}

void CMenuMain::_Init( void )
{
	if( LoadMohaaRoom( ))
	{
		dialog.Link( this );
		for( int i = 0; i < mohaaSpotCount; i++ )
			AddItem( mohaaSpots[i] );
		if( mohaaResume.picPath[0] )
			AddItem( mohaaResume );
		if( mohaaQuit.picPath[0] )
			AddItem( mohaaQuit );
		return;
	}

	const bool dodRetail = UI_IsDayOfDefeat();
#if XASH_GAMECUBE
	/* -gcmenuplaystart already tips MEM1 before UI_Main_Menu; btn_*.tga
	 * strips (~36 KiB decode each) hung ImageLib with no further OSREPORT
	 * (probes 220253/221435). Text buttons still confirm New Game. */
	const bool leanMenuArt = EngFuncs::GetCvarFloat( "gc_menuplaystart" ) >= 1.0f;
	if( leanMenuArt )
		uiStatic.renderPicbuttonText = true;
#else
	const bool leanMenuArt = false;
#endif

	if( gMenu.m_gameinfo.trainmap[0] && stricmp( gMenu.m_gameinfo.trainmap, gMenu.m_gameinfo.startmap ) != 0 )
		bTrainMap = true;
	else bTrainMap = false;

	if( EngFuncs::GetCvarFloat( "host_allow_changegame" ))
		bCustomGame = true;
	else bCustomGame = false;

	resumeGame.SetNameAndStatus( "Resume Game", "Return to game." );
#if XASH_GAMECUBE
	if( !leanMenuArt )
		resumeGame.SetPicture( "resource/gc_menu/btn_resume_game.tga" );
	else
		resumeGame.SetPicture( (const char *)NULL );
#else
	resumeGame.SetPicture( PC_RESUME_GAME );
#endif
	resumeGame.iFlags |= QMF_NOTIFY;
	resumeGame.onReleased = UI_CloseMenu;

	disconnect.SetNameAndStatus( L( "GameUI_GameMenu_Disconnect" ), L( "Disconnect from server" ) );
#if XASH_GAMECUBE
	if( !leanMenuArt )
		disconnect.SetPicture( "resource/gc_menu/btn_disconnect.tga" );
	else
		disconnect.SetPicture( (const char *)NULL );
#else
	disconnect.SetPicture( PC_DISCONNECT );
#endif
	disconnect.iFlags |= QMF_NOTIFY;
	disconnect.onReleased = VoidCb( &CMenuMain::DisconnectDialogCb );

	newGame.SetNameAndStatus( "New Game", "Start a new game." );
#if XASH_GAMECUBE
	if( !leanMenuArt )
		newGame.SetPicture( "resource/gc_menu/btn_new_game.tga" );
	else
		newGame.SetPicture( (const char *)NULL );
#else
	newGame.SetPicture( PC_NEW_GAME );
#endif
	newGame.iFlags |= QMF_NOTIFY;
	newGame.onReleased = UI_NewGame_Menu;

	hazardCourse.SetNameAndStatus( "Hazard Course", "Learn how to play Half-Life." );
#if XASH_GAMECUBE
	if( !leanMenuArt )
		hazardCourse.SetPicture( "resource/gc_menu/btn_hazard_course.tga" );
	else
		hazardCourse.SetPicture( (const char *)NULL );
#else
	hazardCourse.SetPicture( PC_HAZARD_COURSE );
#endif
	hazardCourse.iFlags |= QMF_NOTIFY;
	hazardCourse.onReleasedClActive = VoidCb( &CMenuMain::HazardCourseDialogCb );
	hazardCourse.onReleased = VoidCb( &CMenuMain::HazardCourseCb );

	configuration.SetNameAndStatus( "Options", "Change game settings, configure controls" );
#if XASH_GAMECUBE
	if( !leanMenuArt )
		configuration.SetPicture( "resource/gc_menu/btn_config.tga" );
	else
		configuration.SetPicture( (const char *)NULL );
#else
	configuration.SetPicture( PC_CONFIG );
#endif
	configuration.iFlags |= QMF_NOTIFY;
	configuration.onReleased = UI_Options_Menu;

	saveRestore.iFlags |= QMF_NOTIFY;

	quit.SetNameAndStatus( "Quit", "Quit playing Half-Life." );
#if XASH_GAMECUBE
	if( !leanMenuArt )
		quit.SetPicture( "resource/gc_menu/btn_quit.tga" );
	else
		quit.SetPicture( (const char *)NULL );
#else
	quit.SetPicture( PC_QUIT );
#endif
	quit.iFlags |= QMF_NOTIFY;
	quit.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

	quitButton.SetPicture( ART_CLOSEBTN_N, ART_CLOSEBTN_F, ART_CLOSEBTN_D );
	quitButton.iFlags = QMF_MOUSEONLY;
	quitButton.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	quitButton.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

	minimizeBtn.SetPicture( ART_MINIMIZE_N, ART_MINIMIZE_F, ART_MINIMIZE_D );
	minimizeBtn.iFlags = QMF_MOUSEONLY;
	minimizeBtn.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	minimizeBtn.onReleased.SetCommand( false, "minimize\n" );

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY || gMenu.m_gameinfo.startmap[0] == 0 )
		newGame.SetGrayed( true );

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY || dodRetail )
	{
		/* Retail Day of Defeat / MP-only: New game, Find servers, Options, Quit.
		 * New game opens create-server; Find servers replaces Load game. */
		newGame.SetNameAndStatus( "New game", "" );
		newGame.onReleased = UI_CreateGame_Menu;
		newGame.SetGrayed( false );
		saveRestore.SetNameAndStatus( "Find servers", "" );
		configuration.SetNameAndStatus( "Options", "" );
		quit.SetNameAndStatus( "Quit", "" );
		saveRestore.onReleased = UI_InternetGames_Menu;
		saveRestore.SetGrayed( false );
		hazardCourse.SetGrayed( true );
	}

	// server.dll needs for reading savefiles or startup newgame
	if( !EngFuncs::CheckGameDll( ))
	{
		if( gMenu.m_gameinfo.gamemode != GAME_MULTIPLAYER_ONLY && !dodRetail )
			saveRestore.SetGrayed( true );
		hazardCourse.SetGrayed( true );
		if( gMenu.m_gameinfo.gamemode != GAME_MULTIPLAYER_ONLY && !dodRetail )
			newGame.SetGrayed( true );
	}

#if XASH_GAMECUBE
	/* Baked logo from disc build. Steam-bg path disables logo.avi, so draw a
	 * static title mark instead. -gcmenuplaystart skips it to save tip MEM1. */
	if( !leanMenuArt )
	{
		if( dodRetail )
			titleLogo.SetPicture( "resource/gc_menu/logo.tga" );
		else
		{
			/* WON title: baked frame of media/logo.avi (640x100 banner). */
			titleLogo.SetPicture( "resource/gc_menu/logo_banner.tga" );
		}
		/* Position/size are in the 1024×768 menu coordinate system. */
		titleLogo.iFlags = QMF_INACTIVE;
		titleLogo.SetRenderMode( QM_DRAWTRANS );
		AddItem( titleLogo );
	}
#else
	if( FBitSet( gMenu.m_gameinfo.flags, GFL_ANIMATED_TITLE ))
	{
		if( animatedBanner.TryLoad())
			AddItem( animatedBanner );
	}
	else
	{
		AddItem( movieBanner );
	}
#endif

	dialog.Link( this );

	AddItem( banner );
	AddItem( disconnect );
	AddItem( resumeGame );
	AddItem( newGame );
	/* Retail main menu always exposes Hazard Course (when a train map is
	 * configured) and Quit — keep that parity on GameCube. */
	if( bTrainMap )
		AddItem( hazardCourse );
	AddItem( saveRestore );
	AddItem( configuration );
	AddItem( quit );
}

/*
=================
UI_Main_Init
=================
*/
void CMenuMain::VidInit( bool connected )
{
	int hoffset = ( 32 / 640.0 ) * 1024.0;
	int retail_top = ( 333 / 480.0 ) * 768.0;
	int ygap = ( 31 / 480.0 ) * 768.0;
#if XASH_GAMECUBE
	/* Text-only fallback needs taller rows; retail btns_main art uses the
	 * stock ~50 menu-unit gap so the column matches PC Half-Life. */
	if( uiStatic.renderPicbuttonText && ygap < 56 )
		ygap = 56;
#endif

	bool single = gpGlobals->maxClients < 2;
#if XASH_GAMECUBE
	/* Pic buttons may be constructed before menu atlas sizing is ready.
	 * Refresh their virtual dimensions before CMenuBaseItem::VidInit
	 * computes screen bounds, otherwise the menu art draws at 0x0. */
	resumeGame.size = uiStatic.buttons_draw_size;
	disconnect.size = uiStatic.buttons_draw_size;
	newGame.size = uiStatic.buttons_draw_size;
	hazardCourse.size = uiStatic.buttons_draw_size;
	configuration.size = uiStatic.buttons_draw_size;
	saveRestore.size = uiStatic.buttons_draw_size;
	quit.size = uiStatic.buttons_draw_size;
#endif

	if( connected )
	{
		int yoffset = retail_top;

		resumeGame.SetCoord( hoffset, yoffset );
		yoffset += ygap;

		if( single )
		{
			saveRestore.SetNameAndStatus( "Save\\Load Game", "Load a saved game, save the current game." );
#if XASH_GAMECUBE
			if( EngFuncs::GetCvarFloat( "gc_menuplaystart" ) < 1.0f )
				saveRestore.SetPicture( "resource/gc_menu/btn_save_load_game.tga" );
			else
				saveRestore.SetPicture( (const char *)NULL );
#else
			saveRestore.SetPicture( PC_SAVE_LOAD_GAME );
#endif
			saveRestore.onReleased = UI_SaveLoad_Menu;
			disconnect.SetNameAndStatus( "Leave Game", "Return to the main menu." );

			newGame.SetCoord( hoffset, yoffset );
			yoffset += ygap;

			saveRestore.SetCoord( hoffset, yoffset );
			yoffset += ygap;

			configuration.SetCoord( hoffset, yoffset );
			yoffset += ygap;

			disconnect.SetCoord( hoffset, yoffset );
			yoffset += ygap;
		}
		else
		{
		disconnect.SetNameAndStatus( "Disconnect", "Disconnect from the current server." );
			disconnect.SetCoord( hoffset, yoffset );
			yoffset += ygap;

			configuration.SetCoord( hoffset, yoffset );
			yoffset += ygap;
		}
	}
	else
	{
		int yoffset = retail_top;
		const bool mpOnly = ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY || UI_IsDayOfDefeat() );

		if( mpOnly )
		{
#if XASH_GAMECUBE
			/* DoD strip textures have a black fringe on GX; use readable
			 * menu text for the primary action like Find servers/Options. */
			if( UI_IsDayOfDefeat() ) newGame.SetPicture( (const char *)NULL );
#endif
			saveRestore.SetNameAndStatus( "Find servers", "" );
#if XASH_GAMECUBE
			if( UI_IsDayOfDefeat() || EngFuncs::GetCvarFloat( "gc_menuplaystart" ) >= 1.0f )
				saveRestore.SetPicture( (const char *)NULL );
			else
				saveRestore.SetPicture( "resource/gc_menu/btn_load_game.tga" );
#else
			saveRestore.SetPicture( PC_MULTIPLAYER );
#endif
			saveRestore.onReleased = UI_InternetGames_Menu;
			newGame.SetNameAndStatus( "New game", "" );
			newGame.onReleased = UI_CreateGame_Menu;
			quit.SetNameAndStatus( "Quit", "" );
			if( UI_IsDayOfDefeat() )
			{
#if XASH_GAMECUBE
				/* The retail strip says "Configuration"; DoD calls this "Options". */
				configuration.SetPicture( (const char *)NULL );
				quit.SetPicture( (const char *)NULL );
#endif
				newGame.iFlags &= ~QMF_NOTIFY;
				saveRestore.iFlags &= ~QMF_NOTIFY;
				configuration.iFlags &= ~QMF_NOTIFY;
				quit.iFlags &= ~QMF_NOTIFY;
				newGame.colorBase = CColor( 0xFFFFB900 );
				newGame.colorFocus = CColor( 0xFFFFFF63 );
				saveRestore.colorBase = CColor( 0xFFFFB900 );
				saveRestore.colorFocus = CColor( 0xFFFFFF63 );
				configuration.colorBase = CColor( 0xFFFFB900 );
				configuration.colorFocus = CColor( 0xFFFFFF63 );
				quit.colorBase = CColor( 0xFFFFB900 );
				quit.colorFocus = CColor( 0xFFFFFF63 );
				quit.SetVisibility( false );
			}
		}
		else
		{
			saveRestore.SetNameAndStatus( "Load Game", "Load a previously saved game." );
#if XASH_GAMECUBE
			if( EngFuncs::GetCvarFloat( "gc_menuplaystart" ) < 1.0f )
				saveRestore.SetPicture( "resource/gc_menu/btn_load_game.tga" );
			else
				saveRestore.SetPicture( (const char *)NULL );
#else
			saveRestore.SetPicture( PC_LOAD_GAME );
#endif
			saveRestore.onReleased = UI_LoadGame_Menu;
			newGame.SetNameAndStatus( "New Game", "Start a new single player game." );
			quit.SetNameAndStatus( "Quit", "Quit playing Half-Life." );
		}
		configuration.SetNameAndStatus( "Options", UI_IsDayOfDefeat() ? "" : "Change game settings, configure controls." );

		newGame.SetCoord( hoffset, yoffset );
		yoffset += ygap;

		if( bTrainMap && !mpOnly )
		{
			hazardCourse.SetCoord( hoffset, yoffset );
			yoffset += ygap;
		}

		saveRestore.SetCoord( hoffset, yoffset );
		yoffset += ygap;

		configuration.SetCoord( hoffset, yoffset );
		yoffset += ygap;

		quit.SetCoord( hoffset, yoffset );
	}

#if XASH_GAMECUBE
	if( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY || UI_IsDayOfDefeat() )
	{
		/* Keep the DoD wordmark in the lower-left safe area, as on retail. */
		titleLogo.SetCoord( 88, 700 );
		titleLogo.size = Size( 400, 26 );
		titleLogo.SetVisibility( true );
	}
	else
	{
		/* WON places logo.avi full width at y=70 of the 640x480 menu
		 * (CMenuMovieBanner); the baked banner is 640x104. */
		titleLogo.SetCoord( 0, ( 70 / 480.0 ) * 768.0 );
		titleLogo.size = Size( 1024, ( 104 / 480.0 ) * 768.0 );
	}
#endif

	// now figure out what's visible
	resumeGame.SetVisibility( connected );
	disconnect.SetVisibility( connected );
	saveRestore.SetVisibility( !connected || single );
	configuration.SetVisibility( true );
	quit.SetVisibility( !connected );
	if( UI_IsDayOfDefeat() && !connected )
		quit.SetVisibility( false );
	if( bTrainMap )
		hazardCourse.SetVisibility( !connected && gMenu.m_gameinfo.gamemode != GAME_MULTIPLAYER_ONLY );
}

void CMenuMain::_VidInit()
{
	if( mohaaRoom )
	{
		MohaaVidInit( CL_IsActive( ));
		return;
	}
	VidInit( CL_IsActive() );
}

void CMenuMain::Think()
{
	CMenuFramework::Think();
}

ADD_MENU( menu_main, CMenuMain, UI_Main_Menu );
