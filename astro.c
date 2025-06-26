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
#define PREFERENCES_FILE EXT_PATH("apps_data/mitzi-astro/preferences.txt")
#define CITIES_FILE EXT_PATH("apps_data/mitzi-astro/cities.csv")
#define LOGO_FILE EXT_PATH("apps_data/mitzi-astro/logo.png")
#define SPLASH_FILE EXT_PATH("apps_data/mitzi-astro/splash.png") // 128 x 64 pixels, full display size

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

typedef enum {
    MainMenuChooseCity,
    MainMenuCityInfo,
    MainMenuExit,
    MainMenuCount
} MainMenuIndex;

static void city_info_app_draw_callback(Canvas* canvas, void* ctx) {
    CityInfoApp* app = ctx;
    canvas_clear(canvas);

    if(app->loading) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "Loading...");
        return;
    }

    switch(app->current_screen) {
    case ScreenSplash:
        canvas_draw_icon(canvas, 0, 0, &I_splash);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 58, AlignCenter, AlignBottom, "Press any key");
        break;

    case ScreenMainMenu: {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 5, 10, AlignLeft, AlignTop, "City Info");
        canvas_draw_icon(canvas, 100, 0, &I_logo);

        const char* menu_items[MainMenuCount];
        int item_count = 0;
        menu_items[item_count++] = "Choose City";
        if(app->city_selected) menu_items[item_count++] = "City Info";
        menu_items[item_count++] = "Exit";

        for(int i = 0; i < item_count; i++) {
            const char* text = menu_items[i];
            if(i == app->menu_index) {
                canvas_draw_icon(canvas, 0, 20 + i * 12, &I_ButtonRight_4x7);
                canvas_set_font(canvas, FontPrimary);
            } else {
                canvas_set_font(canvas, FontSecondary);
            }
            canvas_draw_str_aligned(canvas, 10, 25 + i * 12, AlignLeft, AlignTop, text);
        }
        break;
    }

    case ScreenChooseCity: {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignTop, "Choose City");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 20, AlignCenter, AlignTop, "Please choose a city");

        // Country selector
        if(!app->selector_focus) canvas_set_font(canvas, FontPrimary);
        else canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 10, 35, AlignLeft, AlignTop, "Country:");
        if(app->city_count > 0) {
            canvas_draw_str_aligned(
                canvas,
                60,
                35,
                AlignLeft,
                AlignTop,
                app->cities[app->city_index].country_name);
        }

        // City selector
        if(app->selector_focus) canvas_set_font(canvas, FontPrimary);
        else canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 10, 50, AlignLeft, AlignTop, "City:");
        if(app->city_count > 0) {
            canvas_draw_str_aligned(
                canvas, 60, 50, AlignLeft, AlignTop, app->cities[app->city_index].city_name);
        }

        // Navigation hint
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 62, AlignCenter, AlignBottom, "OK to save, Back to cancel");
        break;
    }

    case ScreenCityInfo: {
        canvas_set_font(canvas, FontPrimary);
        char title[64];
        snprintf(
            title,
            sizeof(title),
            "%s (%s)",
            app->current_city.city_name,
            app->current_city.country_code);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignTop, title);

        canvas_set_font(canvas, FontSecondary);
        char coordinates[64];
        snprintf(
            coordinates,
            sizeof(coordinates),
            "Longitude: %d°%d'",
            app->current_city.longitude_deg,
            app->current_city.longitude_min);
        canvas_draw_str_aligned(canvas, 64, 25, AlignCenter, AlignTop, coordinates);

        // Wrap description text
        char* desc = app->current_city.description;
        FuriString* wrapped = furi_string_alloc();
        elements_text_box(
            canvas,
            5,
            35,
            118,
            20,
            AlignLeft,
            AlignTop,
            desc,
            false);

        // Current time with UTC offset
        FuriHalRtcDateTime datetime;
        furi_hal_rtc_get_datetime(&datetime);
        datetime.hour += (int)app->current_city.utc_offset;
        if(datetime.hour < 0) datetime.hour += 24;
        if(datetime.hour >= 24) datetime.hour -= 24;

        char time_str[32];
        snprintf(time_str, sizeof(time_str), "Local time: %02d:%02d", datetime.hour, datetime.minute);
        canvas_draw_str_aligned(canvas, 64, 58, AlignCenter, AlignBottom, time_str);

        furi_string_free(wrapped);
        break;
    }
    }
}

