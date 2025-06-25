#pragma once
#include <gui/scene_manager.h>

typedef enum {
    CityExplorerSceneCountryList,
    CityExplorerSceneCityList,
    CityExplorerSceneCityDetails,
    CityExplorerSceneNum,
} CityExplorerScene;

extern const SceneManagerHandlers city_explorer_scene_handlers;

typedef struct {
    int selected_country;
    int selected_city;
} CityExplorerSceneState;
