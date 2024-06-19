#include <furi_hal.h>
#include <furi.h>

#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>

#include <loader/loader.h>
#include <applications.h>

#define TAG "Desktop"

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    Loader* loader;
} Desktop;

typedef enum {
    DesktopViewSubmenu,
} DesktopView;

static void desktop_submenu_callback(void* context, uint32_t index) {
    Desktop* instance = (Desktop*)context;
    // UNUSED(instance);
    // UNUSED(index);

    FuriString* error_message = furi_string_alloc();
    const char* app_name = FLIPPER_APPS[index].name;
    LoaderStatus status = loader_start(instance->loader, app_name, NULL, error_message);

    if(status != LoaderStatusOk) {
        FURI_LOG_E(
            TAG,
            "Failed to start application %s: %s",
            app_name,
            furi_string_get_cstr(error_message));
    }

    furi_string_free(error_message);
}

static uint32_t desktop_exit_callback(void* context) {
    UNUSED(context);
    return VIEW_IGNORE;
}

Desktop* desktop_alloc(void) {
    Desktop* instance = malloc(sizeof(Desktop));

    View* view = NULL;

    instance->gui = furi_record_open(RECORD_GUI);
    instance->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(instance->view_dispatcher);
    view_dispatcher_attach_to_gui(
        instance->view_dispatcher, instance->gui, ViewDispatcherTypeFullscreen);

    // Menu
    instance->submenu = submenu_alloc();
    view = submenu_get_view(instance->submenu);
    view_set_previous_callback(view, desktop_exit_callback);
    view_dispatcher_add_view(instance->view_dispatcher, DesktopViewSubmenu, view);

    for(uint32_t i = 0; i < FLIPPER_APPS_COUNT; i++) {
        const FlipperInternalApplication* app = &FLIPPER_APPS[i];
        submenu_add_item(instance->submenu, app->name, i, desktop_submenu_callback, instance);
    }

    instance->loader = furi_record_open(RECORD_LOADER);

    return instance;
}

int32_t desktop_run(Desktop* instance) {
    view_dispatcher_switch_to_view(instance->view_dispatcher, DesktopViewSubmenu);
    view_dispatcher_run(instance->view_dispatcher);
    return 0;
}

int32_t desktop_srv(void* p) {
    UNUSED(p);
    Desktop* instance = desktop_alloc();
    int32_t ret = desktop_run(instance);
    FURI_LOG_E(TAG, "Desktop exited with %ld", ret);
    furi_crash("Desktop exited");
    return ret;
}
