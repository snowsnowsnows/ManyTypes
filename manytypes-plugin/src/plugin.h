#pragma once

#include "pluginmain.h"

enum
{
    OPEN_EXPLORER_MANYTYPES,
    EDIT_MANYTYPES,
};

void set_workspace_target( std::string image_name );
void plugin_menu_select( int entry );
bool plugin_handle_pt( int argc, char** t );

bool plugin_init( PLUG_INITSTRUCT* initStruct );
void plugin_stop();
void plugin_setup();
