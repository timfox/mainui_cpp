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

class CMenuMain: public CMenuFramework
{
public:
	CMenuMain() : CMenuFramework( "CMenuMain" ) { }

	bool KeyDown( int key ) override;

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
};

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
	VidInit( CL_IsActive() );
}

void CMenuMain::Think()
{
	CMenuFramework::Think();
}

ADD_MENU( menu_main, CMenuMain, UI_Main_Menu );
