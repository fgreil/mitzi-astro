#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include "city_data.h"
#include "scenes/city_explorer_scene.h"

typedef struct {
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    Submenu* submenu;
    Widget* widget;
    CityDatabase* database;
    CityExplorerSceneState scene_state;
} CityExplorerApp;

static bool city_explorer_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    CityExplorerApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool city_explorer_back_event_callback(void* context) {
    furi_assert(context);
    CityExplorerApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

CityExplorerApp* city_explorer_app_alloc() {
    CityExplorerApp* app = malloc(sizeof(CityExplorerApp));
    
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&city_explorer_scene_handlers, app);
    
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, city_explorer_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, city_explorer_back_event_callback);
    
    // Initialize views
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, 0, submenu_get_view(app->submenu));
    
    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, 1, widget_get_view(app->widget));
    
    // Load data
    app->database = malloc(sizeof(CityDatabase));
    if(!city_data_load_from_csv(app->database, "/ext/apps_data/city_explorer/cities.csv")) {
        // Handle loading error
        FURI_LOG_E("CityExplorer", "Failed to load city data");
    }
    
    return app;
}

void city_explorer_app_free(CityExplorerApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, 1);
    widget_free(app->widget);
    
    view_dispatcher_remove_view(app->view_dispatcher, 0);
    submenu_free(app->submenu);
    
    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);
    
    free(app->database);
    free(app);
}

int32_t city_explorer_app(void* p) {
    UNUSED(p);
    
    CityExplorerApp* app = city_explorer_app_alloc();
    
    Gui* gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    
    scene_manager_next_scene(app->scene_manager, CityExplorerSceneCountryList);
    view_dispatcher_run(app->view_dispatcher);
    
    furi_record_close(RECORD_GUI);
    city_explorer_app_free(app);
    
    return 0;
}
