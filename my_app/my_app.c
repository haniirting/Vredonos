#include <furi.h>
#include <string.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>

#include "subghz_brute.h"
#include "ir_jammer.h"

typedef enum {
    MyAppViewSubmenu,
} MyAppView;

typedef enum {
    MyAppItemSubGhz,
    MyAppItemIr,
    MyAppItemStop,
} MyAppItem;

typedef struct {
    Gui*            gui;
    ViewDispatcher* view_dispatcher;
    Submenu*        submenu;
    volatile bool   stop_flag;
    FuriThread*     worker;
} MyApp;

/* --- worker-потоки --- */

static int32_t subghz_worker_cb(void* ctx) {
    MyApp* app = ctx;
    subghz_brute_run(433920000UL, 0x000000, 0x0003FF, &app->stop_flag);
    return 0;
}

static int32_t ir_worker_cb(void* ctx) {
    MyApp* app = ctx;
    ir_jammer_run(&app->stop_flag);
    return 0;
}

/* --- управление worker-потоком --- */

static void start_worker(MyApp* app, FuriThreadCallback cb) {
    app->stop_flag = false;
    app->worker = furi_thread_alloc();
    furi_thread_set_callback(app->worker, cb);
    furi_thread_set_context(app->worker, app);
    furi_thread_set_stack_size(app->worker, 2048);
    furi_thread_start(app->worker);
}

static void stop_worker(MyApp* app) {
    app->stop_flag = true;
    if(app->worker) {
        furi_thread_join(app->worker);
        furi_thread_free(app->worker);
        app->worker = NULL;
    }
}

/* --- меню --- */

static void my_app_menu_cb(void* ctx, uint32_t index) {
    MyApp* app = ctx;
    switch(index) {
    case MyAppItemSubGhz:
        stop_worker(app);
        start_worker(app, subghz_worker_cb);
        break;
    case MyAppItemIr:
        stop_worker(app);
        start_worker(app, ir_worker_cb);
        break;
    case MyAppItemStop:
        stop_worker(app);
        break;
    default:
        break;
    }
}

/* --- точка входа --- */

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
    view_dispatcher_add_view(
        app->view_dispatcher, MyAppViewSubmenu, submenu_get_view(app->submenu));
    view_dispatcher_switch_to_view(app->view_dispatcher, MyAppViewSubmenu);
    view_dispatcher_attach_to_gui(
        app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_run(app->view_dispatcher);

    /* корректное завершение при выходе из приложения */
    stop_worker(app);

    view_dispatcher_remove_view(app->view_dispatcher, MyAppViewSubmenu);
    submenu_free(app->submenu);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);
    return 0;
}
