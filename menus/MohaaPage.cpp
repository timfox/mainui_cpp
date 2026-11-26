/*
MohaaPage.cpp - MOHAA's retail menu pages after the war room

When the game folder has resource/gc_menu/mohaa_<page>.txt (written onto the SD card by mohaa-gc's
converter, tools/mohaa_menu.py, from the player's own Medal of Honor: Allied Assault), that page
shows as retail draws it: the urc's picture is the page, and the parts that light up under the
cursor are its buttons. Without the file the stock page shows.

	options    ui/options_main.urc: the desk with Advanced, Audio, Controls and Video
	gametype   ui/multiplayerstart.urc: Free-For-All, Team-Match, Round and Objective, which
	           lead on to Create Server

The file is the war room's format (Main.cpp) plus the page picture:

	screen 640 480                      the page's screen (rects below are on it)
	background tl tr bl br              the picture in four quadrants (each has a GX CMPR .cmp)
	sign x y w h                        where the lit button's name goes
	spot <action> x y w h lit sign      a part of the picture: its lit image, its name ('-': none)
	button <action> x y w h pic focus   a picture button (Back)
*/

#include "Framework.h"
#include "MohaaRoom.h"
#include <new>

#define MOHAA_PAGE_MAX_SPOTS	8

void UI_OptionsList_Menu( void );
void UI_CreateGameForm_Menu( void );
void UI_Audio_Menu( void );
void UI_Video_Menu( void );
void UI_Controls_Menu( void );
void UI_PadLayout_Menu( void );

// The page picture: four quadrants of the 640x480 screen, loaded while the page shows.
class CMenuMohaaBackdrop : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuMohaaBackdrop() : BaseClass(), loaded( false ), roomW( 640 ), roomH( 480 )
	{
		for( int i = 0; i < 4; i++ )
			path[i][0] = '\0';
		iFlags |= QMF_INACTIVE;
	}

	void VidInit( void ) override
	{
		iFlags |= QMF_DISABLESCAILING | QMF_INACTIVE;
		for( int i = 0; i < 4; i++ )
		{
			mohaaRect_t r;
			r.w = roomW / 2;
			r.h = roomH / 2;
			r.x = ( i & 1 ) ? r.w : 0;
			r.y = ( i & 2 ) ? r.h : 0;
			UI_MohaaRoomToScreen( r, roomW, roomH, qpos[i], qsize[i] );
		}
		BaseClass::VidInit();
	}

	void Draw( void ) override
	{
		// Hide() takes the page off the stack and then it still draws while it closes: loading
		// the picture again there kept its four CMPR tiles for good, and the next page went blurry.
		if( !UI_MohaaOnTop( m_pParent ))
		{
			Unload();
			return;
		}
		if( !loaded )
		{
			for( int i = 0; i < 4; i++ )
				if( path[i][0] )
					quad[i].Load( path[i] );
			loaded = true;
		}
		for( int i = 0; i < 4; i++ )
			if( quad[i].IsValid( ))
				UI_DrawPic( qpos[i], qsize[i], uiColorWhite, quad[i] );
	}

	void Unload( void )
	{
		if( !loaded )
			return;
		for( int i = 0; i < 4; i++ )
			quad[i].ForceUnload();
		loaded = false;
	}

	bool loaded;
	int roomW, roomH;
	char path[4][96];
	CImage quad[4];
	Point qpos[4];
	Size qsize[4];
};

class CMenuMohaaPage : public CMenuFramework
{
public:
	typedef CMenuFramework BaseClass;

	CMenuMohaaPage() : BaseClass( "CMenuMohaaPage" ), spotCount( 0 ), ready( false )
	{
		page[0] = '\0';
	}

	bool Load( const char *name );
	bool KeyDown( int key ) override;
	void Hide( void ) override;

	char page[32];

private:
	void _Init( void ) override;
	void Action( CMenuMohaaSpot &item, const char *action );
	void BackCb( void ) { Hide(); }

	int spotCount;
	bool ready;
	CMenuMohaaBackdrop backdrop;
	CMenuMohaaSpot spots[MOHAA_PAGE_MAX_SPOTS];
	CMenuMohaaSpot back;
};

