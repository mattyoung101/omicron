#pragma once
#include "defines.h"
#include "wren.h"

// Scripting backend, handles the initialisation and management of any scripting VMs
// In theory it can be extended to support any language, but we currently use Wren

/**
 * the VM instance.
 * CAUTION! May be NULL until scripting_init() is called. Use with caution!
 */
extern WrenVM *wrenVM;

/** Initialises the scripting VM and registers functions */
void scripting_init();
/** Destroys scripting if no longer required */
void scripting_destroy();
/** Evaluates some code */
void scripting_eval(char *str);