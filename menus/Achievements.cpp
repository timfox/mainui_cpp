/*
Achievements.cpp -- DoD achievements (Options > Achievements, GameCube build)
A read-only list: each achievement, its progress and whether it is unlocked.
Progress is the engine cvar gc_ach (192 hex characters, written by the game and saved with the settings on the memory card).
The layout of that blob is DodAch_Pack in dod-gc/src/dod_achievements.c.

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
#include <string.h>
#include <stdio.h>

#define ART_BANNER "gfx/shell/head_customize"

namespace {

const int ACH_COUNT = 18;
const int BLOB_BYTES = 96;
const int KW_COUNT = 16;

// kill-weapon slots in the blob (DodKillWeapon in dod_achievements.h)
enum { KW_COLT = 1, KW_LUGER, KW_FRAG, KW_STICK, KW_SHOVEL, KW_KNIFE, KW_FISTS, KW_BAZOOKA, KW_PSCHRECK, KW_K98_RG, KW_GARAND_RG };

struct AchRow { const char *name; int source; unsigned goal; };	// source: 0..15 kills[], 100 rocket class, 101 blocks, 102 plants, 103 defuses, -1 one-shot
const AchRow rows[ACH_COUNT] =
{
	{ "Nuts!", 101, 100 }, { "Old School", KW_COLT, 150 }, { "Roscoe", KW_LUGER, 150 }, { "Egg Layer", KW_FRAG, 250 },
	{ "Potato Masher", KW_STICK, 250 }, { "HEATer", 100, 1000 }, { "Dig For Victory!", KW_SHOVEL, 150 }, { "Demolition Man", 102, 100 },
	{ "Rocket Powered", KW_BAZOOKA, 500 }, { "Black Maria", KW_K98_RG, 250 }, { "Hail Mary", -1, 0 }, { "Tank Terror", KW_PSCHRECK, 500 },
	{ "Carpet Bomber", KW_GARAND_RG, 250 }, { "Cutthroat", KW_KNIFE, 150 }, { "Jack of All Trades", -1, 0 }, { "Knuckleduster", KW_FISTS, 150 },
	{ "Combat Engineer", 103, 100 }, { "War Hero", -1, 0 },
};

int HexVal( char c ) { return c >= '0' && c <= '9' ? c - '0' : ( c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1 ); }
unsigned Word( const unsigned char *b, int i ) { b += i * 4; return b[0] | ( b[1] << 8 ) | ( b[2] << 16 ) | ( (unsigned)b[3] << 24 ); }

// false when nothing is saved yet (or the text is damaged): the page then shows zero progress
bool Decode( const char *hex, unsigned char *out )
{
	if( !hex || (int)strlen( hex ) != BLOB_BYTES * 2 )
		return false;
	for( int i = 0; i < BLOB_BYTES; i++ )
	{
		int hi = HexVal( hex[i * 2] ), lo = HexVal( hex[i * 2 + 1] );
		if( hi < 0 || lo < 0 )
			return false;
		out[i] = (unsigned char)( hi << 4 | lo );
	}
	return Word( out, 0 ) == 0x48434144u && Word( out, 1 ) == 1;
}

}

class CMenuAchievements : public CMenuFramework
{
public:
	CMenuAchievements() : CMenuFramework( "CMenuAchievements" ) { }
	void Show() override;

private:
	void _Init() override;
	void Refresh();

	CMenuAction name[ACH_COUNT], progress[ACH_COUNT];
	char text[ACH_COUNT][40];
};

void CMenuAchievements::Refresh()
{
	unsigned char blob[BLOB_BYTES];
	bool have = Decode( EngFuncs::GetCvarString( "gc_ach" ), blob );
	unsigned unlocked = have ? Word( blob, 2 + KW_COUNT + 4 ) & ( ( 1u << ACH_COUNT ) - 1 ) : 0;

	for( int i = 0; i < ACH_COUNT; i++ )
	{
		unsigned count = 0;
		if( have && rows[i].source >= 0 )
		{
			int w = rows[i].source < KW_COUNT ? 2 + rows[i].source : 2 + KW_COUNT + ( rows[i].source - 100 );
			count = Word( blob, w );
		}
		const bool done = ( unlocked >> i ) & 1;
		if( rows[i].goal )
			snprintf( text[i], sizeof( text[i] ), "%s %u / %u", done ? "[x]" : "[ ]", count > rows[i].goal ? rows[i].goal : count, rows[i].goal );
		else
			snprintf( text[i], sizeof( text[i] ), "%s", done ? "[x]" : "[ ]" );
		progress[i].szName = text[i];
	}
}

void CMenuAchievements::Show()
{
	Refresh();
	CMenuFramework::Show();
}

void CMenuAchievements::_Init()
{
	banner.SetPicture( ART_BANNER );
	AddItem( banner );
	AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuAchievements::Hide ));

	for( int i = 0; i < ACH_COUNT; i++ )
	{
		name[i].iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
		name[i].colorBase = uiColorHelp;
		name[i].SetCharSize( QM_DEFAULTFONT );
		name[i].szName = rows[i].name;
		name[i].SetRect( 300, 180 + i * 30, 300, 26 );

		progress[i].iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
		progress[i].SetCharSize( QM_DEFAULTFONT );
		progress[i].SetRect( 620, 180 + i * 30, 300, 26 );

		AddItem( name[i] );
		AddItem( progress[i] );
	}
	Refresh();
}

ADD_MENU( menu_achievements, CMenuAchievements, UI_Achievements_Menu );