static void UI_MohaaGameType( int type )
{
	char cmd[64];
	// MOHAA's g_gametype: 1 free-for-all, 2 team match, 3 round based, 4 objective
	snprintf( cmd, sizeof( cmd ), "set g_gametype %d\n", type );
	EngFuncs::ClientCmd( false, cmd );
	Con_Printf( "MOHAA menu: game type %d, on to Create Server\n", type );
	UI_CreateGameForm_Menu();
}

static void UI_MohaaGameTypeFFA( void ) { UI_MohaaGameType( 1 ); }
static void UI_MohaaGameTypeTeam( void ) { UI_MohaaGameType( 2 ); }
static void UI_MohaaGameTypeRound( void ) { UI_MohaaGameType( 3 ); }
static void UI_MohaaGameTypeObj( void ) { UI_MohaaGameType( 4 ); }

void CMenuMohaaPage::Action( CMenuMohaaSpot &item, const char *action )
{
	if( !stricmp( action, "back" ))
		item.onReleased = VoidCb( &CMenuMohaaPage::BackCb );
	// options: the desk
	else if( !stricmp( action, "advanced" ))
		item.onReleased = UI_OptionsList_Menu;	// every setting the Cube has (player, network, ...)
	else if( !stricmp( action, "audio" ))
		item.onReleased = UI_Audio_Menu;
	else if( !stricmp( action, "video" ))
		item.onReleased = UI_Video_Menu;
	else if( !stricmp( action, "controls" ))
#if XASH_GAMECUBE
		item.onReleased = UI_PadLayout_Menu;
#else
		item.onReleased = UI_Controls_Menu;
#endif
	// gametype: the game type cards before Create Server
	else if( !stricmp( action, "ffa" ))
		item.onReleased = UI_MohaaGameTypeFFA;
	else if( !stricmp( action, "team" ))
		item.onReleased = UI_MohaaGameTypeTeam;
	else if( !stricmp( action, "round" ))
		item.onReleased = UI_MohaaGameTypeRound;
	else if( !stricmp( action, "obj" ))
		item.onReleased = UI_MohaaGameTypeObj;
	else
		item.SetGrayed( true );
}

bool CMenuMohaaPage::Load( const char *name )
{
	char path[64], token[256];
	char *afile, *pfile;
	mohaaRect_t sign = { 0, 0, 0, 0 };
	int roomW = 640, roomH = 480;

	snprintf( path, sizeof( path ), "resource/gc_menu/mohaa_%s.txt", name );
	if( !EngFuncs::FileExists( path ))
		return false;
	afile = (char *)EngFuncs::COM_LoadFile( path );
	if( !afile )
		return false;

	Q_strncpy( page, name, sizeof( page ));
	spotCount = 0;
	back.picPath[0] = '\0';
	backdrop.path[0][0] = '\0';
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
		else if( !strcmp( token, "background" ))
		{
			for( int i = 0; i < 4 && pfile; i++ )
				pfile = EngFuncs::COM_ParseFile( pfile, backdrop.path[i], sizeof( backdrop.path[i] ));
		}
		else if( !strcmp( token, "spot" ) || !strcmp( token, "button" ))
		{
			const bool isButton = token[0] == 'b';
			char action[32], pic[96], focus[96];
			int v[4];

			if( !( pfile = EngFuncs::COM_ParseFile( pfile, action, sizeof( action )))) break;
			for( int i = 0; i < 4 && pfile; i++ )
			{
				pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ));
				v[i] = atoi( token );
			}
			if( !pfile ) break;
			if( !( pfile = EngFuncs::COM_ParseFile( pfile, pic, sizeof( pic )))) break;
			if( !( pfile = EngFuncs::COM_ParseFile( pfile, focus, sizeof( focus )))) break;
			if( !strcmp( focus, "-" ))
				focus[0] = '\0';

			CMenuMohaaSpot *item;
			if( isButton && !stricmp( action, "back" ))
				item = &back;
			else if( !isButton && spotCount < MOHAA_PAGE_MAX_SPOTS )
				item = &spots[spotCount++];
			else
				continue;
			item->isButton = isButton;
			item->rect.x = v[0], item->rect.y = v[1], item->rect.w = v[2], item->rect.h = v[3];
			Q_strncpy( item->picPath, pic, sizeof( item->picPath ));
			Q_strncpy( item->focusPath, focus, sizeof( item->focusPath ));
			item->szName = item->picPath;
			Action( *item, action );
		}
		// anything else ("//" comments are skipped by the parser): ignore the token
	}
	EngFuncs::COM_FreeFile( afile );

	if( !spotCount || !backdrop.path[0][0] )
	{
		Con_Printf( "MOHAA menu: %s has no spots or no picture, stock page instead\n", path );
		return false;
	}
	backdrop.roomW = roomW, backdrop.roomH = roomH;
	for( int i = 0; i < spotCount; i++ )
	{
		spots[i].signRect = sign;
		spots[i].roomW = roomW, spots[i].roomH = roomH;
	}
	back.roomW = roomW, back.roomH = roomH;
	ready = true;
	Con_Printf( "MOHAA menu: %s page, %d spots from %s\n", page, spotCount, path );
	return true;
}

