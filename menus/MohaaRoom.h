/*
MohaaRoom.h - the pieces of MOHAA's retail menus that mainui draws: a rect on the menu's 640x480
screen mapped the way the background is, and a "spot", a part of the menu picture that lights up
under the pad cursor (or a picture button). Main.cpp (the war room) and MohaaPage.cpp (the other
retail pages) use them. Layout files come from mohaa-gc's converter (tools/mohaa_menu.py).
*/
#pragma once
#ifndef MOHAAROOM_H
#define MOHAAROOM_H

#include "BaseMenu.h"
#include "BaseItem.h"
#include "Image.h"
#include "keydefs.h"
#include "BaseWindow.h"

struct mohaaRect_t
{
	int x, y, w, h;
};

// The room drawn the way CMenuBackgroundBitmap draws the background sheet: scaled to cover the
// screen keeping its aspect (or stretched, with ui_background_stretch), centred.
static inline void UI_MohaaRoomToScreen( const mohaaRect_t &r, int roomW, int roomH, Point &pt, Size &sz )
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

// Only the menu on top loads its art. A closing menu still draws a few frames, and a covered one
// keeps drawing under the page above it; neither should hold textures the GameCube needs.
static inline bool UI_MohaaOnTop( const CMenuItemsHolder *parent )
{
	return parent && static_cast<const CMenuItemsHolder *>( uiStatic.menu.Current( )) == parent;
}

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
			if( !loaded && !UI_MohaaOnTop( m_pParent ))
				return;
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
		// A closing or covered menu still draws a few frames: don't load art for it, let it go.
		if( !focused || !UI_MohaaOnTop( m_pParent ))
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

	void Unload( void )
	{
		if( !loaded )
			return;
		pic.ForceUnload();
		focusPic.ForceUnload();
		loaded = false;
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

#endif // MOHAAROOM_H
