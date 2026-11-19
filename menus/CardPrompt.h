/*
CardPrompt.h -- the "format the memory card?" dialog. Formatting erases the whole card, so the dialog says so, the
default is No, and gc_card_format is only ever sent from the Format button (the engine refuses it unless the last save
answered "the card must be formatted first").
*/
#pragma once
#include "YesNoMessageBox.h"

class CMenuCardFormatBox : public CMenuYesNoMessageBox
{
public:
	typedef CMenuYesNoMessageBox BaseClass;

	CMenuCardFormatBox() : BaseClass( false )
	{
		SetMessage( L( "Format the memory card in slot A?" ));
		SetPositiveButton( L( "Format" ), PC_OK );
		SetNegativeButton( L( "No" ), PC_CANCEL );
		warn.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
		warn.eTextAlignment = QM_TOP;
		warn.colorBase = uiColorHelp;
		warn.SetRect( 0, 72, 640, 80 );
		warn.SetCharSize( QM_DEFAULTFONT );
		warn.szName = L( "This ERASES EVERYTHING on the card, including other games' saves." );
		SET_EVENT( onPositive, EngFuncs::ClientCmd( true, "gc_card_format confirmed\n" ) );
	}

	void _Init() override
	{
		BaseClass::_Init();
		AddItem( warn );
	}

	void Show() override
	{
		BaseClass::Show();
		SetCursorToItem( no ); // the safe answer is the default
	}

private:
	CMenuAction warn;
};