void CMenuMohaaPage::_Init( void )
{
	AddItem( backdrop );
	for( int i = 0; i < spotCount; i++ )
		AddItem( spots[i] );
	if( back.picPath[0] )
		AddItem( back );
}

bool CMenuMohaaPage::KeyDown( int key )
{
	if( UI::Key::IsNavigationKey( key ))
	{
		// One press, one step, as in the war room: Dolphin delivers a second D-pad press 65-85 ms
		// after the first one.
		static int lastNavTime = -1000;
		const int now = uiStatic.realTime;
		if( now - lastNavTime >= 0 && now - lastNavTime < MOHAA_ROOM_NAV_REPEAT_MS )
			return true;
		lastNavTime = now;
	}
	return BaseClass::KeyDown( key );
}

void CMenuMohaaPage::Hide( void )
{
	// The picture is 150 KiB of texture memory, and the lit card up to 64 KiB: give them back as
	// the page goes.
	backdrop.Unload();
	for( int i = 0; i < spotCount; i++ )
		spots[i].Unload();
	back.Unload();
	BaseClass::Hide();
}

/* Two pages at most, kept out of the small GameCube menu heap like Create Server. */
#define MOHAA_PAGES 2
alignas( CMenuMohaaPage ) static unsigned char mohaaPageStorage[MOHAA_PAGES][sizeof( CMenuMohaaPage )];
static CMenuMohaaPage *mohaaPages[MOHAA_PAGES];
static bool mohaaPageMissing[MOHAA_PAGES];
static const char *mohaaPageNames[MOHAA_PAGES] = { "options", "gametype" };

/*
=================
UI_MohaaPage_Menu

Show the retail page called name when the SD card has it. False: show the stock page.
=================
*/
bool UI_MohaaPage_Menu( const char *name )
{
	int i;

	for( i = 0; i < MOHAA_PAGES; i++ )
		if( !stricmp( mohaaPageNames[i], name ))
			break;
	if( i == MOHAA_PAGES || mohaaPageMissing[i] )
		return false;
	if( !mohaaPages[i] )
	{
		CMenuMohaaPage *p = new ( mohaaPageStorage[i] ) CMenuMohaaPage();
		if( !p->Load( name ))
		{
			p->~CMenuMohaaPage();
			mohaaPageMissing[i] = true;
			return false;
		}
		mohaaPages[i] = p;
	}
	mohaaPages[i]->Show();
	return true;
}

static void UI_MohaaPages_Shutdown( void )
{
	for( int i = 0; i < MOHAA_PAGES; i++ )
	{
		if( mohaaPages[i] )
		{
			mohaaPages[i]->~CMenuMohaaPage();
			mohaaPages[i] = NULL;
		}
		mohaaPageMissing[i] = false;
	}
}

static void UI_MohaaPages_Precache( void ) { }
static void UI_MohaaPages_Show( void ) { UI_MohaaPage_Menu( "options" ); }
ADD_MENU4( menu_mohaapages, UI_MohaaPages_Precache, UI_MohaaPages_Show, UI_MohaaPages_Shutdown );