static void city_info_app_input_callback(InputEvent* input_event, void* ctx) {
    FuriMessageQueue* event_queue = ctx;
    furi_assert(event_queue);

    AppEvent event = {.type = EventTypeKey, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void load_preferences(CityInfoApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    if(file_stream_open(stream, PREFERENCES_FILE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        char line[256];
        while(stream_read_line(stream, (uint8_t*)line, sizeof(line))) {
            // Simple format: city_name,country_code
            char* comma = strchr(line, ',');
            if(comma) {
                *comma = '\0';
                for(int i = 0; i < app->city_count; i++) {
                    if(strcmp(app->cities[i].city_name, line) == 0 &&
                       strcmp(app->cities[i].country_code, comma + 1) == 0) {
                        memcpy(&app->current_city, &app->cities[i], sizeof(CityInfo));
                        app->city_selected = true;
                        break;
                    }
                }
            }
        }
    }

    file_stream_close(stream);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
}

static void save_preferences(CityInfoApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    if(file_stream_open(stream, PREFERENCES_FILE, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FuriString* line = furi_string_alloc_printf(
            "%s,%s", app->current_city.city_name, app->current_city.country_code);
        stream_write_string(stream, line);
        furi_string_free(line);
    }

    file_stream_close(stream);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
}

static int load_cities_thread(void* context) {
    CityInfoApp* app = context;
    app->loading = true;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    if(file_stream_open(stream, CITIES_FILE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        // First count lines
        int line_count = 0;
        char line[256];
        while(stream_read_line(stream, (uint8_t*)line, sizeof(line))) {
            if(line[0] == '#' || strlen(line) < 5) continue; // Skip comments and empty lines
            line_count++;
        }

        // Allocate memory
        app->cities = malloc(line_count * sizeof(CityInfo));
        app->city_count = 0;

        // Rewind and read data
        stream_rewind(stream);
        while(stream_read_line(stream, (uint8_t*)line, sizeof(line))) {
            if(line[0] == '#' || strlen(line) < 5) continue;

            CityInfo* city = &app->cities[app->city_count];
            int fields = sscanf(
                line,
                "%3[^,],%31[^,],%31[^,],%31[^,],%63[^,],%f,%d,%d,%d,%d",
                city->country_code,
                city->country_name,
                city->province,
                city->city_name,
                city->description,
                &city->utc_offset,
                &city->longitude_deg,
                &city->longitude_min,
                &city->latitude_deg,
                &city->latitude_min);

            if(fields == 10) {
                app->city_count++;
            }
        }
    }

    file_stream_close(stream);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    load_preferences(app);
    app->loading = false;
    return 0;
}

static CityInfoApp* city_info_app_alloc() {
    CityInfoApp* app = malloc(sizeof(CityInfoApp));
    app->current_screen = ScreenSplash;
    app->menu_index = 0;
    app->city_selected = false;
    app->country_index = 0;
    app->city_index = 0;
    app->selector_focus = true; // Start with city focused
    app->cities = NULL;
    app->city_count = 0;
    app->loading = false;
    app->initialized = false;

    // Start loading cities in a thread
    app->thread = furi_thread_alloc();
    furi_thread_set_name(app->thread, "CityInfoLoader");
    furi_thread_set_stack_size(app->thread, 2048);
    furi_thread_set_context(app->thread, app);
    furi_thread_set_callback(app->thread, load_cities_thread);
    furi_thread_start(app->thread);

    return app;
}

static void city_info_app_free(CityInfoApp* app) {
    furi_assert(app);

    if(app->thread) {
        furi_thread_join(app->thread);
        furi_thread_free(app->thread);
    }

    if(app->cities) {
        free(app->cities);
    }

    free(app);
}

static bool city_info_app_custom_event_callback(void* context, uint32_t event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

static bool city_info_app_back_event_callback(void* context) {
    UNUSED(context);
    return false;
}

int32_t city_info_app(void* p) {
    UNUSED(p);
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(AppEvent));

    CityInfoApp* app = city_info_app_alloc();

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, city_info_app_draw_callback, app);
    view_port_input_callback_set(view_port, city_info_app_input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    AppEvent event;
    bool running = true;

    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress || event.input.type == InputTypeRepeat) {
                    switch(app->current_screen) {
                    case ScreenSplash:
                        if(event.input.key != InputKeyBack) {
                            if(app->city_selected) {
                                app->current_screen = ScreenMainMenu;
                            } else {
                                app->current_screen = ScreenChooseCity;
                            }
                        } else {
                            running = false;
                        }
                        break;

                    case ScreenMainMenu:
                        if(event.input.key == InputKeyBack) {
                            running = false;
                        } else if(event.input.key == InputKeyUp) {
                            app->menu_index--;
                            if(app->menu_index < 0) {
                                int max_items = app->city_selected ? MainMenuCount : MainMenuCount - 1;
                                app->menu_index = max_items - 1;
                            }
                        } else if(event.input.key == InputKeyDown) {
                            app->menu_index++;
                            int max_items = app->city_selected ? MainMenuCount : MainMenuCount - 1;
                            if(app->menu_index >= max_items) {
                                app->menu_index = 0;
                            }
                        } else if(event.input.key == InputKeyOk) {
                            if((app->menu_index == MainMenuChooseCity) ||
                               (!app->city_selected && app->menu_index == 0)) {
                                app->current_screen = ScreenChooseCity;
                            } else if((app->menu_index == MainMenuCityInfo) ||
                                      (app->city_selected && app->menu_index == 1)) {
                                app->current_screen = ScreenCityInfo;
                            } else if((app->menu_index == MainMenuExit) ||
                                      (!app->city_selected && app->menu_index == 1) ||
                                      (app->city_selected && app->menu_index == 2)) {
                                running = false;
                            }
                        }
                        break;

                    case ScreenChooseCity:
                        if(event.input.key == InputKeyBack) {
                            app->current_screen = ScreenMainMenu;
                        } else if(event.input.key == InputKeyLeft || event.input.key == InputKeyRight) {
                            app->selector_focus = !app->selector_focus;
                        } else if(event.input.key == InputKeyUp) {
                            if(app->selector_focus) {
                                // Change city
                                if(app->city_index > 0) app->city_index--;
                            } else {
                                // Change country (simplified - just find previous unique country)
                                if(app->city_index > 0) {
                                    char current_country[32];
                                    strcpy(current_country, app->cities[app->city_index].country_name);
                                    while(app->city_index > 0) {
                                        app->city_index--;
                                        if(strcmp(app->cities[app->city_index].country_name, current_country) != 0) {
                                            break;
                                        }
                                    }
                                }
                            }
                        } else if(event.input.key == InputKeyDown) {
                            if(app->selector_focus) {
                                // Change city
                                if(app->city_index < app->city_count - 1) app->city_index++;
                            } else {
                                // Change country (simplified - just find next unique country)
                                if(app->city_index < app->city_count - 1) {
                                    char current_country[32];
                                    strcpy(current_country, app->cities[app->city_index].country_name);
                                    while(app->city_index < app->city_count - 1) {
                                        app->city_index++;
                                        if(strcmp(app->cities[app->city_index].country_name, current_country) != 0) {
                                            break;
                                        }
                                    }
                                }
                            }
                        } else if(event.input.key == InputKeyOk) {
                            memcpy(&app->current_city, &app->cities[app->city_index], sizeof(CityInfo));
                            app->city_selected = true;
                            save_preferences(app);
                            app->current_screen = ScreenMainMenu;
                        }
                        break;

                    case ScreenCityInfo:
                        if(event.input.key == InputKeyBack || event.input.key == InputKeyOk) {
                            app->current_screen = ScreenMainMenu;
                        }
                        break;
                    }
                }
            }
        }
        view_port_update(view_port);
    }

    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    city_info_app_free(app);
    furi_record_close(RECORD_GUI);

    return 0;
}
