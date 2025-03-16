#pragma once

#include "pluginmain.h"

#define PLUGIN_NAME "manytypes-plugin"

void plugin_run_loop();
void set_workspace_target(const char* image_name);

bool plugin_init(PLUG_INITSTRUCT* initStruct);
void plugin_stop();
void plugin_setup();
