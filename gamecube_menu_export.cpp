#include "extdll_menu.h"
#include "dll_gamecube.h"

extern "C" EXPORT int GetMenuAPI( UI_FUNCTIONS *pFunctionTable, ui_enginefuncs_t *pEngfuncsFromEngine, ui_globalvars_t *pGlobals );
extern "C" EXPORT int GetExtAPI( int version, UI_EXTENDED_FUNCTIONS *pFunctionTable, ui_extendedfuncs_t *pEngfuncsFromEngine );

static dllexport_t gamecube_menu_exports[] =
{
	{ "GetMenuAPI", (void *)GetMenuAPI },
	{ "GetExtAPI", (void *)GetExtAPI },
	{ NULL, NULL },
};

extern "C" int setup_gamecube_menu_exports( void )
{
	int ret = 0;

	ret |= dll_register( "menu", gamecube_menu_exports );
	ret |= dll_register( "menu.so", gamecube_menu_exports );
	ret |= dll_register( "libmenu.so", gamecube_menu_exports );
	ret |= dll_register( "cl_dlls/gameui.so", gamecube_menu_exports );

	return ret;
}
