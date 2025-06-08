#pragma once

#include "pluginmain.h"

#define PLUGIN_NAME "manytypes-plugin"
#define OPEN_EXPLORER_MANYTYPES 0
#define OPEN_VSCODE_MANYTYPES 1

void plugin_run_loop();

void set_workspace_target( const char* image_name );
void plugin_menu_select( int entry );
bool plugin_handle_pt( int argc, char** t );

bool plugin_init( PLUG_INITSTRUCT* initStruct );
void plugin_stop();
void plugin_setup();
