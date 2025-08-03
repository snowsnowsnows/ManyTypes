#include "pluginmain.h"
#include "plugin.h"

int pluginHandle;
HWND hwndDlg;
int hMenu;
int hMenuDisasm;
int hMenuDump;
int hMenuStack;
int hMenuGraph;
int hMenuMemmap;
int hMenuSymmod;

PLUG_EXPORT bool pluginit( PLUG_INITSTRUCT* initStruct )
{
    initStruct->pluginVersion = PLUGIN_VERSION;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s( initStruct->pluginName, PLUGIN_NAME, _TRUNCATE );
    pluginHandle = initStruct->pluginHandle;

    return plugin_init( initStruct );
}

PLUG_EXPORT bool plugstop()
{
    plugin_stop();
    return true;
}

PLUG_EXPORT void plugsetup( PLUG_SETUPSTRUCT* setupStruct )
{
    hwndDlg = setupStruct->hwndDlg;
    hMenu = setupStruct->hMenu;
    hMenuDisasm = setupStruct->hMenuDisasm;
    hMenuDump = setupStruct->hMenuDump;
    hMenuStack = setupStruct->hMenuStack;
    hMenuGraph = setupStruct->hMenuGraph;
    hMenuMemmap = setupStruct->hMenuMemmap;
    hMenuSymmod = setupStruct->hMenuSymmod;

    plugin_setup();
}

PLUG_EXPORT void CBMENUENTRY( CBTYPE, PLUG_CB_MENUENTRY* info )
{
    plugin_menu_select( info->hEntry );
}

PLUG_EXPORT void CBINITDEBUG( CBTYPE bType, PLUG_CB_INITDEBUG* callbackInfo )
{
    set_workspace_target( callbackInfo->szFileName );
    dprintf( "initialized workspace %s\n", callbackInfo->szFileName );
}

PLUG_EXPORT void CBSTOPDEBUG( CBTYPE bType, PLUG_CB_STOPDEBUG* callbackInfo )
{
    set_workspace_target( {} );
}