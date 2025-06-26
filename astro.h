#pragma once

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <storage/storage.h>
#include <dialogs/dialogs.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>

#define TAG "CityInfoApp"
#define PREFERENCES_FILE EXT_PATH("apps_data/city_info/preferences.txt")
#define CITIES_FILE EXT_PATH("apps_data/city_info/cities.csv")
#define LOGO_FILE EXT_PATH("apps_data/city_info/logo.png")
#define SPLASH_FILE EXT_PATH("apps_data/city_info/splash.png")

typedef enum {
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} AppEvent;

typedef enum {
    ScreenSplash,
    ScreenMainMenu,
    ScreenChooseCity,
    ScreenCityInfo,
} ScreenType;

typedef struct {
    char country_code[4];
    char country_name[32];
    char province[32];
    char city_name[32];
    char description[64];
    float utc_offset;
    int longitude_deg;
    int longitude_min;
    int latitude_deg;
    int latitude_min;
} CityInfo;

typedef enum {
    MainMenuChooseCity,
    MainMenuCityInfo,
    MainMenuExit,
    MainMenuCount
} MainMenuIndex;

typedef struct {
    ScreenType current_screen;
    int menu_index;
    bool city_selected;
    int country_index;
    int city_index;
    bool selector_focus; // true = city, false = country
    CityInfo* cities;
    int city_count;
    CityInfo current_city;
    FuriString* file_path;
    FuriThread* thread;
    bool loading;
    bool initialized;
} CityInfoApp;

// Function declarations
CityInfoApp* city_info_app_alloc();
void city_info_app_free(CityInfoApp* app);
void load_preferences(CityInfoApp* app);
void save_preferences(CityInfoApp* app);
int32_t city_info_app(void* p);
