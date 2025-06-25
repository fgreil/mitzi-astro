// city_explorer_scene_country_list.c
#include "../city_explorer_scene.h"

void city_explorer_scene_country_list_on_enter(void* context) {
    CityExplorerApp* app = context;
    
    submenu_reset(app->submenu);
    
    for(int i = 0; i < app->database->total_countries; i++) {
        submenu_add_item(app->submenu, app->database->countries[i].name, i, 
                        city_explorer_scene_country_list_callback, app);
    }
    
    view_dispatcher_switch_to_view(app->view_dispatcher, 0);
}

bool city_explorer_scene_country_list_on_event(void* context, SceneManagerEvent event) {
    CityExplorerApp* app = context;
    
    if(event.type == SceneManagerEventTypeCustom) {
        app->scene_state.selected_country = event.event;
        scene_manager_next_scene(app->scene_manager, CityExplorerSceneCityList);
        return true;
    }
    
    return false;
}

void city_explorer_scene_country_list_on_exit(void* context) {
    CityExplorerApp* app = context;
    submenu_reset(app->submenu);
}

void city_explorer_scene_country_list_callback(void* context, uint32_t index) {
    CityExplorerApp* app = context;
    scene_manager_handle_custom_event(app->scene_manager, index);
}