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

PLUG_EXPORT bool plugstop( )
{
    plugin_stop( );
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

    plugin_setup( );
}

extern "C" __declspec(dllexport) void CBWINEVENT( CBTYPE bType, PLUG_CB_WINEVENT* info )
{
    plugin_run_loop( );
}

extern "C" __declspec(dllexport) void CBINITDEBUG( CBTYPE bType, PLUG_CB_INITDEBUG* callbackInfo )
{
    set_workspace_target( callbackInfo->szFileName );
    dprintf( "debug inited %s\n", callbackInfo->szFileName );
}

extern "C" __declspec(dllexport) void CBSTOPDEBUG( CBTYPE bType, PLUG_CB_STOPDEBUG* callbackInfo )
{
    dprintf( "debug stopped\n" );
    set_workspace_target( nullptr );
}