#include <furi.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <input/input.h>

#include "subghz_brute.h"
#include "ir_jammer.h"

typedef enum {
    MyAppViewSubmenu,
    MyAppViewRunning,
} MyAppView;

typedef enum {
    MyAppItemSubGhz,
    MyAppItemIr,
    MyAppItemStop,
} MyAppItem;

typedef struct {
    Gui*             gui;
    ViewDispatcher*  view_dispatcher;
    Submenu*         submenu;
    Popup*           popup;
    volatile bool    stop_flag;
    FuriThread*      worker;
} MyApp;

static uint32_t my_app_exit_cb(void* ctx) {
    UNUSED(ctx);
    return VIEW_NONE;
}

static void my_app_menu_cb(void* ctx, uint32_t index) {
    MyApp* app = ctx;
    switch (index) {
    case MyAppItemSubGhz:
        app->stop_flag = false;
        // freq = 433.92 МГц, брутфорс последних 10 бит вокруг 0
        // в реальности start/end берутся из захваченного кода
        subghz_brute_run(433920000UL, 0x000000, 0x0003FF, &app->stop_flag);
        break;
    case MyAppItemIr:
        app->stop_flag = false;
        ir_jammer_run(&app->stop_flag);
        break;
    case MyAppItemStop:
        app->stop_flag = true;
        break;
    }
}

int32_t my_app_main(void* p) {
    UNUSED(p);
    MyApp* app = malloc(sizeof(MyApp));
    memset(app, 0, sizeof(MyApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();

    app->submenu = submenu_alloc();
    submenu_add_item(app->submenu, "SubGHz brute", MyAppItemSubGhz, my_app_menu_cb, app);
    submenu_add_item(app->submenu, "IR jam",      MyAppItemIr,    my_app_menu_cb, app);
    submenu_add_item(app->submenu, "STOP",        MyAppItemStop,  my_app_menu_cb, app);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, NULL); // back = exit, по желанию
    view_dispatcher_add_view(
        app->view_dispatcher, MyAppViewSubmenu, submenu_get_view(app->submenu));

    view_dispatcher_switch_to_view(app->view_dispatcher, MyAppViewSubmenu);
    view_dispatcher_attach_to_gui(
        app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_run(app->view_dispatcher);

    // cleanup
    view_dispatcher_remove_view(app->view_dispatcher, MyAppViewSubmenu);
    submenu_free(app->submenu);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);
    return 0;
}
