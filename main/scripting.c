#include "scripting.h"
#include "esp_log.h"
#include "string.h"
#include "map.h"

static const char *TAG = "WrenScript";
WrenVM *wrenVM = NULL;
static map_str_t moduleLookup;
static map_str_t methodLookup;

static void wren_writefn(WrenVM *vm, const char *text){
    if (strlen(text) == 1 && text[0] == '\n') return; // skip newline print (Wren does it separately for some reason)
    ESP_LOGD(TAG, "%s", text);
}

// based on: https://github.com/wren-lang/wren/blob/master/src/cli/vm.c#L246
static void wren_errorfn(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message){
    switch (type){
        case WREN_ERROR_COMPILE:
            ESP_LOGE(TAG, "[%s line %d] %s", module, line, message);
            break;
        case WREN_ERROR_RUNTIME:
            ESP_LOGE(TAG, "%s", message);
            break;
        case WREN_ERROR_STACK_TRACE:
            ESP_LOGE(TAG, "[%s line %d] in %s", module, line, message);
            break;
    }
}

void scripting_init(){
    // TODO is it a good idea to run the Wren VM in its own task?
    WrenConfiguration wrenConfig;
    wrenInitConfiguration(&wrenConfig);
    wrenConfig.writeFn = wren_writefn;
    wrenConfig.errorFn = wren_errorfn;
    wrenConfig.initialHeapSize = WREN_INIT_HEAP_SIZE * 1024;
    wrenConfig.minHeapSize = WREN_INIT_HEAP_SIZE * 1024;
    wrenVM = wrenNewVM(&wrenConfig);

    // create bindings to native code
    // ... mainly FSM ...

    ESP_LOGI(TAG, "Wren VM init OK!");
}

void scripting_eval(char *str){
    wrenInterpret(wrenVM, "omicron", str);
}

void scripting_destroy(){
    wrenFreeVM(wrenVM);
    wrenVM = NULL;
}